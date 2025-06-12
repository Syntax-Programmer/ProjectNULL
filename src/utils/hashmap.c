#include "../../include/utils/hashmap.h"
#include "../../include/utils/arena.h"
#include "../../include/utils/array.h"
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
   * This shall always be a power of 2 to take advantage of the probing
   * algorithm effectively.
   * Its the capacity of hashmap_structure.
   */
  size_t hashmap_capacity;
  /*
   * Assume this is the actual hashmap with the values it contain map to the
   * hashmap entries to represent what is stored where. This will be the
   * size of the capacity of the hashmap and will be resized if its about ~70%
   * filled.
   * It will contain either a UINT64_MAX to indicate that the index is empty
   * and UINT64_MAX - 1 to indicate the index was occupied but now its
   * deleted. The deleted index will be used in the probing algorithm to not
   * stop searching early giving false negatives.
   */
  uint64_t *hashmap_structure;
  /*
   * The hashmap entries separate from the hashmap structure in an effort to
   * save memory.
   */
  Array *hashmap_entries;
};

static StatusCode GrowHashmapStructure(StrIntHashmap *hashmap);
static size_t GrowHashmapEntriesCallback(size_t old_size);
static void FetchStructureAndEntryIndex(StrIntHashmap *hashmap, String key,
                                        uint64_t *pStructure_index,
                                        uint64_t *pEntry_index);

StrIntHashmap *hashmap_Create() {
  StrIntHashmap *hashmap = arena_Alloc(sizeof(StrIntHashmap));
  if (!hashmap) {
    LOG("Can't allocate memory for StrIntHashmap.");
    return NULL;
  }

  hashmap->hashmap_structure =
      arena_Alloc(sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE);
  hashmap->hashmap_entries = array_Create(sizeof(HashmapEntries));

  if (!hashmap->hashmap_structure || !hashmap->hashmap_entries) {
    LOG("Can't allocate memory for StrIntHashmap.");
    hashmap_Delete(hashmap);
    return NULL;
  }

  hashmap->hashmap_capacity = MIN_HASH_BUCKET_SIZE;
  // This will set each index to EMPTY as memset works per byte.
  memset(hashmap->hashmap_structure, 0xFF,
         sizeof(uint64_t) * hashmap->hashmap_capacity);

  return hashmap;
}

void hashmap_Delete(StrIntHashmap *hashmap) {
  if (hashmap) {
    if (hashmap->hashmap_structure) {
      arena_Dealloc(hashmap->hashmap_structure,
                    sizeof(uint64_t) * hashmap->hashmap_capacity);
      hashmap->hashmap_structure = NULL;
    }
    if (hashmap->hashmap_entries) {
      array_Delete(hashmap->hashmap_entries);
      hashmap->hashmap_entries = NULL;
    }
    arena_Dealloc(hashmap, sizeof(StrIntHashmap));
  }
}

static StatusCode GrowHashmapStructure(StrIntHashmap *hashmap) {
  assert(hashmap);

  size_t old_size = hashmap->hashmap_capacity * sizeof(uint64_t);
  uint64_t *new_indices =
      arena_Realloc(hashmap->hashmap_structure, old_size, old_size * 2);
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
  HashmapEntries *entries_arr = array_RawData(hashmap->hashmap_entries);
  for (size_t i = 0; i < hashmap_GetLen(hashmap); i++) {
    // Since hashmap_len == entries_len, no bound checks needed.
    uint64_t perturb = entries_arr[i].hash, j = perturb & mask;
    while (hashmap->hashmap_structure[j] != EMPTY_INDEX) {
      PROBER(j, perturb, mask);
    }
    hashmap->hashmap_structure[j] = i;
  }

  return SUCCESS;
}

// static StatusCode GrowHashmapEntries(StrIntHashmap *hashmap) {
//   assert(hashmap);

//   size_t old_size =
//       array_GetCapacity(hashmap->hashmap_entries) * sizeof(HashmapEntries);
//   size_t new_size = old_size + (MIN_HASH_BUCKET_SIZE *
//   sizeof(HashmapEntries)); HashmapEntries *new_entries =
//       arena_Realloc(hashmap->hashmap_entries, old_size, new_size);

//   if (!new_entries) {
//     LOG("Can't resize the hashmap entries array. Future failure may be "
//         "imminent.")
//     return FAILURE;
//   }

//   hashmap->hashmap_entries = new_entries;
//   hashmap->hashmap_entries_capacity += MIN_HASH_BUCKET_SIZE;

//   return SUCCESS;
// }

static size_t GrowHashmapEntriesCallback(size_t old_size) {
  return old_size + MIN_HASH_BUCKET_SIZE;
}

StatusCode hashmap_AddEntry(StrIntHashmap *hashmap, String key, int64_t val) {
  assert(hashmap);

  size_t hashmap_len = hashmap_GetLen(hashmap);

  // No failure on growing failure due to still being some space left.
  if (hashmap_len >= hashmap->hashmap_capacity * LOAD_FACTOR) {
    GrowHashmapStructure(hashmap);
  }
  if (hashmap_len == hashmap->hashmap_capacity) {
    LOG("Hashmap is filled completely because previous resize attempt failed. "
        "Can't add anymore data.");
    return FAILURE;
  }

  uint64_t mask = hashmap->hashmap_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask, arr_index;
  HashmapEntries *entries_arr = array_RawData(hashmap->hashmap_entries);
  while (hashmap->hashmap_structure[i] != EMPTY_INDEX &&
         hashmap->hashmap_structure[i] != TOMBSTONE_INDEX) {
    arr_index = hashmap->hashmap_structure[i];
    // Since arr_index is guaranteed to give valid indices.
    MATCH_TOKEN(entries_arr[arr_index].key, key) {
      entries_arr[arr_index].val = val;
      return SUCCESS;
    }
    PROBER(i, perturb, mask);
  }
  /*
   * Since str_arr and hashes act as a plain pythonic list, we only "append" the
   * string to it and the actual hashmap order is preserved in the indices
   * array.
   */
  HashmapEntries new_entry = {.hash = hash, .val = val};
  snprintf(new_entry.key, DEFAULT_STR_BUFFER_SIZE, "%s", key);
  /*
   * We again check for size failure here and not above is due to resize trigger
   * can only be from this function. And checking for it above will give false
   * results
   */
  if (array_Push(hashmap->hashmap_entries, &new_entry,
                 GrowHashmapEntriesCallback) == FAILURE) {
    LOG("Hashmap entries array is filled completely because previous resize "
        "attempt failed. Can't add anymore data.");
    return FAILURE;
  }
  hashmap->hashmap_structure[i] = hashmap_len;

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
  uint64_t perturb = hash, i = perturb & mask, arr_index;
  HashmapEntries *entries_arr = array_RawData(hashmap->hashmap_entries);
  while ((arr_index = hashmap->hashmap_structure[i]) != EMPTY_INDEX) {
    if (arr_index != TOMBSTONE_INDEX) {
      MATCH_TOKEN(entries_arr[arr_index].key, key) {
        return entries_arr[arr_index].val;
      }
    }
    PROBER(i, perturb, mask);
  }

  return INVALID_HASHMAP_VALUE;
}

static void FetchStructureAndEntryIndex(StrIntHashmap *hashmap, String key,
                                        uint64_t *pStructure_index,
                                        uint64_t *pEntry_index) {
  if (pStructure_index) {

    *pStructure_index = EMPTY_INDEX;
  }
  if (pEntry_index) {
    *pEntry_index = EMPTY_INDEX;
  }

  uint64_t mask = hashmap->hashmap_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask, arr_index;
  HashmapEntries *entries_arr = array_RawData(hashmap->hashmap_entries);
  while ((arr_index = hashmap->hashmap_structure[i]) != EMPTY_INDEX) {
    if (arr_index != TOMBSTONE_INDEX) {
      MATCH_TOKEN(entries_arr[arr_index].key, key) {
        if (pStructure_index) {
          *pStructure_index = i;
        }
        if (pEntry_index) {
          *pEntry_index = arr_index;
        }
        return;
      }
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

  size_t hashmap_len = hashmap_GetLen(hashmap);
  HashmapEntries *entries_arr = array_RawData(hashmap->hashmap_entries);
  FetchStructureAndEntryIndex(hashmap, entries_arr[hashmap_len - 1].key,
                              &j_structure_index, NULL);

  // Repacking the array as order of this array doesn't matter.
  array_SetIndexValue(hashmap->hashmap_entries, &entries_arr[hashmap_len - 1],
                      i_entry_index);
  array_Pop(hashmap->hashmap_entries);

  // Updating the string index, as the index of the string was moved.
  hashmap->hashmap_structure[j_structure_index] =
      hashmap->hashmap_structure[i_structure_index];
  hashmap->hashmap_structure[i_structure_index] = TOMBSTONE_INDEX;
  return SUCCESS;
}

size_t hashmap_GetLen(StrIntHashmap *hashmap) {
  assert(hashmap);
  return array_GetLen(hashmap->hashmap_entries);
}
