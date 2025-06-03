#include "../../include/utility/arena.h"
#include <stdlib.h>

/*
This is just an estimate and may require further tweaking.
Currently using 10 KB
*/
#define DEFAULT_ARENA_SIZE (10 * 1024)

// This should theoretically correspond to 1.5 KB of meta data.
#define RESERVED_FREE_SPOTS_ARR_SIZE (96)
#define RESERVED_FREE_SPOTS_BYTES                                              \
  (sizeof(FreeSpots) * RESERVED_FREE_SPOTS_ARR_SIZE)

/*
This will be an array of structs and not struct of arrays due to the offset and
size being used together all the time.
*/
typedef struct {
  size_t offset;
  size_t size;
} FreeSpots;

typedef struct {
  // Actual arena memory
  uint8_t *mem;
  // Will an array of size RESERVED_FREE_SPOTS_ARR_SIZE
  FreeSpots *free_spots_arr;
  size_t free_spots_len;
} Arena;

static Arena arena;

/*
Some algorithms in this file, such as AddArenaFreeSpot, could be optimized if
free_spots_arr were kept sorted by offset. However, since arena allocations
only occur at program startup or during occasional resource loading, these
inefficiencies are acceptable and do not significantly impact overall
performance.
*/

/*
If this function is used from the start, there will be no worry of having rogue
free spaces that can be merged.
*/
static bool AddArenaFreeSpot(size_t old_offset, size_t old_size);

static bool AddArenaFreeSpot(size_t old_offset, size_t old_size) {
  // An attempt to merge empty block if they are adjacent.
  size_t left = -1, right = -1;

  for (size_t i = 0; i < arena.free_spots_len; i++) {
    size_t spot_offset = arena.free_spots_arr[i].offset;
    size_t spot_size = arena.free_spots_arr[i].size;

    if (spot_offset == old_offset && spot_size == old_size) {
#ifdef DEBUG
      fprintf(stderr, "The block is already freed.\n")
#endif
          return true;
    }
    if (spot_offset + spot_size == old_offset) {
      left = i;
    }
    if (old_offset + old_size == spot_offset) {
      right = i;
    }
  }

  if (left != -1 && right != -1) {
    arena.free_spots_arr[left].size +=
        old_size + arena.free_spots_arr[right].size;
    arena.free_spots_arr[right] = arena.free_spots_arr[--arena.free_spots_len];
    return true;
  } else if (left != -1) {
    arena.free_spots_arr[left].size += old_size;
    return true;
  } else if (right != -1) {
    arena.free_spots_arr[right].offset = old_offset;
    arena.free_spots_arr[right].size += old_size;
    return true;
  } else if (arena.free_spots_len == RESERVED_FREE_SPOTS_ARR_SIZE) {
#ifdef DEBUG
    fprintf(stderr, "WARNING: Memory is too fragmented, no further "
                    "reallocation possible.\n");
#endif
    return false;
  }
  arena.free_spots_arr[arena.free_spots_len].offset = old_offset;
  arena.free_spots_arr[arena.free_spots_len].size = old_size;
  arena.free_spots_len++;

  return true;
}

bool arena_Init() {
  arena.mem = malloc(DEFAULT_ARENA_SIZE);
  if (!arena.mem) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize arena memory for the game.\n");
#endif
    return false;
  }

  arena.free_spots_len = 0;
  arena.free_spots_arr = (FreeSpots *)arena.mem;
  memset(arena.free_spots_arr, 0, RESERVED_FREE_SPOTS_BYTES);
  // Initialize first free spot to be the rest of the arena
  arena.free_spots_arr[0].size = DEFAULT_ARENA_SIZE - RESERVED_FREE_SPOTS_BYTES;
  arena.free_spots_arr[0].offset = RESERVED_FREE_SPOTS_BYTES;
  arena.free_spots_len++;

  return true;
}

size_t arena_AllocData(size_t data_size) {
  size_t empty_offset = INVALID_OFFSET;

  for (size_t i = 0; i < arena.free_spots_len; i++) {
    if (arena.free_spots_arr[i].size == data_size) {
      empty_offset = arena.free_spots_arr[i].offset;
      // Packing the array for no dead bytes in the middle of the arr.
      arena.free_spots_arr[i] = arena.free_spots_arr[--arena.free_spots_len];
      break;
    } else if (arena.free_spots_arr[i].size > data_size) {
      empty_offset = arena.free_spots_arr[i].offset;
      arena.free_spots_arr[i].size -= data_size;
      arena.free_spots_arr[i].offset += data_size;
      break;
    }
  }

  if (empty_offset == INVALID_OFFSET) {
#ifdef DEBUG
    fprintf(stderr,
            "Arena memory can't account for the data of the given size: %zu.",
            data_size);
#endif
  }

  return empty_offset;
}

bool arena_SetData(uint8_t *data, size_t data_offset, size_t data_size) {
  if (data_offset > DEFAULT_ARENA_SIZE ||
      data_size > DEFAULT_ARENA_SIZE - data_offset) {
#ifdef DEBUG
    fprintf(stderr,
            "Arena doesn't have the amount of memory asked to be set.\n");
#endif
    return false;
  }
  if (data_offset < RESERVED_FREE_SPOTS_BYTES) {
#ifdef DEBUG
    fprintf(
        stderr,
        "WARNING: Can't override reserved memory for tracking free spaces.\n");
#endif
    return false;
  }

  memcpy(arena.mem + data_offset, data, data_size);

  return true;
}

/*
If this returns (size_t)-1 means the original data is at its place and not
freed.
*/
size_t arena_ReallocData(size_t original_data_offset, size_t data_size,
                         size_t new_size) {
  if (original_data_offset > DEFAULT_ARENA_SIZE ||
      data_size > DEFAULT_ARENA_SIZE - original_data_offset) {
#ifdef DEBUG
    fprintf(
        stderr,
        "Arena doesn't have the amount of memory asked to be reallocated.\n");
#endif
    return INVALID_OFFSET;
  }
  if (original_data_offset < RESERVED_FREE_SPOTS_BYTES) {
#ifdef DEBUG
    fprintf(stderr, "WARNING: Can't reallocate reserved memory for tracking "
                    "free spaces.\n");
#endif
    return INVALID_OFFSET;
  }
  if (new_size == data_size) {
#ifdef DEBUG
    fprintf(stderr, "Cannot reallocate to the same size.\n");
#endif
    return original_data_offset;
  }
  // Treating this as a deallocation call.
  if (new_size == 0) {
#ifdef DEBUG
    fprintf(stderr, "Reallocation with size 0 is treated as free.\n");
#endif
    if (!AddArenaFreeSpot(original_data_offset, data_size)) {
#ifdef DEBUG
      fprintf(stderr,
              "Deallocation of data at offset %zu with size %zu failed. Data "
              "is still persistent.\n",
              original_data_offset, data_size);
#endif
      return original_data_offset;
    }
    /*
    It is up to the caller to appropriately handle dangling pointer after free
    call has been issued.
    */
    return INVALID_OFFSET;
  }

  /*
  TODO: This doesn't currently grow the existing array if space is available. It
  always finds a new location. This is a problem.
  */
  size_t new_offset;
  if ((new_offset = arena_AllocData(new_size)) == INVALID_OFFSET) {
    return new_offset;
  }
  memcpy(arena_FetchData(new_offset, new_size),
         arena_FetchData(original_data_offset, data_size), data_size);
  AddArenaFreeSpot(original_data_offset, data_size);

  return new_offset;
}

uint8_t *arena_FetchData(size_t data_offset, size_t data_size) {
  if (data_offset > DEFAULT_ARENA_SIZE ||
      data_size > DEFAULT_ARENA_SIZE - data_offset) {
#ifdef DEBUG
    fprintf(stderr, "Arena doesn't have the amount of memory asked to be"
                    "fetched.\n");
#endif
    return NULL;
  }
  if (data_offset < RESERVED_FREE_SPOTS_BYTES) {
#ifdef DEBUG
    fprintf(
        stderr,
        "WARNING: Can't access reserved memory for tracking free spaces.\n");
#endif
    return NULL;
  }

  return (uint8_t *)(arena.mem + data_offset);
}

void arena_Free() {
  if (arena.mem) {
    free(arena.mem);
    arena.mem = NULL;
  }
  arena.free_spots_arr = NULL;
  arena.free_spots_len = 0;
}
