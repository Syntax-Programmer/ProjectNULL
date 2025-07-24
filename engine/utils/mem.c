#include "mem.h"
#include "common.h"

/* ----  BUMP ARENA  ---- */

struct __BumpArena {
  void *mem;
  u64 size;
  u64 offset;
};

BumpArena *mem_BumpArenaCreate(u64 size) {
  BumpArena *arena = malloc(sizeof(BumpArena));
  CHECK_ALLOC_FAILURE(arena, NULL);

  arena->mem = malloc(size);
  CHECK_ALLOC_FAILURE(arena->mem, NULL, mem_BumpArenaDelete(arena));

  arena->offset = 0;
  arena->size = size;

  return arena;
}

StatusCode mem_BumpArenaDelete(BumpArena *arena) {
  CHECK_NULL_ARG(arena, FAILURE);

  free(arena->mem);
  free(arena);

  return SUCCESS;
}

void *mem_BumpArenaAlloc(BumpArena *arena, u64 size) {
  CHECK_NULL_ARG(arena, NULL);

  if (arena->offset + size > arena->size) {
    printf("Bump arena has: %zu, while allocation attempt of: %zu.\n",
           arena->size - arena->offset, size);
    return NULL;
  }

  void *ptr = MEM_OFFSET(arena->mem, arena->offset);
  arena->offset += size;

  return ptr;
}

void *mem_BumpArenaCalloc(BumpArena *arena, u64 size) {
  void *ptr = mem_BumpArenaAlloc(arena, size);
  CHECK_ALLOC_FAILURE(ptr, NULL);

  memset(ptr, 0, size);

  return ptr;
}

StatusCode mem_BumpArenaReset(BumpArena *arena) {
  CHECK_NULL_ARG(arena, FAILURE);

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
  CHECK_NULL_ARG(arena, FAILURE);

  MemBlock *block = malloc(sizeof(MemBlock));
  CHECK_ALLOC_FAILURE(block, FAILURE);

  block->mem = malloc(arena->block_size * STD_POOL_SIZE);
  CHECK_ALLOC_FAILURE(block->mem, FAILURE, free(block));

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
  PoolArena *arena = malloc(sizeof(PoolArena));
  CHECK_ALLOC_FAILURE(arena, NULL);

  arena->block_size =
      (block_size < sizeof(void *)) ? sizeof(void *) : block_size;
  CHECK_FUNCTION_FAILURE(
      AddPoolMem(arena), NULL,
      printf("Cannot create initial memory of the pool arena with size: %zu.\n",
             block_size);
      free(arena));

  return arena;
}

StatusCode mem_PoolArenaDelete(PoolArena *arena) {
  CHECK_NULL_ARG(arena, FAILURE);

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
  CHECK_NULL_ARG(arena, NULL);

  void *ptr = NULL;

  if (arena->free_list) {
    ptr = arena->free_list;
    arena->free_list = *(void **)arena->free_list;
    return ptr;
  }
  CHECK_FUNCTION_FAILURE(
      AddPoolMem(arena), NULL,
      printf("Cannot allocate memory from pool, memory exhausted.\n");
      free(arena));

  ptr = arena->free_list;
  arena->free_list = *(void **)arena->free_list;

  return ptr;
}

void *mem_PoolArenaCalloc(PoolArena *arena) {
  void *ptr = mem_PoolArenaAlloc(arena);
  CHECK_ALLOC_FAILURE(ptr, NULL);

  memset(ptr, 0, arena->block_size);

  return ptr;
}

StatusCode mem_PoolArenaFree(PoolArena *arena, void *entry) {
  CHECK_NULL_ARG(arena, FAILURE);
  CHECK_NULL_ARG(entry, FAILURE);

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

  printf("'entry' ptr is not a valid pointer inside the provided pool arena "
         "in: %s.\n",
         __func__);

  return FAILURE;
}

StatusCode mem_PoolArenaReset(PoolArena *arena) {
  CHECK_NULL_ARG(arena, FAILURE);

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
