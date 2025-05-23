#include "../include/common.h"
#include <stdlib.h>
#include <string.h>

/*
This is just an estimate and may require further tweaking.
Currently using 50 KB
*/
#define DEFAULT_ARENA_SIZE (50 * 1024)

Arena common_arena;

bool common_InitArena() {
  common_arena.mem = malloc(DEFAULT_ARENA_SIZE);
  if (!common_arena.mem) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize arena memory for the game.\n");
#endif
    return false;
  }
  common_arena.offset = 0;
  common_arena.size = DEFAULT_ARENA_SIZE;

  return true;
}

size_t common_AllocData(size_t data_size) {
  if (data_size > common_arena.size ||
      common_arena.offset + data_size > common_arena.size) {
#ifdef DEBUG
    fprintf(stderr,
            "Arena memory can't account for the data of the given size: %zu.",
            data_size);
#endif
    // So that any and every subsequent alloc fails.
    return -1;
  }
  size_t allocated_offset = common_arena.offset;
  common_arena.offset += data_size;

  return allocated_offset;
}

bool common_SetData(uint8_t *data, size_t data_offset, size_t data_size) {
  if (data_offset > common_arena.size ||
      data_size > common_arena.size - data_offset) {
#ifdef DEBUG
    fprintf(stderr,
            "Arena doesn't have the amount of memory asked to be set.\n");
#endif
    return false;
  }

  memcpy(common_arena.mem + data_offset, data, data_size);

  return true;
}

uint8_t *common_FetchData(size_t data_offset, size_t data_size) {
  if (data_offset > common_arena.size ||
      data_size > common_arena.size - data_offset) {
#ifdef DEBUG
    fprintf(stderr,
            "Arena doesn't have the amount of memory asked to be fetched.\n");
#endif
    return false;
  }
  return (uint8_t *)(common_arena.mem + data_offset);
}

void common_FreeArena() {
  if (common_arena.mem) {
    free(common_arena.mem);
    common_arena.mem = NULL;
  }
  common_arena.offset = common_arena.size = 0;
}
