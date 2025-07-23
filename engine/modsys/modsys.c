#include "modsys.h"
#include "../types/hm.h"
#include "../utils/mem.h"

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

#define PROP_ARR_CAP (24)

/* ----  INTERNAL STRUCTS  ---- */

typedef struct {
  u64 size[MODSYS_PROPS_COUNT];
  bool is_serializable[MODSYS_PROPS_COUNT];
} PropsMetadata;

typedef struct __ModSysChunk {
  void *data;
  u64 len;
  struct __ModSysChunk *next;
} ModSysChunk;

/* ----  USER FACING STRUCTS  ---- */

struct __ModSysTmpl {
  ModSysChunk *chunk;
  // Size of one of each prop's struct for quick chunk creation.
  u64 props_struct_size;
  ModSysProps props;
};

struct __ModSysHandle {
  ModSysChunk *chunk;
  /*
   * Not storing the props, but the tmpl itself. This is to boost
   * performance since we don't have to re fetch from hashmap each time.
   */
  ModSysTmpl *tmpl;
  u64 entry_index;
};

/* ----  BUFFERS  ---- */

typedef struct {
  /*
   * Prop1 prop1;
   * ...
   */
  ModSysProps props;
  bool in_use;
} EntityDefineBuffer;

typedef struct {
  ModSysProps props;
  bool in_use;
} TmplDefineBuffer;

/* ----  INTERANL STATE  ---- */

typedef struct {
  /*
   * Since all three ModSysChunk, ModSysTeml and ModSysHandle are 24 bytes fixed
   * size, and require frequent creation and deletion, it is natural for this
   * engine to prefer them to be allocated in pools to avoid that constant
   * malloc.
   *
   * chunk_alloc_arena, and tmpl_alloc_arena are also special as they are
   * values of the ecs hashmap. The Hm_IntKey implementation needs a
   * **val_delete_callback**. Will now also provide NULL to it(and let Hm
   * scream logs at us), which means the Hm won't attempt to free the values. We
   * can do it ourselves by purging the pool arenas in one go.
   */
  PoolArena *chunk_alloc_arena;
  PoolArena *tmpl_alloc_arena;
  PoolArena *handle_alloc_arena;
  /*
   * This represents the entire ECS, where the key is ModSysProps, and the
   * values are ModSysTmpl. A hashmap for O(1) access and manipulation made for
   * performant entity creation.
   */
  Hm_IntKey *ecs;
  /*
   * Defines the metadata for each component, mapping the props enum to its
   * frequently used props. This is to reduce the time that it may take
   * determining metadata from props.
   */
  PropsMetadata *builtin_props_metadata;
  /*
   * Entity defs are written here before copying it into the actual ecs. This
   * reduces the number of definitions reallocation and search time.
   * This also significantly simplifies the amount of memory handling.
   */
  EntityDefineBuffer *entity_buffer;
  /*
   * Tmpl defs are written here before creating it. This is to allow the user
   * to add props whenever they want, before locking it to lock it into the ecs.
   */
  TmplDefineBuffer *tmpl_buffer;
} ECSState;

static ECSState *ecs_state = NULL;

/* ----  UTILITY FUNCTIONS   ---- */

static u64 PropTableIndex(u64 x);
static StatusCode PopulateBuiltinPropsMetadata(void);

/* ----  CHUNK RELATED FUNCTIONS  ---- */

static StatusCode ModSysChunkAdd(ModSysTmpl *tmpl);
static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl);
static StatusCode ModSysChunkFindFreeSpot(ModSysTmpl *tmpl,
                                          ModSysHandle *handle);
static StatusCode ModSysChunkReclaimFreeSpot(ModSysHandle *handle);

/* ----  TEMPLATE RELATED FUNCTIONS  ---- */

static StatusCode ModSysTmplAdd(ModSysProps props);
static StatusCode ModSysTmplFreeCallback(void *tmpl);
static StatusCode ModSysTmplDelete(ModSysTmpl *tmpl);

/* ----  HANDLE RELATED FUNCTIONS  ---- */

static ModSysHandle *ModSysHandleCreate(ModSysTmpl *tmpl);
static StatusCode ModSysHandleDelete(ModSysHandle *handle);

/* ----  UTILITY FUNCTIONS   ---- */

static u64 PropTableIndex(ModSysProps prop) {
  if (prop == 0)
    return -1; // Undefined for zero

#if defined(__GNUC__) || defined(__clang__)
  return __builtin_ctzll(prop);

#elif defined(_MSC_VER)
  unsigned long index;
  _BitScanForward64(&index, prop);
  return (u64)index;

#else
  // Portable fallback
  u64 n = 0;
  while ((prop & 1) == 0) {
    prop >>= 1;
    n++;
  }
  return n;
#endif
}

static StatusCode PopulateBuiltinPropsMetadata(void) {
  CHECK_NULL_VAR(ecs_state, FAILURE);
  /*
   * TODO:
   * builtin_props_metadata.size[PROP1] = sizeof(Prop1)
   * builtin_props_metadata.is_serializable[PROP1] = T/F.
   * ...
   */

  return SUCCESS;
}

/* ----  CHUNK RELATED FUNCTIONS  ---- */

static StatusCode ModSysChunkAdd(ModSysTmpl *tmpl) {
  CHECK_NULL_VAR(ecs_state, FAILURE);
  CHECK_NULL_ARG(tmpl, FAILURE);

  ModSysChunk *chunk = mem_PoolArenaAlloc(ecs_state->chunk_alloc_arena);
  CHECK_ALLOC_FAILURE(chunk, FAILURE);

  chunk->data = malloc(tmpl->props_struct_size * PROP_ARR_CAP);
  CHECK_ALLOC_FAILURE(chunk, FAILURE,
                      mem_PoolArenaFree(ecs_state->chunk_alloc_arena, chunk));

  chunk->len = 0;

  chunk->next = tmpl->chunk;
  tmpl->chunk = chunk;

  return SUCCESS;
}

static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl) {
  CHECK_NULL_VAR(ecs_state, FAILURE);
  CHECK_NULL_ARG(tmpl, FAILURE);

  ModSysChunk *curr = tmpl->chunk, *next = NULL;
  while (curr) {
    next = curr->next;

    free(curr->data);
    mem_PoolArenaFree(ecs_state->chunk_alloc_arena, curr);

    curr = next;
  }

  return SUCCESS;
}

static StatusCode ModSysChunkFindFreeSpot(ModSysTmpl *tmpl,
                                          ModSysHandle *handle) {
  CHECK_NULL_VAR(ecs_state, FAILURE);
  CHECK_NULL_ARG(tmpl, FAILURE);
  CHECK_NULL_ARG(handle, FAILURE);

  ModSysChunk *curr = tmpl->chunk;
  while (curr) {
    if (curr->len != PROP_ARR_CAP) {
      handle->chunk = curr;
      handle->entry_index = curr->len++;
      return SUCCESS;
    }
    curr = curr->next;
  }

  CHECK_FUNCTION_FAILURE(ModSysChunkAdd(tmpl), FAILURE,
                         printf("Unable to find valid free spot in tmpl"));

  handle->chunk = tmpl->chunk;
  handle->entry_index = tmpl->chunk->len++;

  return SUCCESS;
}

static StatusCode ModSysChunkReclaimFreeSpot(ModSysHandle *handle) {
  CHECK_NULL_VAR(ecs_state, FAILURE);
  CHECK_NULL_ARG(handle, FAILURE);

  /*
   * Guarantee: If a handle is created through valid means and not some idiot
   * way, it will defininetly have at least len == 1.
   */

  u64 last_index = --(handle->chunk->len);
  ModSysProps props = handle->tmpl->props;
  // Tracks the offset of the current arr we are packing.
  u64 arr_offset = 0;
  while (props) {
    ModSysProps prop = props & -props;

    /*
     * Now it is important that lowest bit props always come before in the chunk
     * memory for this to work.
     */
    u64 index = PropTableIndex(prop);
    u64 prop_size = ecs_state->builtin_props_metadata->size[index];
    void *chunk_mem = handle->chunk->data;

    void *arr_last = chunk_mem + arr_offset + prop_size * last_index;
    void *arr_to_free =
        chunk_mem + arr_offset + prop_size * handle->entry_index;

    // Repacking each individual array.
    memmove(arr_to_free, arr_last, prop_size);

    arr_offset += prop_size;

    props ^= prop;
  }

  return SUCCESS;
}

/* ----  TEMPLATE RELATED FUNCTIONS  ---- */

static StatusCode ModSysTmplAdd(ModSysProps props) {
  CHECK_NULL_VAR(ecs_state, FAILURE);

  if (props == 0) {
    printf("Can not add a tmpl with no/invalid props.\n");
    return FAILURE;
  }

  if (hm_IntKeyFetchEntry(ecs_state->ecs, props) != NULL) {
    // The tmpl already exists.
    return SUCCESS;
  }

  ModSysTmpl *tmpl = mem_PoolArenaAlloc(ecs_state->tmpl_alloc_arena);
  CHECK_ALLOC_FAILURE(tmpl, FAILURE);

  tmpl->props = props;
  tmpl->props_struct_size = 0;
  /*
   * TODO: Make the size when we have some props defined.
   */
  CHECK_FUNCTION_FAILURE(ModSysChunkAdd(tmpl), FAILURE,
                         mem_PoolArenaFree(ecs_state->tmpl_alloc_arena, tmpl));

  hm_IntKeyAddEntry(ecs_state->ecs, props, tmpl, false);

  return SUCCESS;
}

static StatusCode ModSysTmplFreeCallback(void *tmpl) {
  CHECK_NULL_VAR(ecs_state, FAILURE);
  CHECK_NULL_ARG(tmpl, FAILURE);

  ModSysTmpl *tmpl2 = tmpl;
  if (!hm_IntKeyFetchEntry(ecs_state->ecs, tmpl2->props)) {
    printf("Invalid tmpl provided to free that doesn't exist in ecs.\n");
    return FAILURE;
  }

  ModSysChunkDelete(tmpl2);
  mem_PoolArenaFree(ecs_state->tmpl_alloc_arena, tmpl2);

  return SUCCESS;
}

static StatusCode ModSysTmplDelete(ModSysTmpl *tmpl) {
  return hm_IntKeyDeleteEntry(ecs_state->ecs, tmpl->props,
                              ModSysTmplFreeCallback);
}

/* ----  HANDLE RELATED FUNCTIONS  ---- */

static ModSysHandle *ModSysHandleCreate(ModSysTmpl *tmpl) {
  CHECK_NULL_VAR(ecs_state, NULL);
  CHECK_NULL_ARG(tmpl, NULL);

  ModSysHandle *handle = mem_PoolArenaAlloc(ecs_state->handle_alloc_arena);
  CHECK_ALLOC_FAILURE(handle, NULL);

  handle->tmpl = tmpl;
  CHECK_FUNCTION_FAILURE(
      ModSysChunkFindFreeSpot(tmpl, handle), NULL,
      mem_PoolArenaFree(ecs_state->handle_alloc_arena, handle));

  return handle;
}

static StatusCode ModSysHandleDelete(ModSysHandle *handle) {
  CHECK_NULL_VAR(ecs_state, FAILURE);
  CHECK_NULL_ARG(handle, FAILURE);

  ModSysChunkReclaimFreeSpot(handle);
  mem_PoolArenaFree(ecs_state->handle_alloc_arena, handle);

  return SUCCESS;
}

StatusCode modsys_Init(void) {
  // Calloc to give proper cleanup if failed to initialize.
  ecs_state = calloc(1, sizeof(ECSState));
  CHECK_ALLOC_FAILURE(ecs_state, FAILURE);

  ecs_state->chunk_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysChunk));
  CHECK_ALLOC_FAILURE(ecs_state->chunk_alloc_arena, FAILURE, modsys_Exit());

  ecs_state->tmpl_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysTmpl));
  CHECK_ALLOC_FAILURE(ecs_state->tmpl_alloc_arena, FAILURE, modsys_Exit());

  ecs_state->handle_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysHandle));
  CHECK_ALLOC_FAILURE(ecs_state->handle_alloc_arena, FAILURE, modsys_Exit());

  ecs_state->ecs = hm_IntKeyCreate();
  CHECK_ALLOC_FAILURE(ecs_state->ecs, FAILURE, modsys_Exit());

  ecs_state->builtin_props_metadata = malloc(sizeof(PropsMetadata));
  CHECK_ALLOC_FAILURE(ecs_state->builtin_props_metadata, FAILURE,
                      modsys_Exit());
  PopulateBuiltinPropsMetadata();

  ecs_state->entity_buffer = malloc(sizeof(EntityDefineBuffer));
  CHECK_ALLOC_FAILURE(ecs_state->entity_buffer, FAILURE, modsys_Exit());

  ecs_state->tmpl_buffer = malloc(sizeof(TmplDefineBuffer));
  CHECK_ALLOC_FAILURE(ecs_state->tmpl_buffer, FAILURE, modsys_Exit());

  return SUCCESS;
}

StatusCode modsys_Exit(void) {
  if (!ecs_state) {
    return SUCCESS;
  }
  if (ecs_state->chunk_alloc_arena) {
    mem_PoolArenaDelete(ecs_state->chunk_alloc_arena);
  }
  if (ecs_state->tmpl_alloc_arena) {
    mem_PoolArenaDelete(ecs_state->tmpl_alloc_arena);
  }
  if (ecs_state->handle_alloc_arena) {
    mem_PoolArenaDelete(ecs_state->handle_alloc_arena);
  }
  if (ecs_state->ecs) {
    // We don't pass val delete callback, as we purged their pools.
    hm_IntKeyDelete(ecs_state->ecs, NULL);
  }
  if (ecs_state->builtin_props_metadata) {
    free(ecs_state->builtin_props_metadata);
  }
  if (ecs_state->entity_buffer) {
    free(ecs_state->entity_buffer);
  }
  if (ecs_state->tmpl_buffer) {
    free(ecs_state->tmpl_buffer);
  }
  free(ecs_state);
  ecs_state = NULL;

  return SUCCESS;
}

/*
 * TODO:
 * Now it is important that lowest bit props always come before in the chunk
 * memory for the ModSysChunkReclaimFreeSpot to work.
 */
