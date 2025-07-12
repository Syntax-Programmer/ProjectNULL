#include "mem.h"
#include "common.h"
#include <cstdio>
#include <cstdlib>

/* ----  BUMP ARENA  ---- */

struct __BumpArena {
  void *mem;
  u64 size;
  u64 offset;
};

BumpArena *mem_BumpArenaCreate(u64 size) {
  BumpArena *arena = malloc(sizeof(BumpArena));
  if (!arena) {
    printf("Can not create bump arena, memory failure.\n");
    return NULL;
  }

  arena->mem = malloc(size);
  if (!(arena->mem)) {
    free(arena);
    printf("Can not create bump arena, memory failure.\n");
    return NULL;
  }

  arena->offset = 0;
  arena->size = size;

  return arena;
}

StatusCode mem_BumpArenaDelete(BumpArena *arena) {
  if (!arena) {
    printf("Can not delete an invalid bump arena.\n");
    return NULL;
  }

  free(arena->mem);
  free(arena);

  return SUCCESS;
}

void *mem_BumpArenaAlloc(BumpArena *arena, u64 size) {
  if (!arena) {
    printf("Can not allocate from an invalid bump arena.\n");
    return NULL;
  }

  if (arena->offset + size > arena->size) {
    printf("Bump arena has: %zu, while allocation attempt of: %zu.\n",
           arena->size - arena->offset, size);
    return NULL;
  }

  void *ptr = MEM_OFFSET(arena->mem, arena->offset);
  arena->offset += size;

  return ptr;
}

StatusCode mem_BumpArenaReset(BumpArena *arena) {
  if (!arena) {
    printf("Can not reset an invalid bump arena.\n");
    return FAILURE;
  }

  memset(arena->mem, 0, arena->size);
  arena->offset = 0;

  return SUCCESS;
}

/* ----  POOL ARENA  ---- */

#define STD_POOL_SIZE (16)

struct __PoolArena {
  void *mem;
  void *free_list;
  u64 block_size;
  struct __PoolArena *next;
};

static StatusCode AddPoolLink(PoolArena **pHead, u64 block_size);

static StatusCode AddPoolLink(PoolArena **pHead, u64 block_size) {
  PoolArena *arena = malloc(sizeof(PoolArena));
  if (!arena) {
    printf("Can link new pool arena, memory failure.\n");
    return FAILURE;
  }

  arena->mem = malloc(block_size * STD_POOL_SIZE);
  if (!(arena->mem)) {
    free(arena);
    printf("Can link new pool arena, memory failure.\n");
    return FAILURE;
  }

  arena->free_list = arena->mem;
  u8 *curr = (u8 *)arena->mem;
  for (u64 i = 0; i < STD_POOL_SIZE - 1; i++) {
    *(void **)curr = curr + block_size;
    curr += block_size;
  }
  *(void **)curr = NULL; // Last block points to NULL

  arena->block_size = block_size;

  arena->next = *pHead;
  *pHead = arena;

  return SUCCESS;
}

PoolArena *mem_PoolArenaCreate(u64 block_size) {
  block_size = (block_size < sizeof(void *)) ? sizeof(void *) : block_size;

  PoolArena *arena = NULL;

  AddPoolLink(&arena, block_size);

  return arena;
}

StatusCode mem_PoolArenaDelete(PoolArena *arena) {
  if (!arena) {
    printf("Can not delete an invalid pool arena.\n");
    return FAILURE;
  }

  PoolArena *curr = arena, *next = NULL;

  while (curr) {
    next = curr->next;
    free(curr->mem);
    free(curr);
    curr = next;
  }

  return SUCCESS;
}

void *mem_PoolArenaAlloc(PoolArena *arena) {
  if (!arena) {
    printf("Can not allocate from an invalid pool arena.\n");
    return NULL;
  }

  PoolArena *curr = arena, *next = NULL;

  while (curr) {
    next = curr->next;

    void *ptr = curr->free_list;
    if (ptr) {
      curr->free_list = *(void **)ptr;
      return ptr;
    }

    curr = next;
  }

  if (AddPoolLink(&arena, arena->block_size) != SUCCESS) {
    printf("Can not allocate memory from pool arena, memory exhausted.\n");
    return NULL;
  }
  /*
   * In the next recursion it will for sure allocate, and max recursion depth is
   * 2 always.
   * Recursing to remove another refactor if the add pool link is changed ever.
   */
  return mem_PoolArenaAlloc(arena);
}

StatusCode mem_PoolArenaFree(PoolArena *arena, void *entry) {
  if (!arena || !entry) {
    printf("Invalid arguments for mem_PoolArenaFree function.\n");
    return FAILURE;
  }

  u64 offset = (u8 *)entry - (u8 *)arena->mem;
  if (offset % arena->block_size != 0 ||
      offset >= arena->block_size * STD_POOL_SIZE) {
    printf("Invalid data pointer provided to dealloc.\n");
    return FAILURE;
  }

  *(void **)entry = arena->free_list;
  arena->free_list = entry;

  return SUCCESS;
}

StatusCode mem_PoolArenaReset(PoolArena *arena) {
  if (!arena) {
    printf("Can not reset an invalid pool arena.\n");
    return FAILURE;
  }

  memset(arena->mem, 0, arena->block_size * arena->cap);

  arena->free_list = arena->mem;
  u8 *curr = (u8 *)arena->mem;
  for (u64 i = 0; i < arena->cap - 1; i++) {
    *(void **)curr = curr + arena->block_size;
    curr += arena->block_size;
  }
  *(void **)curr = NULL; // Last block points to NULL

  return SUCCESS;
}
