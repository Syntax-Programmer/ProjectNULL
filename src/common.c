#include "../include/common.h"
#include <stdlib.h>
#include <string.h>

Arena common_arena;

bool common_InitArena() {
  common_arena.mem = malloc(DEFAULT_ARENA_SIZE);
  if (!common_arena.mem) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize arena memory for the game.\n");
#endif
    return false;
  }

  return true;
}

size_t common_AllocData(void *pData, size_t data_size) {
  if (!pData) {
#ifdef DEBUG
    fprintf(stderr, "Source data is NULL.\n");
#endif
    return -1;
  }

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

  memcpy(common_arena.mem + common_arena.offset, pData, data_size);
  common_arena.offset += data_size;

  return common_arena.offset;
}

bool common_FetchData(size_t data_offset, size_t data_size, void *pDest) {
  if (!pDest) {
#ifdef DEBUG
    fprintf(stderr, "Destination pointer is NULL.\n");
#endif
    return false;
  }

  if (data_offset > common_arena.size ||
      data_size > common_arena.size - data_offset) {
#ifdef DEBUG
    fprintf(stderr,
            "Arena doesn't have the amount of memory asked to be fetched.\n");
#endif
    return false;
  }

  memcpy(pDest, &common_arena.mem[data_offset], data_size);
  return true;
}
