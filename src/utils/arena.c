#include "../../include/utils/arena.h"

/*
 * This arena implementation offers excellent performance when used with proper
 * bounds checking. It achieves this by simply updating metadata—no physical
 * memory movement or copying is done unless absolutely necessary.
 *
 * However, this arena is *not memory-safe*. It won't prevent you from, for
 * example, accessing memory beyond an array's capacity, potentially reading or
 * writing into unrelated memory regions. So *robust bounds checking* is
 * essential to avoid data corruption or unintended data leaks.
 *
 * Think of it like classic `malloc`/`realloc`—if used carefully, it won't crash
 * (segfault), but it also won't protect you from bugs caused by incorrect
 * indexing or misuse of allocated memory.
 */

#define FREE_SPOTS_LEN_OFFSET (0)
#define FREE_SPOTS_LEN_SIZE (sizeof(size_t))

#define FREE_SPOTS_OFFSET (FREE_SPOTS_LEN_OFFSET + FREE_SPOTS_LEN_SIZE)
#define MAX_ISOLATED_FREE_SPOTS (48)
#define FREE_SPOTS_SIZE (sizeof(FreeSpots) * MAX_ISOLATED_FREE_SPOTS)

#define METADATA_SIZE (FREE_SPOTS_LEN_SIZE + FREE_SPOTS_SIZE)

/*
 * 25 KB of total space + metadata size.
 *
 * Make sure this memory is around 2-4x the occupied memory during play-testing
 * to make sure program never crashes and there is enough memory for unexpected
 * scenarios.
 */
#define DEFAULT_ARENA_SIZE (METADATA_SIZE + (25 * 1024))

typedef struct {
  size_t offset, size;
} FreeSpots;

typedef struct {
  void *memory;
  /*
   * The following pointers fields are metadata field of the arena. They will in
   * of itself be stored on the arena in a special reserved section for
   * metadata.
   */

  size_t available_spots_len;
  FreeSpots *available_spots;
} Arena;

static Arena *arena;

/*
 * No direct access to the arena memory permitted. To manipulate the memory,
 * first use arena_AllocAndFetch() to allocate your data and then cast the
 * returned pointer to the desired data type. Then you can manipulate that
 * particular data through the cast variable.
 *
 * You can think of the above defined method be similar to malloc, just that you
 * have to mandatory cast the returned the pointer.
 */

static StatusCode AddFreeSpot(size_t offset, size_t size, int32_t left_index,
                              int32_t right_index);

StatusCode arena_Init(void) {
  arena = malloc(sizeof(Arena));
  if (!arena) {
    LOG("Unable to initialize the arena struct.");
    return FATAL_ERROR;
  }

  arena->memory = malloc(DEFAULT_ARENA_SIZE);
  if (!arena->memory) {
    LOG("Unable to initialize arena memory.");
    free(arena);
    return FATAL_ERROR;
  }
  arena_Reset();

  return SUCCESS;
}

void arena_Delete(void) {
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

void *arena_Alloc(size_t data_size) {
  assert(arena);
  void *allocated_location = NULL;

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
  // Calloc
  memset(allocated_location, 0, data_size);

  return allocated_location;
}

static StatusCode AddFreeSpot(size_t offset, size_t size, int32_t left_index,
                              int32_t right_index) {
  assert(arena);
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
  } else {
    LOG("Memory too fragmented, arena free spots limit reached");
    return RESOURCE_EXHAUSTED;
  }

  return SUCCESS;
}

/*
 * This basically exploits the thing that C can't check how memory is changes
 * within the arena. So we just update the metadata and then let the allocated
 * data think it can write more data at the same place, while nothing was
 * preventing it from doing the same in the first place.
 *
 *
 * If this returns NULL, it means original data is preserved, and it can't
 * reallocate more memory for whatever reason.
 */
void *arena_Realloc(void *old_data, size_t old_size, size_t new_size) {
  assert(arena);
  if (!old_data) {
    /*
     * Sometimes this is intended behavior to bulk dealloc if one of the data
     * point fails to allocs, in that checking for each pointer before
     * deallocating is cumbersome and unneeded.
     */
    return NULL;
  }
  size_t old_offset = old_data - arena->memory;

  /*
   * To Dear devs of the future. The arena offsets are started from arena memory
   * start and not from writable memory.
   * I spent a whole day debugging why arena was not allocating memory when it
   * clearly had enough memory. The issue was detailed further:
   *
   * Old approach:
   * if (old_offset < METADATA_SIZE || old_offset > DEFAULT_ARENA_SIZE
   * - METADATA_SIZE || old_offset + old_size > DEFAULT_ARENA_SIZE -
   * METADATA_SIZE);
   *
   * New approach:
   * if (old_offset < METADATA_SIZE || old_offset > DEFAULT_ARENA_SIZE ||
   *     old_offset + old_size > DEFAULT_ARENA_SIZE);
   *
   * You see the problem? By doing "-METADATA_SIZE", I was implying that the
   * offsets start from the writable portion of the arena and not the start,
   * which is obviously wrong and doing this capped off the storage amount
   * early.
   *
   * How stupid I was: I was thinking it was invalid offset but my mind
   * didn't ponder that the system didn't seg fault when I wrote to it.
   */
  if (old_offset < METADATA_SIZE || old_offset > DEFAULT_ARENA_SIZE ||
      old_offset + old_size > DEFAULT_ARENA_SIZE) {
    LOG("Invalid data/data-size provided for reallocation.\n");
    return NULL;
  }

  int32_t left_index = -1, right_index = -1;
  for (size_t i = 0; i < arena->available_spots_len; i++) {
    if (arena->available_spots[i].offset + arena->available_spots[i].size ==
        old_offset) {
      left_index = i;
    } else if (arena->available_spots[i].offset == old_offset + old_size) {
      right_index = i;
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
    AddFreeSpot(old_offset + new_size, old_size - new_size,
                (new_size == 0) ? left_index : -1, right_index);
    return (new_size == 0) ? NULL : old_data;
  }

  if (right_index != -1) {
    size_t right_offset = arena->available_spots[right_index].offset,
           right_size = arena->available_spots[right_index].size;
    // Chance for the arena to grow in place, rather than reallocation.
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
    // Callocing the new space.
    memset(old_data + old_size, 0, new_size - old_size);
  }

  // No growing possible, have to reallocate and add the original to free spots
  void *new_data = arena_Alloc(new_size);
  if (!new_data) {
    return NULL;
  }
  // Callocing the extra space.
  memset(new_data + old_size, 0, new_size - old_size);
  memmove(new_data, old_data, old_size);
  AddFreeSpot(old_offset, old_size, left_index, right_index);

  return new_data;
}

void arena_Reset(void) {
  assert(arena);
  memset(arena->memory, 0, DEFAULT_ARENA_SIZE);
  arena->available_spots_len = 1;

  arena->available_spots = arena->memory + FREE_SPOTS_OFFSET;
  arena->available_spots[0] = (FreeSpots){
      .offset = METADATA_SIZE, .size = DEFAULT_ARENA_SIZE - METADATA_SIZE};
}

void arena_Dump(void) {
  assert(arena);
  size_t sum = 0;

  printf("\n\nArena Status\n");
  for (size_t i = 0; i < arena->available_spots_len; i++) {
    printf("Free Spots: Offset: %zu Size: %zu\n",
           arena->available_spots[i].offset, arena->available_spots[i].size);
    sum += arena->available_spots[i].size;
  }
  printf("Total arena free space: %zu bytes\n", sum);
  printf("Total free block: %zu\n\n", arena->available_spots_len);
}
