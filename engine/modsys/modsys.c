#include "modsys.h"
#include "../types/hm.h"
#include "../utils/mem.h"

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

/* ----  FUNCTIONS   ---- */

static StatusCode PopulateBuiltinPropsMetadata(void);

static StatusCode ModSysChunkAdd(ModSysTmpl *tmpl);
static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl);

static StatusCode AddECSTmpl(ModSysProps props);
static StatusCode FreeEcsTmplCallback(void *tmpl);
static StatusCode DeleteECSTmpl(ModSysTmpl *tmpl);

static StatusCode PopulateBuiltinPropsMetadata(void) {
  if (!ecs_state) {
    printf("Invalid function call to %s, before initializing the ecs_state.\n",
           __func__);
    return FAILURE;
  }

  /*
   * TODO:
   * builtin_props_metadata.size[PROP1] = sizeof(Prop1)
   * builtin_props_metadata.is_serializable[PROP1] = T/F.
   * ...
   */

  return SUCCESS;
}

static StatusCode ModSysChunkAdd(ModSysTmpl *tmpl) {
  if (!ecs_state) {
    printf("Invalid function call to %s, before initializing the ecs_state.\n",
           __func__);
    return FAILURE;
  }
  if (!tmpl) {
    printf("NULL argument 'tmpl' passed to %s\n", __func__);
    return FAILURE;
  }

  ModSysChunk *chunk = mem_PoolArenaAlloc(ecs_state->chunk_alloc_arena);
  if (!chunk) {
    printf(
        "Can not allocate chunk for template with props: %ld, memory failure",
        tmpl->props);
    return FAILURE;
  }
  chunk->data = malloc(tmpl->props_struct_size * PROP_ARR_CAP);
  if (!(chunk->data)) {
    mem_PoolArenaFree(ecs_state->chunk_alloc_arena, chunk);
    printf(
        "Can not allocate chunk for template with props: %ld, memory failure",
        tmpl->props);
    return FAILURE;
  }
  chunk->len = 0;

  chunk->next = tmpl->chunk;
  tmpl->chunk = chunk;

  return SUCCESS;
}

static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl) {
  if (!ecs_state) {
    printf("Invalid function call to %s, before initializing the ecs_state.\n",
           __func__);
    return FAILURE;
  }
  if (!tmpl) {
    printf("NULL argument 'tmpl' passed to %s\n", __func__);
    return FAILURE;
  }

  ModSysChunk *curr = tmpl->chunk, *next = NULL;
  while (curr) {
    next = curr->next;

    free(curr->data);
    mem_PoolArenaFree(ecs_state->chunk_alloc_arena, curr);

    curr = next;
  }

  return SUCCESS;
}

static StatusCode AddECSTmpl(ModSysProps props) {
  if (!ecs_state) {
    printf("Invalid function call to %s, before initializing the ecs_state.\n",
           __func__);
    return FAILURE;
  }

  if (props == 0) {
    printf("Can not add a tmpl with no/invalid props.\n");
    return FAILURE;
  }

  if (hm_IntKeyFetchEntry(ecs_state->ecs, props)) {
    // The tmpl already exists.
    return SUCCESS;
  }

  ModSysTmpl *tmpl = mem_PoolArenaAlloc(ecs_state->tmpl_alloc_arena);
  if (!tmpl) {
    printf("Can not create new tmpl with props: %ld, memory failure.\n", props);
    return FAILURE;
  }
  tmpl->props = props;
  tmpl->props_struct_size = 0;
  /*
   * TODO: Make the size when we have some props defined.
   */
  if (ModSysChunkAdd(tmpl) != SUCCESS) {
    mem_PoolArenaFree(ecs_state->tmpl_alloc_arena, tmpl);
    printf("Can not create initial memory chunk for the tmpl with props: "
           "%ld.\n",
           props);
    return FAILURE;
  }

  hm_IntKeyAddEntry(ecs_state->ecs, props, tmpl, false);

  return SUCCESS;
}

static StatusCode FreeEcsTmplCallback(void *tmpl) {
  if (!tmpl) {
    // Null pointers are still valid hashmap values, so not denying it.
    return SUCCESS;
  }

  ModSysTmpl *tmpl2 = tmpl;
  if (!hm_IntKeyFetchEntry(ecs_state->ecs, tmpl2->props)) {
    printf("Invalid tmpl provided to free that doesn't exist in ecs. Critical "
           "error.\n");
    return FAILURE;
  }

  ModSysChunkDelete(tmpl2);
  mem_PoolArenaFree(ecs_state->tmpl_alloc_arena, tmpl2);

  return SUCCESS;
}

static StatusCode DeleteECSTmpl(ModSysTmpl *tmpl) {
  return hm_IntKeyDeleteEntry(ecs_state->ecs, tmpl->props, FreeEcsTmplCallback);
}

StatusCode modsys_Init(void) {
  // Calloc to give proper cleanup if failed to initialize.
  ecs_state = calloc(1, sizeof(ECSState));
  if (!ecs_state) {
    printf("Can not initialize ecs_state for the engine, memory failure.\n");
    return FAILURE;
  }

  ecs_state->chunk_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysChunk));
  ecs_state->tmpl_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysTmpl));
  ecs_state->handle_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysHandle));
  if (!(ecs_state->chunk_alloc_arena) || !(ecs_state->tmpl_alloc_arena) ||
      !(ecs_state->handle_alloc_arena)) {
    printf("Can not initialize memory pools for the ECS.\n");
    modsys_Exit();
    return FAILURE;
  }

  ecs_state->ecs = hm_IntKeyCreate();
  if (!ecs_state->ecs) {
    printf("Can not initialize ECS for the engine.\n");
    modsys_Exit();
    return FAILURE;
  }

  ecs_state->builtin_props_metadata = malloc(sizeof(PropsMetadata));
  if (!(ecs_state->builtin_props_metadata)) {
    printf("Can not initialize the props metadata for the engine, memory "
           "failure.\n");
    modsys_Exit();
    return FAILURE;
  }
  PopulateBuiltinPropsMetadata();

  ecs_state->entity_buffer = malloc(sizeof(EntityDefineBuffer));
  if (!(ecs_state->entity_buffer)) {
    printf("Can not initialize the entity buffer for the engine, memory "
           "failure.\n");
    modsys_Exit();
    return FAILURE;
  }
  ecs_state->tmpl_buffer = malloc(sizeof(TmplDefineBuffer));
  if (!(ecs_state->tmpl_buffer)) {
    printf("Can not initialize the tmpl buffer for the engine, memory "
           "failure.\n");
    modsys_Exit();
    return FAILURE;
  }

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
