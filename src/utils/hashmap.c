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

/*
 * This is a abridged implementation of python's general dict class, aiming to
 * use somewhat the same principal modified to this specific use case.
 *
 * During testing It was figured out:
 * For a hashmap with 500 elements, this implementation takes about 40 kb.
 * For a hashmap with 100 elements, this implementation takes about 8.9 kb.
 * No reason for the stats, just wanted to keep it as a record.
 */
struct StrHashmap {
  /*
   * A counter of how many times we added to the hash array. This will also
   * double as a sort of len as we aim to make a perfect hash algorithm.
   */
  size_t add_count;
  /*
   * This shall always be a power of 2 to take advantage of the probing
   * algorithm effectively.
   */
  size_t indices_capacity;
  /*
   * This will be different from the indices_capacity, to prioritize low memory
   * usage and resize only when the array is full and not when the load factor
   * is met.
   */
  size_t str_hash_arr_capacity;
  /*
   * Assume this is the actual hashmap with the values it contain map to the
   * string_arr to represent what string is stored where. This will be the size
   * of the capacity of the hashmap and will be resized if its about ~70%
   * filled.
   * It will contain either a UINT32_MAX to indicate that the index is empty
   * and UINT32_MAX - 1 to indicate the index was occupied but now its
   * deleted. The deleted index will be used in the probing algorithm to not
   * stop searching early giving false negatives.
   */
  uint64_t *indices;
  /*
   * This maps to indices array to store hashes to the strings it has been
   * already calculated for. This avoids the re-calculating of hashes when
   * rehashing happens during a array resize event and then we can focus solely
   * on re-probing. They also follow the sizing scheme and layout scheme of the
   * string_arr to help save memory.
   */
  uint64_t *hashes;
  /*
   * This can be thought of as a pythonic list of strings. It has a separate
   * size scheme from the indices/hashes array. It is a step to save memory not
   * allocating large amount of string buffers which are the most memory
   * demanding. This also improves code structure by allowing us to not deal
   * with strings on many points, but rather we can just handle it once and be
   * done with it.
   */
  FixedSizeString *string_arr;
};

static StatusCode ResizeHashMapIndicesArr(StrHashmap *hashmap);
static StatusCode ResizeHashMapStrHashArr(StrHashmap *hashmap);

StrHashmap *hash_InitStrHashMap() {
  StrHashmap *hashmap = (StrHashmap *)arena_AllocAndFetch(sizeof(StrHashmap));
  if (!hashmap) {
    LOG("Can't allocate memory for StrHashmap.");
    return NULL;
  }

  hashmap->indices =
      (uint64_t *)arena_AllocAndFetch(sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE);
  hashmap->hashes =
      (uint64_t *)arena_AllocAndFetch(sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE);
  hashmap->string_arr = (FixedSizeString *)arena_AllocAndFetch(
      sizeof(FixedSizeString) * MIN_HASH_BUCKET_SIZE);

  if (!hashmap->indices || !hashmap->hashes || !hashmap->string_arr) {
    LOG("Can't allocate memory for StrHashmap.");
    arena_ReallocAndFetch((uint8_t *)hashmap->indices,
                          sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE, 0);
    arena_ReallocAndFetch((uint8_t *)hashmap->hashes,
                          sizeof(uint64_t) * MIN_HASH_BUCKET_SIZE, 0);
    arena_ReallocAndFetch((uint8_t *)hashmap->string_arr,
                          sizeof(FixedSizeString) * MIN_HASH_BUCKET_SIZE, 0);
    arena_ReallocAndFetch((uint8_t *)hashmap, sizeof(StrHashmap), 0);
    return NULL;
  }

  hashmap->add_count = 0;
  hashmap->indices_capacity = hashmap->str_hash_arr_capacity =
      MIN_HASH_BUCKET_SIZE;
  // This will set each index to EMPTY as memset works per byte.
  memset(hashmap->indices, 0xFF, sizeof(uint64_t) * hashmap->indices_capacity);

  return hashmap;
}

static StatusCode ResizeHashMapIndicesArr(StrHashmap *hashmap) {
  assert(hashmap);
  uint64_t *new_indices = (uint64_t *)arena_ReallocAndFetch(
      (uint8_t *)hashmap->indices, hashmap->indices_capacity * sizeof(uint64_t),
      hashmap->indices_capacity * 2 * sizeof(uint64_t));
  if (!new_indices) {
    LOG("Can't resize the indices array. Future failure may be imminent.")
    return FAILURE;
  }

  hashmap->indices = new_indices;
  hashmap->indices_capacity *= 2;
  // This will set each index to EMPTY as memset works per byte.
  memset(hashmap->indices, 0xFF, sizeof(uint64_t) * hashmap->indices_capacity);

  // Rehashing each string.
  uint64_t mask = hashmap->indices_capacity - 1;
  for (size_t i = 0; i < hashmap->add_count; i++) {
    uint64_t perturb = hashmap->hashes[i], j = perturb & mask;
    while (hashmap->indices[j] != EMPTY_INDEX) {
      j = (PERTURB_CONST * j + 1 + perturb) & mask;
      perturb >>= PERTURB_SHIFT;
    }
    hashmap->indices[j] = i;
  }

  return SUCCESS;
}

static StatusCode ResizeHashMapStrHashArr(StrHashmap *hashmap) {
  assert(hashmap);
  uint64_t *new_hashes = (uint64_t *)arena_ReallocAndFetch(
      (uint8_t *)hashmap->hashes,
      hashmap->str_hash_arr_capacity * sizeof(uint64_t),
      // Adding a fixed amount of new slots(not too much to save memory).
      (hashmap->str_hash_arr_capacity + MIN_HASH_BUCKET_SIZE) *
          sizeof(uint64_t));
  FixedSizeString *new_strings_arr = (FixedSizeString *)arena_ReallocAndFetch(
      (uint8_t *)hashmap->string_arr,
      hashmap->str_hash_arr_capacity * sizeof(FixedSizeString),
      // Adding a fixed amount of new slots(not too much to save memory).
      (hashmap->str_hash_arr_capacity + MIN_HASH_BUCKET_SIZE) *
          sizeof(FixedSizeString));

  if (!new_hashes || !new_strings_arr) {
    arena_ReallocAndFetch(
        (uint8_t *)new_hashes,
        (hashmap->str_hash_arr_capacity + MIN_HASH_BUCKET_SIZE) *
            sizeof(uint64_t),
        0);
    arena_ReallocAndFetch(
        (uint8_t *)new_strings_arr,
        (hashmap->str_hash_arr_capacity + MIN_HASH_BUCKET_SIZE) *
            sizeof(FixedSizeString),
        0);
    LOG("Can't resize the string and hashes array. Future failure may be "
        "imminent.")
    return FAILURE;
  }

  // Accounting for array growing doesn't need any operations.
  if (hashmap->hashes != new_hashes) {
    memcpy(new_hashes, hashmap->hashes,
           hashmap->str_hash_arr_capacity * sizeof(uint64_t));
    hashmap->hashes = new_hashes;
  }
  if (hashmap->string_arr != new_strings_arr) {
    memcpy(new_strings_arr, hashmap->string_arr,
           hashmap->str_hash_arr_capacity * sizeof(FixedSizeString));
    hashmap->string_arr = new_strings_arr;
  }
  hashmap->str_hash_arr_capacity += MIN_HASH_BUCKET_SIZE;

  return SUCCESS;
}

StatusCode hash_AddStrToMap(StrHashmap *hashmap, FixedSizeString key) {
  assert(hashmap);
  if (hashmap->add_count >= hashmap->indices_capacity * LOAD_FACTOR) {
    ResizeHashMapIndicesArr(hashmap);
  }
  if (hashmap->add_count == hashmap->str_hash_arr_capacity) {
      ResizeHashMapStrHashArr(hashmap);
  }
  if (hashmap->add_count == hashmap->str_hash_arr_capacity ||
      hashmap->add_count == hashmap->indices_capacity) {
    LOG("Hashmap is filled completely because previous resize attempt failed. "
        "Can't add anymore data.");
    return FAILURE;
  }

  uint64_t mask = hashmap->indices_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask;
  uint64_t arr_index;
  while (hashmap->indices[i] != EMPTY_INDEX &&
         hashmap->indices[i] != TOMBSTONE_INDEX) {
    arr_index = hashmap->indices[i];
    MATCH_TOKEN(hashmap->string_arr[arr_index], key) { return SUCCESS; }
    i = (PERTURB_CONST * i + 1 + perturb) & mask;
    perturb >>= PERTURB_SHIFT;
  }
  /*
   * Since str_arr and hashes act as a plain pythonic list, we only "append" the
   * string to it and the actual hashmap order is preserved in the indices
   * array.
   */
  snprintf(hashmap->string_arr[hashmap->add_count], DEFAULT_STR_BUFFER_SIZE,
           "%s", key);
  hashmap->hashes[hashmap->add_count] = hash;
  hashmap->indices[i] = hashmap->add_count++;

  return SUCCESS;
}

uint64_t hash_FetchHashIndexFromMap(StrHashmap *hashmap, FixedSizeString key) {
  assert(hashmap);
  /*
   * This is python like open list indexed hash map.
   * Here when we encounter collision we probe for a new location using the
   * perturb to generate random spread which ensures perfect hashing.
   */
  uint64_t mask = hashmap->indices_capacity - 1, hash = STR_HASHER(key);
  uint64_t perturb = hash, i = perturb & mask;
  uint64_t arr_index;
  while ((arr_index = hashmap->indices[i]) != EMPTY_INDEX) {
    MATCH_TOKEN(hashmap->string_arr[arr_index], key) { return i; }
    i = (PERTURB_CONST * i + 1 + perturb) & mask;
    perturb >>= PERTURB_SHIFT;
  }

  return EMPTY_INDEX;
}

StatusCode hash_DeleteStrFromMap(StrHashmap *hashmap, FixedSizeString key) {
  assert(hashmap);
  uint64_t i;

  if ((i = hash_FetchHashIndexFromMap(hashmap, key)) != EMPTY_INDEX) {
    hashmap->add_count--;
    uint64_t j = hash_FetchHashIndexFromMap(
        hashmap, hashmap->string_arr[hashmap->add_count]);
    uint64_t i_str_hash_index = hashmap->indices[i],
             j_str_hash_index = hashmap->add_count;
    hashmap->indices[j] = hashmap->indices[i];
    snprintf(hashmap->string_arr[i_str_hash_index], DEFAULT_STR_BUFFER_SIZE,
             "%s", hashmap->string_arr[j_str_hash_index]);
    hashmap->hashes[i_str_hash_index] = hashmap->hashes[j_str_hash_index];

    hashmap->indices[i] = TOMBSTONE_INDEX;
    return SUCCESS;
  }

  return FAILURE;
}
