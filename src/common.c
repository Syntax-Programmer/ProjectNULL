#include "../include/common.h"
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
  int32_t free_spots_len;
} Arena;

static Arena common_arena;

bool common_InitArena() {
  common_arena.mem = malloc(DEFAULT_ARENA_SIZE);
  if (!common_arena.mem) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize arena memory for the game.\n");
#endif
    return false;
  }

  common_arena.free_spots_len = 0;
  common_arena.free_spots_arr = (FreeSpots *)common_arena.mem;
  memset(common_arena.free_spots_arr, 0, RESERVED_FREE_SPOTS_BYTES);
  // Initialize first free spot to be the rest of the arena
  common_arena.free_spots_arr[0].size =
      DEFAULT_ARENA_SIZE - RESERVED_FREE_SPOTS_BYTES;
  common_arena.free_spots_arr[0].offset = RESERVED_FREE_SPOTS_BYTES;
  common_arena.free_spots_len++;

  return true;
}

size_t common_AllocData(size_t data_size) {
  size_t empty_offset = (size_t)-1;

  for (int32_t i = 0; i < common_arena.free_spots_len; i++) {
    if (common_arena.free_spots_arr[i].size == data_size) {
      empty_offset = common_arena.free_spots_arr[i].offset;
      // Packing the array for no dead bytes in the middle of the arr.
      common_arena.free_spots_arr[i] =
          common_arena.free_spots_arr[--common_arena.free_spots_len];
      break;
    } else if (common_arena.free_spots_arr[i].size > data_size) {
      empty_offset = common_arena.free_spots_arr[i].offset;
      common_arena.free_spots_arr[i].size -= data_size;
      common_arena.free_spots_arr[i].offset += data_size;
      break;
    }
  }

  if (empty_offset == (size_t)-1) {
#ifdef DEBUG
    fprintf(stderr,
            "Arena memory can't account for the data of the given size: %zu.",
            data_size);
#endif
  }

  return empty_offset;
}

bool common_SetData(uint8_t *data, size_t data_offset, size_t data_size) {
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

  memcpy(common_arena.mem + data_offset, data, data_size);

  return true;
}

/*
If this returns (size_t)-1 means the original data is at its place and not
freed.
*/
size_t common_ReallocData(size_t original_data_offset, size_t data_size,
                          size_t new_size) {
  if (original_data_offset > DEFAULT_ARENA_SIZE ||
      data_size > DEFAULT_ARENA_SIZE - original_data_offset) {
#ifdef DEBUG
    fprintf(
        stderr,
        "Arena doesn't have the amount of memory asked to be reallocated.\n");
#endif
    return (size_t)-1;
  }
  if (original_data_offset < RESERVED_FREE_SPOTS_BYTES) {
#ifdef DEBUG
    fprintf(stderr, "WARNING: Can't reallocate reserved memory for tracking "
                    "free spaces.\n");
#endif
    return (size_t)-1;
  }
  if (new_size == data_size) {
#ifdef DEBUG
    fprintf(stderr, "Cannot reallocate to the same size.\n");
#endif
  }

  size_t new_offset;
  if ((new_offset = common_AllocData(new_size)) == (size_t)-1) {
    return new_offset;
  }

  memcpy(common_FetchData(new_offset, new_size),
         common_FetchData(original_data_offset, data_size), data_size);

  common_arena.free_spots_arr[common_arena.free_spots_len].offset =
      original_data_offset;
  common_arena.free_spots_arr[common_arena.free_spots_len].size = data_size;

  common_arena.free_spots_len++;

  return new_offset;
}

uint8_t *common_FetchData(size_t data_offset, size_t data_size) {
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

  return (uint8_t *)(common_arena.mem + data_offset);
}

void common_FreeArena() {
  if (common_arena.mem) {
    free(common_arena.mem);
    common_arena.mem = NULL;
  }
  common_arena.free_spots_arr = NULL;
  common_arena.free_spots_len = 0;
}
