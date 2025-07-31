#include "hm.h"
#include "array.h"

/*
 * With current implementation, this means 1KB+24Bytes of memory taken.
 * Please make sure MIN_HASH_BUCKET_SIZE is a power of 2 for the probing
 * algorithm to work correctly. It is == 16 for a good starting point.
 */
#define MIN_HASH_BUCKET_SIZE (16)
#define PERTURB_CONST (5)
#define PERTURB_SHIFT (5)
#define LOAD_FACTOR (0.66)

#define EMPTY_INDEX (INT64_MAX)
/*
 * This will be used in the probing algorithm to not stop searching early giving
 * false negatives.
 */
#define TOMBSTONE_INDEX (INT64_MAX - 1)

#define PROBER(i, perturb, mask)                                               \
  i = (PERTURB_CONST * i + 1 + perturb) & mask;                                \
  perturb >>= PERTURB_SHIFT;

typedef struct {
  u64 key;
  void *val;
  u64 hash;
} Hm_IntKeyEntries;

/*
 * This is a abridged implementation of python's general dict class, aiming to
 * use somewhat the same principal modified to this specific use case.
 */
struct __HmIntKey {
  /*
   * Assume this is the actual hashmap with the values it contain map to the
   * hashmap entries to represent what is stored where. This will be the
   * size of the capacity of the hashmap and will be resized if its about ~70%
   * filled.
   */
  BuffArr *structure;
  /*
   * The hashmap entries separate from the hashmap structure in an effort to
   * save memory.
   */
  Vector *entries;
};

static inline u64 SplitMix64Hasher(u64 x);
static u64 GrowHmStructureCallback(u64 old_cap);
static u64 GrowHmEntriesCallback(u64 old_cap);
static StatusCode GrowHmIntKeyStructure(Hm_IntKey *hm);
static StatusCode FetchHmIntKeyStructureEntryIndices(Hm_IntKey *hm, u64 key,
                                                     u64 *pStructure_i,
                                                     u64 *pEntry_i);

static inline u64 SplitMix64Hasher(u64 x) {
  x += 0x9e3779b97f4a7c15;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
  x = (x ^ (x >> 27)) * 0x94d049bb133111eb;

  return x ^ (x >> 31);
}

Hm_IntKey *hm_IntKeyCreate(void) {
  Hm_IntKey *hm = malloc(sizeof(Hm_IntKey));
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(hm, NULL);

  hm->structure = arr_BuffArrCreate(MIN_HASH_BUCKET_SIZE, sizeof(u64));
  IF_NULL(hm->structure) {
    free(hm);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(hm->mem, NULL);
  }

  hm->entries = arr_VectorCreate(sizeof(Hm_IntKeyEntries));
  IF_NULL(hm->entries) {
    arr_BuffArrDelete(hm->structure);
    free(hm);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(hm->mem, NULL);
  }

  Hm_IntKeyEntries *structure = arr_BuffArrRaw(hm->structure);
  // This will set each index to EMPTY, as memset works per byte.
  memset(structure, 0xFF, sizeof(u64) * MIN_HASH_BUCKET_SIZE);

  return hm;
}

StatusCode hm_IntKeyDelete(Hm_IntKey *hm,
                           StatusCode (*val_delete_callback)(void *val)) {
  NULL_FUNC_ARG_ROUTINE(hm, FAILURE);

  if (hm->structure) {
    arr_BuffArrDelete(hm->structure);
  }
  if (hm->entries) {
    if (val_delete_callback) {
      Hm_IntKeyEntries *entries = arr_VectorRaw(hm->entries);
      u64 len = arr_VectorLen(hm->entries);
      for (u64 i = 0; i < len; i++) {
        val_delete_callback(entries[i].val);
      }
    } else {
      STATUS_LOG(WARNING, "Invalid val delete callback provided to delete "
                          "hm_IntKeyDelete. Memory leaks can occur.");
    }
    arr_VectorDelete(hm->entries);
  }
  free(hm);

  return SUCCESS;
}

static u64 GrowHmStructureCallback(u64 old_cap) { return old_cap * 2; }

static u64 GrowHmEntriesCallback(u64 old_cap) {
  return old_cap + MIN_HASH_BUCKET_SIZE;
}

static StatusCode GrowHmIntKeyStructure(Hm_IntKey *hm) {
  NULL_FUNC_ARG_ROUTINE(hm, NULL_EXCEPTION);

  IF_FUNC_FAILED(arr_BuffArrGrow(hm->structure, GrowHmStructureCallback)) {
    STATUS_LOG(FAILURE, "Cannot grow hashmap.");
    return FAILURE;
  }

  u64 mask = arr_BuffArrCap(hm->structure) - 1;
  Hm_IntKeyEntries *entries = arr_VectorRaw(hm->entries);
  u64 *structure = arr_BuffArrRaw(hm->structure);

  // First setting each index to empty for proper hashing.
  memset(structure, 0xFF, sizeof(uint64_t) * arr_BuffArrCap(hm->structure));
  // Rehashing due to size increase.
  for (u64 i = 0; i < hm_IntKeyGetLen(hm); i++) {
    u64 perturb = entries[i].hash, j = perturb & mask;
    while (structure[j] != EMPTY_INDEX) {
      PROBER(j, perturb, mask);
    }
    structure[j] = i;
  }

  return SUCCESS;
}

StatusCode hm_IntKeyAddEntry(Hm_IntKey *hm, u64 key, void *val,
                             HmAddModes mode) {
  NULL_FUNC_ARG_ROUTINE(hm, NULL_EXCEPTION);

  u64 hm_len = hm_IntKeyGetLen(hm);
  u64 structure_cap = arr_BuffArrCap(hm->structure);

  if (hm_len == structure_cap) {
    STATUS_LOG(FAILURE, "Hashmap is filled completely, previous grow attempts "
                        "must have failed.");
    return FAILURE;
  } else if (hm_len >= structure_cap * LOAD_FACTOR) {
    // Not failing here as about 30% of slots still empty.
    GrowHmIntKeyStructure(hm);
    structure_cap = arr_BuffArrCap(hm->structure);
  }

  u64 mask = arr_BuffArrCap(hm->structure) - 1;
  u64 hash = SplitMix64Hasher(key);
  u64 perturb = hash;
  u64 i = perturb & mask;
  Hm_IntKeyEntries *entries = arr_VectorRaw(hm->entries);
  u64 *structure = arr_BuffArrRaw(hm->structure);
  u64 entry_index;

  while (structure[i] != EMPTY_INDEX && structure[i] != TOMBSTONE_INDEX) {
    entry_index = structure[i]; // Can never be invalid index.
    // Key already exists in the hm.
    if (entries[entry_index].key == key) {
      if (mode == HM_ADD_FAIL) {
        STATUS_LOG(FAILURE, "Duplicate key found in failover mode.");
        return FAILURE;
      } else if (mode == HM_ADD_OVERWRITE) {
        entries[entry_index].val = val;
      }
      return SUCCESS;
    }
    PROBER(i, perturb, mask);
  }

  Hm_IntKeyEntries new_entry = {.hash = hash, .val = val, .key = key};
  IF_FUNC_FAILED(
      arr_VectorPush(hm->entries, &new_entry, GrowHmEntriesCallback)) {
    STATUS_LOG(FAILURE, "Cannot push any more entries to the hashmap.");
    return FAILURE;
  }

  arr_BuffArrSet(hm->structure, i, &hm_len);

  return SUCCESS;
}

void *hm_IntKeyFetchEntry(const Hm_IntKey *hm, u64 key) {
  NULL_FUNC_ARG_ROUTINE(hm, NULL);

  u64 mask = arr_BuffArrCap(hm->structure) - 1;
  u64 hash = SplitMix64Hasher(key);
  u64 perturb = hash;
  u64 i = perturb & mask;
  Hm_IntKeyEntries *entries = arr_VectorRaw(hm->entries);
  u64 *structure = arr_BuffArrRaw(hm->structure);
  u64 entry_index;

  while (structure[i] != EMPTY_INDEX && structure[i] != TOMBSTONE_INDEX) {
    entry_index = structure[i]; // Can never be invalid index.
    if (entries[entry_index].key == key) {
      return entries[entry_index].val;
    }
    PROBER(i, perturb, mask);
  }

  STATUS_LOG(OUT_OF_BOUNDS_ACCESS,
             "Cannot fetch a key that doesn't exist in the hm: %ld", key);
  return NULL;
}

static StatusCode FetchHmIntKeyStructureEntryIndices(Hm_IntKey *hm, u64 key,
                                                     u64 *pStructure_i,
                                                     u64 *pEntry_i) {
  NULL_FUNC_ARG_ROUTINE(hm, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(pStructure_i, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(pEntry_i, NULL_EXCEPTION);

  *pStructure_i = *pEntry_i = EMPTY_INDEX;

  u64 mask = arr_BuffArrCap(hm->structure) - 1;
  u64 hash = SplitMix64Hasher(key);
  u64 perturb = hash;
  u64 i = perturb & mask;
  u64 entry_index;

  Hm_IntKeyEntries *entries = arr_VectorRaw(hm->entries);
  u64 *structure = arr_BuffArrRaw(hm->structure);

  while (structure[i] != EMPTY_INDEX && structure[i] != TOMBSTONE_INDEX) {
    entry_index = structure[i]; // Can never be invalid index.
    if (entries[entry_index].key == key) {
      *pStructure_i = i;
      *pEntry_i = entry_index;
      return SUCCESS;
    }
    PROBER(i, perturb, mask);
  }

  return OUT_OF_BOUNDS_ACCESS;
}

StatusCode hm_IntKeyDeleteEntry(Hm_IntKey *hm, u64 key,
                                StatusCode (*val_delete_callback)(void *val)) {
  NULL_FUNC_ARG_ROUTINE(hm, NULL_EXCEPTION);

  u64 hm_len = hm_IntKeyGetLen(hm);
  Hm_IntKeyEntries *entries = arr_VectorRaw(hm->entries);
  u64 *structure = arr_BuffArrRaw(hm->structure);

  u64 key_structure_i, key_entry_i;
  u64 last_entry_structure_i, last_entry_i;

  FetchHmIntKeyStructureEntryIndices(hm, key, &key_structure_i, &key_entry_i);
  if (key_structure_i == EMPTY_INDEX) {
    STATUS_LOG(OUT_OF_BOUNDS_ACCESS,
               "Cannot delete a key that doesn't exist in the hm: %ld", key);
    return OUT_OF_BOUNDS_ACCESS;
  }
  FetchHmIntKeyStructureEntryIndices(hm, entries[hm_len - 1].key,
                                     &last_entry_structure_i, &last_entry_i);

  // Repacking the array as order of this array doesn't matter.
  if (val_delete_callback) {
    val_delete_callback(entries[key_entry_i].val);
  } else {
    STATUS_LOG(WARNING, "Invalid val delete callback provided to delete "
                        "hm_IntKeyDelete. Memory leaks can occur.");
  }

  entries[key_entry_i] = entries[last_entry_i];
  arr_VectorPop(hm->entries, NULL);

  // Updating the structure index, as the index of the last key was moved.
  structure[last_entry_structure_i] = key_entry_i;
  structure[key_structure_i] = TOMBSTONE_INDEX;

  return SUCCESS;
}

u64 hm_IntKeyGetLen(const Hm_IntKey *hm) {
  NULL_FUNC_ARG_ROUTINE(hm, NULL_EXCEPTION);

  return arr_VectorLen(hm->entries);
}

StatusCode hm_IntKeyForEach(Hm_IntKey *hm,
                            void (*foreach_callback)(const u64 key,
                                                     void *val)) {
  NULL_FUNC_ARG_ROUTINE(hm, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(foreach_callback, NULL_EXCEPTION);

  Hm_IntKeyEntries *entries = arr_VectorRaw(hm->entries);
  u64 len = hm_IntKeyGetLen(hm);

  for (u64 i = 0; i < len; i++) {
    foreach_callback(entries[i].key, entries[i].val);
  }

  return SUCCESS;
}
