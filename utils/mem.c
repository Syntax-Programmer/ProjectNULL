#include "mem.h"
#include "status.h"

/* ----  BUMP ARENA  ---- */

struct __BumpArena {
  void *mem;
  u64 size;
  u64 offset;
};

BumpArena *mem_BumpArenaCreate(u64 size) {
  BumpArena *arena = malloc(sizeof(BumpArena));
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(arena, NULL);

  arena->mem = malloc(size);
  IF_NULL(arena->mem) {
    mem_BumpArenaDelete(arena);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(arena->mem, NULL);
  }

  arena->offset = 0;
  arena->size = size;

  return arena;
}

StatusCode mem_BumpArenaDelete(BumpArena *arena) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL_EXCEPTION);

  free(arena->mem);
  free(arena);

  return SUCCESS;
}

void *mem_BumpArenaAlloc(BumpArena *arena, u64 size) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL);

  if (arena->offset + size > arena->size) {
    STATUS_LOG(FAILURE,
               "Bump arena has: %zu, while allocation attempt of: %zu.",
               arena->size - arena->offset, size);
    return NULL;
  }

  void *ptr = MEM_OFFSET(arena->mem, arena->offset);
  arena->offset += size;

  return ptr;
}

void *mem_BumpArenaCalloc(BumpArena *arena, u64 size) {
  void *ptr = mem_BumpArenaAlloc(arena, size);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(ptr, NULL);

  memset(ptr, 0, size);

  return ptr;
}

StatusCode mem_BumpArenaReset(BumpArena *arena) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL_EXCEPTION);

  memset(arena->mem, 0, arena->size);
  arena->offset = 0;

  return SUCCESS;
}

/* ----  POOL ARENA  ---- */

#define STD_POOL_SIZE (24)

typedef struct __MemBlock {
  void *mem;
  /*
   * Using linked memory blocks rather than reallocing, because reallocing the
   * memory will render the original free list structure invalid with no way to
   * replicate it to new mem block.
   */
  struct __MemBlock *next;
} MemBlock;

struct __PoolArena {
  MemBlock *mem_blocks;
  // A singular free list tracks everything, for true O(1) alloc and dealloc.
  void *free_list;
  u64 block_size;
};

static StatusCode AddPoolMem(PoolArena *arena);

static StatusCode AddPoolMem(PoolArena *arena) {
  MemBlock *block = malloc(sizeof(MemBlock));
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(block, CREATION_FAILURE);

  block->mem = malloc(arena->block_size * STD_POOL_SIZE);
  IF_NULL(block->mem) {
    free(block);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(block->mem, CREATION_FAILURE);
  }

  // Adding the new block to the free list.
  u8 *curr = block->mem;
  for (u64 i = 0; i < STD_POOL_SIZE - 1; i++) {
    *(void **)curr = curr + arena->block_size;
    curr += arena->block_size;
  }
  *(void **)curr = arena->free_list;
  arena->free_list = block->mem;

  block->next = arena->mem_blocks;
  arena->mem_blocks = block;

  return SUCCESS;
}

PoolArena *mem_PoolArenaCreate(u64 block_size) {
  PoolArena *arena = calloc(1, sizeof(PoolArena));
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(arena, NULL);

  arena->block_size =
      (block_size < sizeof(void *)) ? sizeof(void *) : block_size;
  IF_FUNC_FAILED(AddPoolMem(arena)) {
    free(arena);
    STATUS_LOG(CREATION_FAILURE,
               "Cannot create initial memory of the pool arena with size: %zu.",
               block_size);
    return NULL;
  }

  return arena;
}

StatusCode mem_PoolArenaDelete(PoolArena *arena) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL_EXCEPTION);

  MemBlock *curr = arena->mem_blocks, *next = NULL;
  while (curr) {
    next = curr->next;
    free(curr->mem);
    free(curr);
    curr = next;
  }

  free(arena);

  return SUCCESS;
}

void *mem_PoolArenaAlloc(PoolArena *arena) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL);

  void *ptr = NULL;

  if (arena->free_list) {
    ptr = arena->free_list;
    arena->free_list = *(void **)arena->free_list;
    return ptr;
  }
  IF_FUNC_FAILED(AddPoolMem(arena)) {
    STATUS_LOG(FAILURE,
               "Cannot find appropriate memory from pool to allocate.");
    return NULL;
  }

  ptr = arena->free_list;
  arena->free_list = *(void **)arena->free_list;

  return ptr;
}

void *mem_PoolArenaCalloc(PoolArena *arena) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL);

  void *ptr = mem_PoolArenaAlloc(arena);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(ptr, NULL);

  memset(ptr, 0, arena->block_size);

  return ptr;
}

StatusCode mem_PoolArenaFree(PoolArena *arena, void *entry) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(entry, NULL_EXCEPTION);

  MemBlock *curr = arena->mem_blocks;
  while (curr) {
    u64 offset = (u8 *)entry - (u8 *)curr->mem;
    if (offset % arena->block_size == 0 &&
        offset < arena->block_size * STD_POOL_SIZE) {
      // Valid ptr of the pool arena.
      *(void **)entry = arena->free_list;
      arena->free_list = entry;
      return SUCCESS;
    }

    curr = curr->next;
  }

  STATUS_LOG(FAILURE,
             "'entry' is not a valid memory of the pool arena to free.");

  return FAILURE;
}

StatusCode mem_PoolArenaReset(PoolArena *arena) {
  NULL_FUNC_ARG_ROUTINE(arena, NULL_EXCEPTION);

  MemBlock *curr = arena->mem_blocks;

  while (curr) {
    memset(curr->mem, 0, arena->block_size * STD_POOL_SIZE);

    u8 *curr2 = curr->mem;
    for (u64 i = 0; i < STD_POOL_SIZE - 1; i++) {
      *(void **)curr2 = curr2 + arena->block_size;
      curr2 += arena->block_size;
    }
    *(void **)curr2 = arena->free_list;
    arena->free_list = curr->mem;

    curr = curr->next;
  }

  return SUCCESS;
}
