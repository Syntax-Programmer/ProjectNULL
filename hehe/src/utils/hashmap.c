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
#define LOAD_FACTOR (0.66)

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
  CharBuffer key;
  int64_t val;
  uint64_t hash;
} HashmapEntries;

/*
 * This is a abridged implementation of python's general dict class, aiming to
 * use somewhat the same principal modified to this specific use case.
 */
struct StrIntHashmap {
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
  FlexArr *hm_structure; // uint64_t array
  /*
   * The hashmap entries separate from the hashmap structure in an effort to
   * save memory.
   */
  AppendArr *hm_entries; // HashmapEntries array
};

static size_t GrowHashmapStructureCallback(size_t old_capacity);
static size_t GrowHashmapEntriesCallback(size_t old_size);
static void FetchStructureAndEntryIndex(StrIntHashmap *hashmap, CharBuffer key,
                                        uint64_t *pStructure_index,
                                        uint64_t *pEntry_index);

StrIntHashmap *hm_Create(void) {
  StrIntHashmap *hashmap = arena_Alloc(sizeof(StrIntHashmap));
  if (!hashmap) {
    LOG("Can not allocate memory for StrIntHashmap.");
    return NULL;
  }

  hashmap->hm_structure =
      arr_FlexArrCreate(sizeof(uint64_t), MIN_HASH_BUCKET_SIZE);
  hashmap->hm_entries = arr_AppendArrCreate(sizeof(HashmapEntries));

  if (!hashmap->hm_structure || !hashmap->hm_entries) {
    LOG("Can not allocate memory for StrIntHashmap.");
    hm_Delete(hashmap);
    return NULL;
  }

  FlexArr *structure_array = arr_GetFlexArrRawData(hashmap->hm_structure);
  // This will set each index to EMPTY as memset works per byte.
  memset(structure_array, 0xFF, sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE);

  return hashmap;
}

void hm_Delete(StrIntHashmap *hashmap) {
  if (hashmap) {
    if (hashmap->hm_structure) {
      arr_FlexArrDelete(hashmap->hm_structure);
      hashmap->hm_structure = NULL;
    }
    if (hashmap->hm_entries) {
      arr_AppendArrDelete(hashmap->hm_entries);
      hashmap->hm_entries = NULL;
    }
    arena_Dealloc(hashmap, sizeof(StrIntHashmap));
  }
}

static size_t GrowHashmapStructureCallback(size_t old_capacity) {
  return old_capacity * 2;
}

static size_t GrowHashmapEntriesCallback(size_t old_capacity) {
  return old_capacity + MIN_HASH_BUCKET_SIZE;
}

static StatusCode GrowHashmapStructure(StrIntHashmap *hashmap) {
  assert(hashmap);
  if (arr_GrowFlexArr(hashmap->hm_structure, GrowHashmapStructureCallback) ==
      RESOURCE_EXHAUSTED) {
    LOG("Can not resize the hashmap structure array.");
    return RESOURCE_EXHAUSTED;
  }
  FlexArr *structure_array = arr_GetFlexArrRawData(hashmap->hm_structure);
  // This will set each index to EMPTY as memset works per byte.
  memset(structure_array, 0xFF,
         sizeof(uint64_t) * arr_GetFlexArrCapacity(hashmap->hm_structure));

  // Rehashing each string.
  uint64_t mask = arr_GetFlexArrCapacity(hashmap->hm_structure) - 1;
  HashmapEntries *entries_arr = arr_GetAppendArrRawData(hashmap->hm_entries);
  uint64_t *structure_arr = arr_GetFlexArrRawData(hashmap->hm_structure);

  for (size_t i = 0; i < hm_GetLen(hashmap); i++) {
    // Since hm_len == entries_len, no bound checks needed.
    uint64_t perturb = entries_arr[i].hash, j = perturb & mask;
    while (structure_arr[j] != EMPTY_INDEX) {
      PROBER(j, perturb, mask);
    }
    structure_arr[j] = i;
  }

  return SUCCESS;
}

StatusCode hm_AddEntry(StrIntHashmap *hashmap, CharBuffer key, int64_t val) {
  assert(hashmap);
  size_t hm_len = hm_GetLen(hashmap);
  size_t structure_capacity = arr_GetFlexArrCapacity(hashmap->hm_structure);

  // No failure on growing failure due to still being some space left.
  if (hm_len >= structure_capacity * LOAD_FACTOR) {
    GrowHashmapStructure(hashmap);
    // Capacity can change so reassigning.
    structure_capacity = arr_GetFlexArrCapacity(hashmap->hm_structure);
  }
  if (hm_len == structure_capacity) {
    LOG("Hashmap is filled completely because previous resize attempt failed. "
        "Can not add anymore data.");
    return RESOURCE_EXHAUSTED;
  }

  uint64_t mask = structure_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask, arr_index;
  HashmapEntries *entries_arr = arr_GetAppendArrRawData(hashmap->hm_entries);
  uint64_t *structure_arr = arr_GetFlexArrRawData(hashmap->hm_structure);

  while (structure_arr[i] != EMPTY_INDEX &&
         structure_arr[i] != TOMBSTONE_INDEX) {
    arr_index = structure_arr[i];
    // Since arr_index is guaranteed to give valid indices.
    IF_CHARBUFF_EQUALS(entries_arr[arr_index].key, key) {
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
  snprintf(new_entry.key, CHAR_BUFFER_SIZE, "%s", key);
  /*
   * We again check for size failure here and not above is due to resize trigger
   * can only be from this function. And checking for it above will give false
   * results
   */
  if (arr_AppendArrPush(hashmap->hm_entries, &new_entry,
                          GrowHashmapEntriesCallback) == RESOURCE_EXHAUSTED) {
    LOG("Hashmap entries array is filled completely because previous resize "
        "attempt failed. Can not add anymore data.");
    return RESOURCE_EXHAUSTED;
  }
  // i Can not ever be out of bounds.
  arr_SetFlexArrIndexValue(hashmap->hm_structure, &hm_len, i);

  return SUCCESS;
}

int64_t hm_FetchValue(StrIntHashmap *hashmap, CharBuffer key) {
  assert(hashmap);
  /*
   * This is python like open list indexed hash map.
   * Here when we encounter collision we probe for a new location using the
   * perturb to generate random spread which ensures perfect hashing.
   */
  uint64_t mask = arr_GetFlexArrCapacity(hashmap->hm_structure) - 1,
           hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask, arr_index;
  HashmapEntries *entries_arr = arr_GetAppendArrRawData(hashmap->hm_entries);
  uint64_t *structure_arr = arr_GetFlexArrRawData(hashmap->hm_structure);

  while ((arr_index = structure_arr[i]) != EMPTY_INDEX) {
    if (arr_index != TOMBSTONE_INDEX) {
      IF_CHARBUFF_EQUALS(entries_arr[arr_index].key, key) {
        return entries_arr[arr_index].val;
      }
    }
    PROBER(i, perturb, mask);
  }

  return INVALID_HASHMAP_VALUE;
}

static void FetchStructureAndEntryIndex(StrIntHashmap *hashmap, CharBuffer key,
                                        uint64_t *pStructure_index,
                                        uint64_t *pEntry_index) {
  if (pStructure_index) {
    *pStructure_index = EMPTY_INDEX;
  }
  if (pEntry_index) {
    *pEntry_index = EMPTY_INDEX;
  }

  uint64_t mask = arr_GetFlexArrCapacity(hashmap->hm_structure) - 1,
           hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask, arr_index;
  HashmapEntries *entries_arr = arr_GetAppendArrRawData(hashmap->hm_entries);
  uint64_t *structure_arr = arr_GetFlexArrRawData(hashmap->hm_structure);

  while ((arr_index = structure_arr[i]) != EMPTY_INDEX) {
    if (arr_index != TOMBSTONE_INDEX) {
      IF_CHARBUFF_EQUALS(entries_arr[arr_index].key, key) {
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

StatusCode hm_DeleteEntry(StrIntHashmap *hashmap, CharBuffer key) {
  assert(hashmap);
  uint64_t i_structure_index, j_structure_index;
  uint64_t i_entry_index;

  FetchStructureAndEntryIndex(hashmap, key, &i_structure_index, &i_entry_index);
  if (i_structure_index == EMPTY_INDEX) {
    LOG("The key to delete: %s, doesn't exist", key);
    return WARNING;
  }

  size_t hm_len = hm_GetLen(hashmap);
  HashmapEntries *entries_arr = arr_GetAppendArrRawData(hashmap->hm_entries);
  FetchStructureAndEntryIndex(hashmap, entries_arr[hm_len - 1].key,
                              &j_structure_index, NULL);

  // Repacking the array as order of this array doesn't matter.
  entries_arr[i_entry_index] = entries_arr[hm_len - 1];
  arr_AppendArrPop(hashmap->hm_entries);

  // Updating the string index, as the index of the string was moved.
  uint64_t *structure_arr = arr_GetFlexArrRawData(hashmap->hm_structure);
  structure_arr[j_structure_index] = structure_arr[i_structure_index];
  structure_arr[i_structure_index] = TOMBSTONE_INDEX;

  return SUCCESS;
}

size_t hm_GetLen(StrIntHashmap *hashmap) {
  assert(hashmap);
  return arr_GetAppendArrLen(hashmap->hm_entries);
}

void hm_ForEach(StrIntHashmap *hashmap,
                void (*foreach_callback)(const CharBuffer key, int64_t *pVal)) {
  HashmapEntries *entries_arr = arr_GetAppendArrRawData(hashmap->hm_entries);
  size_t len = hm_GetLen(hashmap);

  for (size_t i = 0; i < len; i++) {
    foreach_callback(entries_arr[i].key, &entries_arr[i].val);
  }
}
