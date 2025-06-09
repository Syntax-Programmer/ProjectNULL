#include "../../include/utils/hashmap.h"
#include "../../include/utils/arena.h"
#include <xxhash.h>

/*
 * With current implementation, this means 1KB+24Bytes of memory taken.
 * Please make sure MIN_HASH_BUCKET_SIZE is a power of 2 for the probing
 * algorithm to work correctly. It is == 16 for a good starting point.
 */
#define MIN_HASH_BUCKET_SIZE (16)
#define PERTURB_CONST (5)
#define PERTURB_SHIFT (5)
#define LOAD_FACTOR (0.75)

#define EMPTY_INDEX (UINT64_MAX)
#define TOMBSTONE_INDEX (UINT64_MAX - 1)

/*
 * Hash function: XXH3_64bits from the xxHash family by Cyan4973
 * - Non-cryptographic, high-speed hashing algorithm
 * - Produces a 64-bit hash; we truncate to 32-bit for internal use
 * - Deterministic and suitable for hash tables
 *
 * Official source: https://github.com/Cyan4973/xxHash
 */
#define STR_HASHER(str) (uint64_t)XXH3_64bits(str, strlen(str))

#define PROBER(i, perturb, mask)                                               \
  i = (PERTURB_CONST * i + 1 + perturb) & mask;                                \
  perturb >>= PERTURB_SHIFT;

/*
 * TODO: Currently this hashmap doesn't shrink on bulk deletion of entries. Not
 * implementing it rn as it will not be needed rn. This is a future addition to
 * ponder upon.
 */

typedef struct {
  String key;
  int64_t val;
  uint64_t hash;
} HashmapEntries;

/*
 * This is a abridged implementation of python's general dict class, aiming to
 * use somewhat the same principal modified to this specific use case.
 */
struct StrIntHashmap {
  /*
   * A counter of how many times we added to the hash array. This will also
   * double as a sort of len as we aim to make a perfect hash algorithm.
   */
  size_t len;
  /*
   * This shall always be a power of 2 to take advantage of the probing
   * algorithm effectively.
   */
  size_t hashmap_capacity;
  /*
   * This will be different from the hashmap_capacity, to prioritize low memory
   * usage and resize only when the array is full and not when the load factor
   * is met.
   */
  size_t hashmap_entries_capacity;
  /*
   * Assume this is the actual hashmap with the values it contain map to the
   * hashmap entries to represent what is stored where. This will be the
   * size of the capacity of the hashmap and will be resized if its about ~70%
   * filled.
   * It will contain either a UINT64_MAX to indicate that the index is empty
   * and UINT32_MAX - 1 to indicate the index was occupied but now its
   * deleted. The deleted index will be used in the probing algorithm to not
   * stop searching early giving false negatives.
   */
  uint64_t *hashmap_structure;
  /*
   * The hashmap entries separate from the hashmap structure in an effort to
   * save memory.
   */
  HashmapEntries *hashmap_entries;
};

static StatusCode GrowHashmapStructure(StrIntHashmap *hashmap);
static StatusCode GrowHashmapEntries(StrIntHashmap *hashmap);
static void FetchStructureAndEntryIndex(StrIntHashmap *hashmap,
                                        String key,
                                        uint64_t *pStructure_index,
                                        uint64_t *pEntry_index);

StrIntHashmap *hashmap_Init() {
  StrIntHashmap *hashmap =
      (StrIntHashmap *)arena_Alloc(sizeof(StrIntHashmap));
  if (!hashmap) {
    LOG("Can't allocate memory for StrIntHashmap.");
    return NULL;
  }

  hashmap->hashmap_structure =
      (uint64_t *)arena_Alloc(sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE);
  hashmap->hashmap_entries = (HashmapEntries *)arena_Alloc(
      sizeof(HashmapEntries) * MIN_HASH_BUCKET_SIZE);

  if (!hashmap->hashmap_structure || !hashmap->hashmap_entries) {
    LOG("Can't allocate memory for StrIntHashmap.");
    arena_Realloc((uint8_t *)hashmap->hashmap_structure,
                          sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE, 0);
    arena_Realloc((uint8_t *)hashmap->hashmap_entries,
                          sizeof(HashmapEntries) * MIN_HASH_BUCKET_SIZE, 0);
    arena_Realloc((uint8_t *)hashmap, sizeof(StrIntHashmap), 0);
    return NULL;
  }

  hashmap->len = 0;
  hashmap->hashmap_capacity = hashmap->hashmap_entries_capacity =
      MIN_HASH_BUCKET_SIZE;
  // This will set each index to EMPTY as memset works per byte.
  memset(hashmap->hashmap_structure, 0xFF,
         sizeof(uint64_t) * hashmap->hashmap_capacity);

  return hashmap;
}

static StatusCode GrowHashmapStructure(StrIntHashmap *hashmap) {
  assert(hashmap);
  uint64_t *new_indices = (uint64_t *)arena_Realloc(
      (uint8_t *)hashmap->hashmap_structure,
      hashmap->hashmap_capacity * sizeof(uint64_t),
      hashmap->hashmap_capacity * 2 * sizeof(uint64_t));
  if (!new_indices) {
    LOG("Can't resize the hashmap array. Future failure may be imminent.")
    return FAILURE;
  }

  hashmap->hashmap_structure = new_indices;
  hashmap->hashmap_capacity *= 2;
  // This will set each index to EMPTY as memset works per byte.
  memset(hashmap->hashmap_structure, 0xFF,
         sizeof(uint64_t) * hashmap->hashmap_capacity);

  // Rehashing each string.
  uint64_t mask = hashmap->hashmap_capacity - 1;
  for (size_t i = 0; i < hashmap->len; i++) {
    uint64_t perturb = hashmap->hashmap_entries[i].hash, j = perturb & mask;
    while (hashmap->hashmap_structure[j] != EMPTY_INDEX) {
      PROBER(j, perturb, mask);
    }
    hashmap->hashmap_structure[j] = i;
  }

  return SUCCESS;
}

static StatusCode GrowHashmapEntries(StrIntHashmap *hashmap) {
  assert(hashmap);
  HashmapEntries *new_entries = (HashmapEntries *)arena_Realloc(
      (uint8_t *)hashmap->hashmap_entries,
      hashmap->hashmap_entries_capacity * sizeof(HashmapEntries),
      (hashmap->hashmap_entries_capacity + MIN_HASH_BUCKET_SIZE) *
          sizeof(HashmapEntries));

  if (!new_entries) {
    LOG("Can't resize the hashmap entries array. Future failure may be "
        "imminent.")
    return FAILURE;
  }

  hashmap->hashmap_entries = new_entries;
  hashmap->hashmap_entries_capacity += MIN_HASH_BUCKET_SIZE;

  return SUCCESS;
}

StatusCode hashmap_AddEntry(StrIntHashmap *hashmap, String key,
                            int64_t val) {
  assert(hashmap);
  // No failure on growing failure due to still being some space left.
  if (hashmap->len >= hashmap->hashmap_capacity * LOAD_FACTOR) {
    GrowHashmapStructure(hashmap);
  }
  if (hashmap->len == hashmap->hashmap_entries_capacity) {
    GrowHashmapEntries(hashmap);
  }
  if (hashmap->len == hashmap->hashmap_entries_capacity ||
      hashmap->len == hashmap->hashmap_capacity) {
    LOG("Hashmap is filled completely because previous resize attempt failed. "
        "Can't add anymore data.");
    return FAILURE;
  }

  uint64_t mask = hashmap->hashmap_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask, arr_index;
  while (hashmap->hashmap_structure[i] != EMPTY_INDEX &&
         hashmap->hashmap_structure[i] != TOMBSTONE_INDEX) {
    arr_index = hashmap->hashmap_structure[i];
    MATCH_TOKEN(hashmap->hashmap_entries[arr_index].key, key) {
      hashmap->hashmap_entries[arr_index].val = val;
      return SUCCESS;
    }
    PROBER(i, perturb, mask);
  }
  /*
   * Since str_arr and hashes act as a plain pythonic list, we only "append" the
   * string to it and the actual hashmap order is preserved in the indices
   * array.
   */
  snprintf(hashmap->hashmap_entries[hashmap->len].key, DEFAULT_STR_BUFFER_SIZE,
           "%s", key);
  hashmap->hashmap_entries[hashmap->len].hash = hash;
  hashmap->hashmap_entries[hashmap->len].val = val;
  hashmap->hashmap_structure[i] = hashmap->len++;

  return SUCCESS;
}

int64_t hashmap_FetchValue(StrIntHashmap *hashmap, String key) {
  assert(hashmap);
  /*
   * This is python like open list indexed hash map.
   * Here when we encounter collision we probe for a new location using the
   * perturb to generate random spread which ensures perfect hashing.
   */
  uint64_t mask = hashmap->hashmap_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask;
  uint64_t arr_index;
  while ((arr_index = hashmap->hashmap_structure[i]) != EMPTY_INDEX) {
    MATCH_TOKEN(hashmap->hashmap_entries[arr_index].key, key) {
      return hashmap->hashmap_entries[arr_index].val;
    }
    PROBER(i, perturb, mask);
  }

  return INVALID_HASHMAP_VALUE;
}

static void FetchStructureAndEntryIndex(StrIntHashmap *hashmap,
                                        String key,
                                        uint64_t *pStructure_index,
                                        uint64_t *pEntry_index) {
  *pStructure_index = EMPTY_INDEX;
  *pEntry_index = EMPTY_INDEX;

  uint64_t mask = hashmap->hashmap_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask;
  uint64_t arr_index;
  while ((arr_index = hashmap->hashmap_structure[i]) != EMPTY_INDEX) {
    MATCH_TOKEN(hashmap->hashmap_entries[arr_index].key, key) {
      *pStructure_index = i;
      *pEntry_index = arr_index;
      return;
    }
    PROBER(i, perturb, mask);
  }
}

StatusCode hashmap_DeleteEntry(StrIntHashmap *hashmap, String key) {
  assert(hashmap);

  uint64_t i_structure_index, j_structure_index;
  uint64_t i_entry_index, j_entry_index;

  FetchStructureAndEntryIndex(hashmap, key, &i_structure_index, &i_entry_index);
  if (i_structure_index == EMPTY_INDEX) {
    return FAILURE;
  }

  hashmap->len--; // This can now double as the last element index.
  /*
   * Adding the last entry to the deleted entry's location to pack the array.
   * This will not conflict with the hashmap structure as it is its own
   * isolated thing.
   */
  FetchStructureAndEntryIndex(hashmap,
                              hashmap->hashmap_entries[hashmap->len].key,
                              &j_structure_index, &j_entry_index);

  hashmap->hashmap_structure[j_structure_index] =
      hashmap->hashmap_structure[i_structure_index];
  snprintf(hashmap->hashmap_entries[i_entry_index].key, DEFAULT_STR_BUFFER_SIZE,
           "%s", hashmap->hashmap_entries[j_entry_index].key);
  hashmap->hashmap_entries[i_entry_index].hash =
      hashmap->hashmap_entries[j_entry_index].hash;
  hashmap->hashmap_entries[i_entry_index].val =
      hashmap->hashmap_entries[j_entry_index].val;

  hashmap->hashmap_structure[i_structure_index] = TOMBSTONE_INDEX;
  return SUCCESS;
}

uint64_t hashmap_GetLen(StrIntHashmap *hashmap) {
    return hashmap->len;
}
