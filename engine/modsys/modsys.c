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

#define ECS_STATE_MISSING_LOG                                                  \
  ("Modsys functions called without initializing modsys.")
#define CHECK_VALID_ECS_STATE(ret_val)                                         \
  NULL_EXCEPTION_ROUTINE(ecs_state, ret_val, ECS_STATE_MISSING_LOG)

/* ----  UTILITY FUNCTIONS   ---- */

static u64 PropTableIndex(ModSysProps prop);
static StatusCode PopulateBuiltinPropsMetadata(void);

/* ----  CHUNK RELATED FUNCTIONS  ---- */

static StatusCode ModSysChunkAdd(ModSysTmpl *tmpl);
static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl);
static StatusCode ModSysChunkFindFreeSpot(ModSysTmpl *tmpl,
                                          ModSysHandle *handle);
static StatusCode ModSysChunkReclaimFreeSpot(ModSysHandle *handle);

/* ----  TEMPLATE RELATED FUNCTIONS  ---- */

static ModSysTmpl *ModSysTmplAdd(ModSysProps props);
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
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
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
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(tmpl, NULL_EXCEPTION);

  ModSysChunk *chunk = mem_PoolArenaAlloc(ecs_state->chunk_alloc_arena);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(chunk, CREATION_FAILURE);

  chunk->data = malloc(tmpl->props_struct_size * PROP_ARR_CAP);
  IF_NULL(chunk->data) {
    mem_PoolArenaFree(ecs_state->chunk_alloc_arena, chunk);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(chunk->data, CREATION_FAILURE);
  }

  chunk->len = 0;

  chunk->next = tmpl->chunk;
  tmpl->chunk = chunk;

  return SUCCESS;
}

static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(tmpl, NULL_EXCEPTION);

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
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(tmpl, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(handle, NULL_EXCEPTION);

  ModSysChunk *curr = tmpl->chunk;
  while (curr) {
    if (curr->len != PROP_ARR_CAP) {
      handle->chunk = curr;
      handle->entry_index = curr->len++;
      return SUCCESS;
    }
    curr = curr->next;
  }

  IF_FUNC_FAILED(ModSysChunkAdd(tmpl)) {
    STATUS_LOG(FAILURE, "Unable to find valid free spot in tmpl.");
    return FAILURE;
  }

  handle->chunk = tmpl->chunk;
  handle->entry_index = tmpl->chunk->len++;

  return SUCCESS;
}

static StatusCode ModSysChunkReclaimFreeSpot(ModSysHandle *handle) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(handle, NULL_EXCEPTION);

  /*
   * Guarantee: If a handle is created through valid means and not some idiot
   * way, it will defininetly have at least len == 1.
   */

  if (handle->entry_index == PROP_ARR_CAP - 1) {
    --(handle->chunk->len);
    return SUCCESS;
  }

  u64 last_index = --(handle->chunk->len);
  ModSysProps props = handle->tmpl->props;
  // Tracks the offset of the current arr we are packing.
  u64 arr_offset = 0;
  // This lets us decompose prop bitflags into individual props.
  while (props) {
    // Extract the lowest most set bit.
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

    // Repacking each array. Using memcpy, indices of array don't collide.
    memcpy(arr_to_free, arr_last, prop_size);

    arr_offset += prop_size;

    // Clearing the lowest set bit from the props;
    props ^= prop;
  }

  handle->chunk = NULL;
  handle->entry_index = -1;

  return SUCCESS;
}

/* ----  TEMPLATE RELATED FUNCTIONS  ---- */

static ModSysTmpl *ModSysTmplAdd(ModSysProps props) {
  CHECK_VALID_ECS_STATE(NULL);

  if (props == NO_PROP) {
    STATUS_LOG(FAILURE, "Cannot add a tmpl with props = NO_PROP.");
    return NULL;
  }

  ModSysTmpl *tmpl = NULL;
  if ((tmpl = hm_IntKeyFetchEntry(ecs_state->ecs, props)) != NULL) {
    // The tmpl already exists.
    return tmpl;
  }

  tmpl = mem_PoolArenaCalloc(ecs_state->tmpl_alloc_arena);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(tmpl, NULL);

  tmpl->props = props;
  tmpl->props_struct_size = 0;
  // This lets us decompose prop bitflags into individual props.
  while (props) {
    // Extract the lowest most set bit.
    ModSysProps prop = props & -props;

    u64 index = PropTableIndex(prop);
    tmpl->props_struct_size += ecs_state->builtin_props_metadata->size[index];

    // Clearing the lowest set bit from the props;
    props ^= prop;
  }

  IF_FUNC_FAILED(ModSysChunkAdd(tmpl)) {
    mem_PoolArenaFree(ecs_state->tmpl_alloc_arena, tmpl);
    STATUS_LOG(CREATION_FAILURE,
               "Failed to create initial chunk for template.");
    return NULL;
  }

  hm_IntKeyAddEntry(ecs_state->ecs, props, tmpl, false);

  return tmpl;
}

static StatusCode ModSysTmplFreeCallback(void *tmpl) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(tmpl, NULL_EXCEPTION);

  ModSysTmpl *tmpl2 = tmpl;
  if (!hm_IntKeyFetchEntry(ecs_state->ecs, tmpl2->props)) {
    STATUS_LOG(FAILURE,
               "Invalid tmpl provided to free that doesn't exist in ecs.");
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

StatusCode modsys_StartTmplDefinition(void) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);

  if (ecs_state->tmpl_buffer->in_use) {
    STATUS_LOG(FAILURE, "Cannot start template definition, please lock the "
                        "current definition first.");
    return FAILURE;
  }

  ecs_state->tmpl_buffer->in_use = true;
  ecs_state->tmpl_buffer->props = NO_PROP;

  return SUCCESS;
}

StatusCode modsys_CancelTmplDefinition(void) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);

  if (!ecs_state->tmpl_buffer->in_use) {
    STATUS_LOG(FAILURE,
               "Cannot cancel template definition, please start a template "
               "definition first.");
    return FAILURE;
  }

  ecs_state->tmpl_buffer->in_use = false;

  return SUCCESS;
}

StatusCode modsys_AttachPropsToTmpl(ModSysProps props) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);

  if (!ecs_state->tmpl_buffer->in_use) {
    STATUS_LOG(FAILURE, "Cannot add props to template, please start a template "
                        "definition first.");
    return FAILURE;
  }

  SET_FLAG(ecs_state->tmpl_buffer->props, props);

  return SUCCESS;
}

ModSysTmpl *modsys_LockTmplDefinition(void) {
  CHECK_VALID_ECS_STATE(NULL);

  if (!ecs_state->tmpl_buffer->in_use) {
    STATUS_LOG(FAILURE, "Cannot lock tmpl definition, please start a template "
                        "definition first.");
    return NULL;
  }

  ModSysTmpl *tmpl = ModSysTmplAdd(ecs_state->tmpl_buffer->props);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(tmpl, NULL);

  ecs_state->tmpl_buffer->in_use = false;

  return tmpl;
}

StatusCode modsys_DeleteTmplDefinition(ModSysTmpl *tmpl) {
  return ModSysTmplDelete(tmpl);
}

/* ----  HANDLE RELATED FUNCTIONS  ---- */

static ModSysHandle *ModSysHandleCreate(ModSysTmpl *tmpl) {
  CHECK_VALID_ECS_STATE(NULL);
  NULL_FUNC_ARG_ROUTINE(tmpl, NULL);

  ModSysHandle *handle = mem_PoolArenaAlloc(ecs_state->handle_alloc_arena);
  IF_NULL(handle) {
    STATUS_LOG(CREATION_FAILURE, "Can not allocate memory for modsys handle.");
    return NULL;
  }

  handle->tmpl = tmpl;
  IF_FUNC_FAILED(ModSysChunkFindFreeSpot(tmpl, handle)) {
    mem_PoolArenaFree(ecs_state->handle_alloc_arena, handle);
    STATUS_LOG(CREATION_FAILURE,
               "Cannot find free spot in tmpl to make the handle.");
    return NULL;
  }

  return handle;
}

static StatusCode ModSysHandleDelete(ModSysHandle *handle) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(handle, NULL_EXCEPTION);

  ModSysChunkReclaimFreeSpot(handle);
  mem_PoolArenaFree(ecs_state->handle_alloc_arena, handle);

  return SUCCESS;
}

/* ----  INIT/EXIT FUNCTIONS  ---- */

#define INIT_FAILED_ROUTINE(x)                                                 \
  do {                                                                         \
    IF_NULL((x)) {                                                             \
      modsys_Exit();                                                           \
      MEM_ALLOC_FAILURE_SUB_ROUTINE(ecs_state->chunk_alloc_arena,              \
                                    CREATION_FAILURE);                         \
    }                                                                          \
  } while (0)

StatusCode modsys_Init(void) {
  // Calloc to give proper cleanup if failed to initialize.
  ecs_state = calloc(1, sizeof(ECSState));
  INIT_FAILED_ROUTINE(ecs_state);

  ecs_state->chunk_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysChunk));
  INIT_FAILED_ROUTINE(ecs_state->chunk_alloc_arena);

  ecs_state->tmpl_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysTmpl));
  INIT_FAILED_ROUTINE(ecs_state->tmpl_alloc_arena);

  ecs_state->handle_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysHandle));
  INIT_FAILED_ROUTINE(ecs_state->handle_alloc_arena);

  ecs_state->ecs = hm_IntKeyCreate();
  INIT_FAILED_ROUTINE(ecs_state->ecs);

  ecs_state->builtin_props_metadata = calloc(1, sizeof(PropsMetadata));
  INIT_FAILED_ROUTINE(ecs_state->builtin_props_metadata);
  PopulateBuiltinPropsMetadata();

  ecs_state->entity_buffer = calloc(1, sizeof(EntityDefineBuffer));
  INIT_FAILED_ROUTINE(ecs_state->entity_buffer);

  ecs_state->tmpl_buffer = calloc(1, sizeof(TmplDefineBuffer));
  INIT_FAILED_ROUTINE(ecs_state->tmpl_buffer);

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
