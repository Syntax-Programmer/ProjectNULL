#include "mem.h"

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
  if (!arena) {
  printf("Unreachable condition hit in function %s. Argument 'arena' is NULL.\n", __func__);
  return FAILURE;
  }

  MemBlock *block = malloc(sizeof(MemBlock));
  if (!block) {
    printf("Can not add more memory to pool arena, memory alloc failure.\n");
    return FAILURE;
  }

  block->mem = malloc(arena->block_size * STD_POOL_SIZE);
  if (!(block->mem)) {
    free(block);
    printf("Can not add more memory to pool arena, memory alloc failure.\n");
    return FAILURE;
  }

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
  if (!arena) {
    printf("Can not create the pool arena with the size: %zu", block_size);
    return NULL;
  }

  arena->block_size =
      (block_size < sizeof(void *)) ? sizeof(void *) : block_size;
  if (AddPoolMem(arena) != SUCCESS) {
    printf("Can not create the pool arena with the size: %zu", block_size);
    return NULL;
  };

  return arena;
}

StatusCode mem_PoolArenaDelete(PoolArena *arena) {
  if (!arena) {
    printf("Can not delete an invalid pool arena.\n");
    return FAILURE;
  }

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
  if (!arena) {
    printf("Can not allocate from an invalid pool arena.\n");
    return NULL;
  }

  void *ptr = NULL;

  if (arena->free_list) {
    ptr = arena->free_list;
    arena->free_list = *(void **)arena->free_list;
    return ptr;
  }
  if (AddPoolMem(arena) != SUCCESS) {
    printf("Can not allocate memory from pool arena, memory exhausted.\n");
    return NULL;
  }

  ptr = arena->free_list;
  arena->free_list = *(void **)arena->free_list;

  return ptr;
}

StatusCode mem_PoolArenaFree(PoolArena *arena, void *entry) {
  if (!arena || !entry) {
    printf("Invalid arguments for mem_PoolArenaFree function.\n");
    return FAILURE;
  }

  // u64 offset = (u8 *)entry - (u8 *)arena->mem;
  // if (offset % arena->block_size != 0 ||
  //     offset >= arena->block_size * STD_POOL_SIZE) {
  //   printf("Invalid data pointer provided to dealloc.\n");
  //   return FAILURE;
  // }
#ifdef DEBUG
  fprintf(
      stderr,
      "mem_PoolArenaFree, is a unsafe function that currently doesn't check "
      "if the 'to-free' pointer is a part of the pool. Use it in absolutely "
      "trusted envirnoments.\n");
#endif
  /*
   * Currently there is no safegaurd in freeing. The developer believes that
   * this function will be used through abstractions above it, and not
   * revealed to the end user, its a compromise that can be changed if the
   * situation ever changes.
   */
  *(void **)entry = arena->free_list;
  arena->free_list = entry;

  return SUCCESS;
}

StatusCode mem_PoolArenaReset(PoolArena *arena) {
  if (!arena) {
    printf("Can not reset an invalid pool arena.\n");
    return FAILURE;
  }

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
