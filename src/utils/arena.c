#include "../../include/utils/arena.h"
#include <stdlib.h>

#define FREE_SPOTS_LEN_OFFSET (0)
#define FREE_SPOTS_LEN_SIZE (sizeof(size_t))

#define FREE_SPOTS_OFFSET (FREE_SPOTS_LEN_OFFSET + FREE_SPOTS_LEN_SIZE)
#define MAX_ISOLATED_FREE_SPOTS (48)
#define FREE_SPOTS_SIZE (sizeof(FreeSpots) * MAX_ISOLATED_FREE_SPOTS)

#define METADATA_SIZE (FREE_SPOTS_LEN_SIZE + FREE_SPOTS_SIZE)

// 10 KB of total space + metadata size.
#define DEFAULT_ARENA_SIZE (METADATA_SIZE + (10 * 1024))

typedef struct {
  size_t offset, size;
} FreeSpots;

struct Arena {
  uint8_t *memory;

  /*
   * The following pointers fields are metadata field of the arena. They will in
   * of itself be stored on the arena in a special reserved section for
   * metadata.
   */

  size_t available_spots_len;
  FreeSpots *available_spots;
};

/*
 * No direct access to the arena memory permitted. To manipulate the memory,
 * first use arena_AllocAndFetch() to allocate your data and then cast the
 * returned pointer to the desired data type. Then you can manipulate that
 * particular data through the cast variable.
 *
 * You can think of the above defined method be similar to malloc, just that you
 * have to mandatory cast the returned the pointer.
 */

static void SortWRTOffset(FreeSpots *available_spots,
                          size_t available_spots_len);
static StatusCode AddFreeSpot(Arena *arena, size_t offset, size_t size,
                              int32_t left_index, int32_t right_index);

Arena *arena_Create(void) {
  Arena *arena = malloc(sizeof(Arena));
  if (!arena) {
    LOG("Unable to initialize the arena struct.");
    return NULL;
  }

  arena->memory = malloc(DEFAULT_ARENA_SIZE);
  if (!arena->memory) {
    LOG("Unable to initialize arena memory.");
    free(arena);
    return NULL;
  }
  arena_Reset(arena);

  return arena;
}

void arena_Delete(Arena *arena) {
  if (arena) {
    if (arena->memory) {
      free(arena->memory);
      arena->memory = NULL;
    }
    arena->available_spots = NULL;
    arena->available_spots_len = 0;
    free(arena);
  }
}

uint8_t *arena_AllocAndFetch(Arena *arena, size_t data_size) {
  uint8_t *allocated_location = NULL;

  for (size_t i = 0; i < arena->available_spots_len; i++) {
    if (arena->available_spots[i].size == data_size) {
      allocated_location = arena->memory + arena->available_spots[i].offset;
      arena->available_spots[i] =
          arena->available_spots[--arena->available_spots_len];
      break;
    } else if (arena->available_spots[i].size > data_size) {
      allocated_location = arena->memory + arena->available_spots[i].offset;
      arena->available_spots[i].offset += data_size;
      arena->available_spots[i].size -= data_size;
      break;
    }
  }

  if (!allocated_location) {
    LOG("Couldn't find appropriate memory chunk to allocate.");
  }

  return allocated_location;
}

static void SortWRTOffset(FreeSpots *available_spots,
                          size_t available_spots_len) {
  size_t offset_1, offset_2;
  FreeSpots temp;

  /*
   * Using inserting sort here as majority of time only 1 singular offset will
   * be out of order and rest will be ordered. So this provides best case O(N)
   * time complexity
   */
  for (size_t i = 0; i < available_spots_len; i++) {
    for (int32_t j = i - 1; j >= 0; j--) {
      offset_1 = available_spots[j].offset;
      offset_2 = available_spots[j + 1].offset;
      if (offset_1 < offset_2) {
        break;
      }
      temp = available_spots[j];
      available_spots[j] = available_spots[j + 1];
      available_spots[j + 1] = temp;
    }
  }
}

static StatusCode AddFreeSpot(Arena *arena, size_t offset, size_t size,
                              int32_t left_index, int32_t right_index) {
  /*
   * This has a chance to disrupt the sorting, but this method is always called
   * after sorting so it won't matter.
   */
  if (left_index != -1 && right_index != -1) {
    // Merging all 3 adjacent block.
    arena->available_spots[left_index].size +=
        size + arena->available_spots[right_index].size;
    arena->available_spots[right_index] =
        arena->available_spots[--arena->available_spots_len];
  } else if (left_index != -1) {
    arena->available_spots[left_index].size += size;
  } else if (right_index != -1) {
    arena->available_spots[right_index].offset = offset;
    arena->available_spots[right_index].size += size;
  } else if (arena->available_spots_len < MAX_ISOLATED_FREE_SPOTS) {
    arena->available_spots[arena->available_spots_len++] =
        (FreeSpots){.offset = offset, .size = size};
    if (arena->available_spots_len == MAX_ISOLATED_FREE_SPOTS) {
      LOG("Memory too fragmented, arena free spots limit reached");
    }
  } else {
    return FAILURE;
  }
  return SUCCESS;
}

/*
 * If this returns NULL, it means original data is preserved, and it can't
 * reallocate more memory for whatever reason.
 */
uint8_t *arena_ReallocAndFetch(Arena *arena, uint8_t *old_data, size_t old_size,
                               size_t new_size) {
  size_t old_offset = old_data - arena->memory;

  if (old_offset < METADATA_SIZE ||
      old_offset > DEFAULT_ARENA_SIZE - METADATA_SIZE ||
      old_offset + old_size > DEFAULT_ARENA_SIZE - METADATA_SIZE) {
    LOG("Invalid data/data-size provided for reallocation.\n");
    return NULL;
  }

  // Sorting will allow for optimizations and other stuff.
  SortWRTOffset(arena->available_spots, arena->available_spots_len);

  int32_t left_index = -1, right_index = -1;
  for (size_t i = 0; i < arena->available_spots_len; i++) {
    if (arena->available_spots[i].offset < old_offset) {
      left_index = i;
    } else if (arena->available_spots[i].offset > old_offset) {
      right_index = i;
      // Early exit due to sorting.
      break;
    }
  }
  if (new_size == old_size) {
    LOG("Can't realloc to the same old size.");
    return NULL;
  } else if (new_size < old_size) {
    /*
     * if new_size is 0, it will be deallocated automatically else it would be
     * shrunk.
     *
     * The left_index in case of shrinking will be -1 as there is no neighbors
     * that on the left side merge.
     */
    AddFreeSpot(arena, old_offset + new_size, old_size - new_size,
                (new_size == 0) ? left_index : -1, right_index);
    return (new_size == 0) ? NULL : old_data;
  }

  size_t right_offset = arena->available_spots[right_index].offset,
         right_size = arena->available_spots[right_index].size;

  // Chance for the arena to grow, rather than reallocation.
  if (old_offset + old_size == right_offset) {
    if (old_offset + new_size == right_offset + right_size) {
      // Can exactly grow into the adjacent block.
      arena->available_spots[right_index] =
          arena->available_spots[arena->available_spots_len--];
      return old_data;
    } else if (old_offset + new_size < right_offset + right_size) {
      // Can grow to partially fill the available memory
      arena->available_spots[right_index].offset += new_size - old_size;
      arena->available_spots[right_index].size -= new_size - old_size;
      return old_data;
    }
  }
  // No growing possible, have to reallocate and add the original to free spots
  uint8_t *new_offset = arena_AllocAndFetch(arena, new_size);
  if (!new_offset) {
    return NULL;
  }
  memcpy(new_offset, old_data, old_size);
  AddFreeSpot(arena, old_offset, old_size, left_index, right_index);

  return new_offset;
}

void arena_Reset(Arena *arena) {
  memset(arena->memory, 0, DEFAULT_ARENA_SIZE);
  arena->available_spots_len = 1;

  arena->available_spots = (FreeSpots *)(arena->memory + FREE_SPOTS_OFFSET);
  arena->available_spots[0] =
      (FreeSpots){.offset = FREE_SPOTS_OFFSET + FREE_SPOTS_SIZE,
                  .size = DEFAULT_ARENA_SIZE - METADATA_SIZE};
}
