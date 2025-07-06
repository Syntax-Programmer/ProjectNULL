
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef uint8_t u8;
#define U8_SIZE (sizeof(u8))
typedef uint16_t u16;
#define U16_SIZE (sizeof(u16))
typedef uint32_t u32;
#define U32_SIZE (sizeof(u32))
typedef uint64_t u64;
#define U64_SIZE (sizeof(u64))

#if defined(_MSC_VER) && !defined(__clang__)
#define PACKED_ENUM __pragma(pack(push, 1)) enum __pragma(pack(pop))
#else
#define PACKED_ENUM enum __attribute__((__packed__))
#endif // defined(_MSC_VER) && !defined(__clang__)

typedef PACKED_ENUM{
    SUCCESS, WARNING, FAILURE, FATAL_ERROR, MEMORY_FAILURE,
} StatusCode;

#define STATUS_LOG(status_code, msg, ...)                                      \
  do {                                                                         \
    switch (status_code) {                                                     \
    case SUCCESS:                                                              \
      printf("SUCCESS: " msg "\n", ##__VA_ARGS__);                             \
      break;                                                                   \
    case WARNING:                                                              \
      printf("WARNING: " msg "\n", ##__VA_ARGS__);                             \
      break;                                                                   \
    case FAILURE:                                                              \
      printf("FAILURE: " msg "\n", ##__VA_ARGS__);                             \
      break;                                                                   \
    case FATAL_ERROR:                                                          \
      printf("FATAL ERROR: " msg "\n", ##__VA_ARGS__);                         \
      break;                                                                   \
    case MEMORY_FAILURE:                                                       \
      printf("MEMORY FAILURE: " msg "\n", ##__VA_ARGS__);                      \
      break;                                                                   \
    default:                                                                   \
      printf("UNKNOWN STATUS: " msg "\n", ##__VA_ARGS__);                      \
      break;                                                                   \
    }                                                                          \
  } while (0)

#ifdef DEBUG
#define DEBUG_STATUS_LOG(status_code, msg, ...)                                \
  STATUS_LOG(status_code, msg, ##__VA_ARGS__)
#else
#define DEBUG_STATUS_LOG(status_code, msg, ...)
#endif

#define GENERIC_LOG(msg, ...) printf(msg "\n", ##__VA_ARGS__)

#ifdef DEBUG
#define DEBUG_GENERIC_LOG(msg, ...) GENERIC_LOG(msg, ##__VA_ARGS__)
#else
#define DEBUG_GENERIC_LOG(msg, ...)
#endif

#define MEM_BYTE_OFFSET(mem, offset) ((u8 *)(mem) + (offset))

#define STD_PAGE_SIZE (25 * 1024)

typedef struct FreeBlocks {
  /*
   * This will store its size in number of blocks, not actual mem capacity.
   *
   * Having this is what reduces alloc/dealloc time from O(n) to O(1).
   *
   * This header is 10 bytes in size => MIN_ALLOC_SIZE is 10 bytes, and due to
   * the page size = 25KB, we have 2650 allocable slots per page.
   */
  u16 blocks;
  struct FreeBlocks *next_free_block;
} FreeBlocks;

#define MIN_ALLOC_SIZE (sizeof(FreeBlocks))
// This is guaranteed to be a whole number = 2650.
#define PAGE_BLOCK_COUNT (STD_PAGE_SIZE / MIN_ALLOC_SIZE)

#define SIZE_TO_BLOCK(size) ((size + MIN_ALLOC_SIZE - 1) / MIN_ALLOC_SIZE)

struct __FLHeader {
  void *mem;
  /*
   * The key to store free block with zero extra memory overhead is to store
   * them in the mem itself like how a pool allocator does it.
   *
   * Now this does mean we have a sizeof(FreeBlocks) min allocable size, which I
   * guess is a reasonable tradeoff for the massive performance gains we will
   * get.
   *
   * With the implementation of min allocable size, we will get O(1) dealloc and
   * almost a O(1) alloc time, in a free list arena.
   *
   * Any mismatched data will be padded to match min allocable size.
   */
  FreeBlocks *free_blocks;
  struct __FLHeader *next;
  /*
   * The arena can allow custom > STD_PAGE_SIZE allocs within itself, to provide
   * ease of use. This just tells the realloc function(if the data is again
   * realloced) to free this current memory, and in place of it allocate a new
   * one.
   */
  bool is_custom;
};

typedef struct __FLHeader FLHeader;
typedef FLHeader *FLArena;

static StatusCode FLHeaderPrepend(FLArena *arena, size_t size);
static void FLHeaderDealloc(FLHeader *header);

static void *FLArenaFindExistingSuitableAllocMem(FLArena *arena,
                                                 FLHeader **pAlloc_header,
                                                 u16 to_alloc_blocks);

static StatusCode FLHeaderPrepend(FLArena *arena, size_t size) {
  assert(arena);

  FLHeader *header = malloc(sizeof(FLHeader));
  if (!header) {
    STATUS_LOG(MEMORY_FAILURE,
               "Couldn't allocate memory for free list header.");
    return MEMORY_FAILURE;
  }

  header->mem = malloc(size);
  if (!header->mem) {
    STATUS_LOG(MEMORY_FAILURE,
               "Couldn't allocate memory for free list header.");
    free(header);
    return MEMORY_FAILURE;
  }

  if (size == STD_PAGE_SIZE) {
    header->free_blocks = header->mem;
    header->free_blocks->blocks = PAGE_BLOCK_COUNT;
    header->free_blocks->next_free_block = NULL;
    header->is_custom = false;
  } else {
    header->free_blocks = NULL;
    header->is_custom = true;
  }

  header->next = *arena;
  *arena = header;

  return SUCCESS;
}

static void FLHeaderDealloc(FLHeader *header) {
  assert(header);

  free(header->mem);
  free(header);
}

FLArena *mem_FLArenaCreate() {
  FLArena *arena = malloc(sizeof(FLArena));
  if (!arena) {
    STATUS_LOG(MEMORY_FAILURE, "Couldn't allocate memory for free list arena.");
    return NULL;
  }

  *arena = NULL;

  if (FLHeaderPrepend(arena, STD_PAGE_SIZE) != SUCCESS) {
    STATUS_LOG(MEMORY_FAILURE, "Couldn't allocate memory for free list arena.");
    free(arena);
    return NULL;
  }

  return arena;
}

StatusCode mem_FLArenaDelete(FLArena *arena) {
  if (!arena || !(*arena)) {
    DEBUG_STATUS_LOG(WARNING, "Invalid free list arena provided to delete.");
    return WARNING;
  }

  FLHeader *curr = *arena, *next = NULL;
  while (curr) {
    next = curr->next;
    FLHeaderDealloc(curr);
    curr = next;
  }
  free(arena);

  return SUCCESS;
}

static void *FLArenaFindExistingSuitableAllocMem(FLArena *arena,
                                                 FLHeader **pAlloc_header,
                                                 u16 to_alloc_blocks) {
  assert(arena && pAlloc_header);

  void *ptr = NULL;

  FLHeader *curr = *arena, *next = NULL;
  while (curr) {
    FreeBlocks *prev_block = NULL, *curr_block = curr->free_blocks;
    while (curr_block) {
      if (curr_block->blocks > to_alloc_blocks) {
        ptr = curr_block;

        if (prev_block) {
          FreeBlocks *new_curr = (FreeBlocks *)MEM_BYTE_OFFSET(
              prev_block->next_free_block, to_alloc_blocks * MIN_ALLOC_SIZE);
          new_curr->blocks = curr_block->blocks - to_alloc_blocks;
          new_curr->next_free_block = curr_block->next_free_block;
          prev_block->next_free_block = new_curr;

        } else {
          FreeBlocks *new_curr = (FreeBlocks *)MEM_BYTE_OFFSET(
              curr->free_blocks, to_alloc_blocks * MIN_ALLOC_SIZE);
          new_curr->blocks = curr->free_blocks->blocks - to_alloc_blocks;
          new_curr->next_free_block = curr->free_blocks->next_free_block;
          curr->free_blocks = new_curr;
        }

        *pAlloc_header = curr;
        return ptr;
      } else if (curr_block->blocks == to_alloc_blocks) {
        ptr = curr_block;

        if (prev_block) {
          prev_block->next_free_block = curr_block->next_free_block;
        } else {
          curr->free_blocks = curr_block->next_free_block;
        }

        *pAlloc_header = curr;
        return ptr;
      }
      prev_block = curr_block;
      curr_block = curr_block->next_free_block;
    }

    curr = curr->next;
  }

  return ptr;
}

void *mem_FLArenaAlloc(FLArena *arena, FLHeader **pAlloc_header, size_t size) {
  if (!arena || !(*arena)) {
    DEBUG_STATUS_LOG(WARNING, "Invalid free list arena provided to delete.");
    return NULL;
  }
  if (!pAlloc_header) {
    DEBUG_STATUS_LOG(WARNING,
                     "Invalid free list alloc header provided for alloc.");
    return NULL;
  }

  if (size > STD_PAGE_SIZE) {
    if (FLHeaderPrepend(arena, size) != SUCCESS) {
      STATUS_LOG(
          FAILURE,
          "Couldn't allocate a memory block of %zu in the free list arena.",
          size);
      return NULL;
    }
    *pAlloc_header = *arena;
    return (*pAlloc_header)->mem;
  }
  if (size == 0) {
    STATUS_LOG(WARNING,
               "Allocation of 0 bytes is prohibited in the free list arena.");
    return NULL;
  }
  u16 to_alloc_blocks = SIZE_TO_BLOCK(size);

  void *ptr = FLArenaFindExistingSuitableAllocMem(arena, pAlloc_header,
                                                  to_alloc_blocks);
  if (ptr) {
    return ptr;
  }

  if (FLHeaderPrepend(arena, STD_PAGE_SIZE) != SUCCESS) {
    STATUS_LOG(
        FAILURE,
        "Couldn't allocate a memory block of %zu in the free list arena.",
        size);
    return NULL;
  }

  /*
   * This will return a valid pointer from the newly allocated page, while
   * handing all the special logic for us, and it has 0 chance of return NULL
   * now.
   */
  return FLArenaFindExistingSuitableAllocMem(arena, pAlloc_header,
                                             to_alloc_blocks);
}

void DumpArena(FLArena *arena) {
  FLHeader *curr = *arena;

  while (curr) {
    printf("Curr: %p\n", curr);
    printf("MEM: %p\n", curr->mem);
    printf("isCustom: %d\n", curr->is_custom);
    printf("NEXT: %p\n", curr->next);

    FreeBlocks *c = curr->free_blocks;
    while (c) {
      printf("Free Block: %p\n", c);
      printf("Size: %d\n", c->blocks);
      printf("Next Block: %p\n", c->next_free_block);
      c = c->next_free_block;
    }

    curr = curr->next;
  }
}

int main() {
  const size_t step = 8;
  const size_t max_alloc = 25 * 1024 * 128;
  const int alloc_count = max_alloc / step;

  // -----------------------------
  // Benchmark: malloc + free
  // -----------------------------
  clock_t start_malloc = clock();

  void **sys_ptrs = malloc(sizeof(void *) * alloc_count);
  for (int i = 0; i < alloc_count; i++) {
    size_t size =
        step * ((i % (max_alloc / step)) + 1); // Sizes: 8, 16, ..., 512
    sys_ptrs[i] = malloc(size);
  }
  for (int i = 0; i < alloc_count; i++) {
    free(sys_ptrs[i]);
  }

  clock_t end_malloc = clock();
  double elapsed_malloc = (double)(end_malloc - start_malloc) / CLOCKS_PER_SEC;
  printf("Malloc/free time:       %.3f seconds\n", elapsed_malloc);
  free(sys_ptrs);

  // -----------------------------
  // Benchmark: mem_FLArenaAlloc
  // -----------------------------
  //
  clock_t start_freelist = clock();

  FLArena *arena = mem_FLArenaCreate();
  if (!arena) {
    fprintf(stderr, "Arena creation failed.\n");
    return 1;
  }

  FLHeader *header = NULL;

  for (int i = 0; i < alloc_count; i++) {
    size_t size = step * ((i % (max_alloc / step)) + 1);
    mem_FLArenaAlloc(arena, &header, size);
  }

  StatusCode status = mem_FLArenaDelete(arena);
  if (status != SUCCESS) {
    fprintf(stderr, "Arena deletion failed.\n");
    return 1;
  }

  clock_t end_freelist = clock();
  double elapsed_freelist =
      (double)(end_freelist - start_freelist) / CLOCKS_PER_SEC;
  printf("Free list arena time:   %.3f seconds\n", elapsed_freelist);

  return 0;
}
