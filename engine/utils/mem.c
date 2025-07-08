#include "mem.h"

struct __BumpArena {
  void *mem;
  u64 size;
  u64 offset;
};

BumpArena *mem_BumpArenaCreate(u64 size) {
  BumpArena *arena = malloc(sizeof(BumpArena));
  if (!arena) {
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  arena->mem = malloc(size);
  if (!(arena->mem)) {
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  arena->offset = 0;
  arena->size = size;

  return arena;
}

StatusCode mem_BumpArenaDelete(BumpArena *arena) {
  if (!arena) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return NULL;
  }

  free(arena->mem);
  free(arena);

  return SUCCESS;
}

void *mem_BumpArenaAlloc(BumpArena *arena, u64 size) {
  if (!arena) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return NULL;
  }

  if (arena->offset + size > arena->size) {
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  void *ptr = MEM_OFFSET(arena->mem, arena->offset);
  arena->offset += size;

  return ptr;
}

StatusCode mem_BumpArenaReset(BumpArena *arena) {
  if (!arena) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  arena->offset = 0;

  return SUCCESS;
}
