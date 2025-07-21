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

/*
 * This represents the entire ECS, where the key is ModSysProps, and the values
 * are ModSysTmpl.
 * A hashmap for O(1) access and manipulation made for performant entity
 * creation.
 */
static Hm_IntKey *ecs = NULL;
/*
 * Defines the metadata for each component, mapping the props enum to its
 * frequently used props. This is to reduce the time that it may take
 * determining metadata from props.
 */
static PropsMetadata *builtin_props_metadata = NULL;
/*
 * Entity defs are written here before copying it into the actual ecs. This
 * reduces the number of definitions reallocation and search time.
 * This also significantly simplifies the amount of memory handling.
 */
static EntityDefineBuffer *entity_buffer = NULL;
/*
 * Tmpl defs are written here before creating it. This is to allow the user
 * to add props whenever they want, before locking it to lock it into the ecs.
 */
static TmplDefineBuffer *tmpl_buffer = NULL;

/* ----  FUNCTIONS   ---- */

static StatusCode PopulateBuiltinPropsMetadata(void);

static StatusCode ModSysChunkAdd(ModSysTmpl *tmpl);
static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl);

static StatusCode AddECSTmpl(ModSysProps props);
static StatusCode DeleteEcsTmplCallback(void *tmpl);
static StatusCode DeleteEcsTmpl(ModSysTmpl *tmpl);

static StatusCode PopulateBuiltinPropsMetadata(void) {
  assert(builtin_props_metadata);

  /*
   * TODO:
   * builtin_props_metadata.size[PROP1] = sizeof(Prop1)
   * builtin_props_metadata.is_serializable[PROP1] = T/F.
   * ...
   */

  return SUCCESS;
}

static StatusCode ModSysChunkAdd(ModSysTmpl *tmpl) {
  assert(ecs && tmpl);
  /*
   * This method of malloc prevents heap fragmentation, and also allows us to
   * only free ModSysChunk, and the chunk->data memory will be freed itself.
   *
   * Its a variation of struct hack.
   */
  void *ptr =
      malloc(sizeof(ModSysChunk) + tmpl->props_struct_size * PROP_ARR_CAP);
  if (!ptr) {
    printf("Can not create a new modsys chunk, memory failure.\n");
    return FAILURE;
  }

  ModSysChunk *chunk = ptr;
  chunk->data = MEM_OFFSET(ptr, sizeof(ModSysChunk));
  chunk->len = 0;

  chunk->next = tmpl->chunk;
  tmpl->chunk = chunk;

  return SUCCESS;
}

static StatusCode ModSysChunkDelete(ModSysTmpl *tmpl) {
  assert(ecs && tmpl);

  ModSysChunk *curr = tmpl->chunk, *next = NULL;
  while (curr) {
    next = curr->next;

    // Only need to free curr since the struct was created using struct hack.
    free(curr);

    curr = next;
  }

  return SUCCESS;
}

static StatusCode AddECSTmpl(ModSysProps props) {
  assert(ecs);

  if (props == 0) {
    printf("Can not add a tmpl with no/invalid props.\n");
    return FAILURE;
  }

  if (hm_IntKeyFetchEntry(ecs, props)) {
    // The tmpl already exists.
    return SUCCESS;
  }

  ModSysTmpl *tmpl = malloc(sizeof(ModSysTmpl));
  if (!tmpl) {
    printf("Can not create memory for new tmpl with props: %ld.\n", props);
    return FAILURE;
  }
  tmpl->props = props;
  tmpl->props_struct_size = 0;
  /*
   * TODO: Make the size when we have some props defined.
   */
  if (ModSysChunkAdd(tmpl) != SUCCESS) {
    free(tmpl);
    printf("Can not create initial memory chunk for the tmpl with props: "
           "%ld.\n",
           props);
    return FAILURE;
  }

  hm_IntKeyAddEntry(ecs, props, tmpl, false);

  return SUCCESS;
}

static StatusCode DeleteEcsTmplCallback(void *tmpl) {
  assert(ecs && tmpl);

  ModSysTmpl *tmp = tmpl;

  ModSysChunkDelete(tmp);
  free(tmp);

  return SUCCESS;
}

static StatusCode DeleteEcsTmpl(ModSysTmpl *tmpl) {
  assert(ecs);

  return hm_IntKeyDeleteEntry(ecs, tmpl->props, DeleteEcsTmplCallback);
}

StatusCode modsys_Init(void) {
  ecs = hm_IntKeyCreate();
  if (!ecs) {
    printf("Can not initialize ECS for the engine.\n");
    return FAILURE;
  }
  builtin_props_metadata = malloc(sizeof(PropsMetadata));
  if (!builtin_props_metadata) {
    printf("Can not initialize the props metadata for the engine, memory "
           "failure.\n");
    modsys_Exit();
    return FAILURE;
  }
  entity_buffer = malloc(sizeof(EntityDefineBuffer));
  if (!entity_buffer) {
    printf("Can not initialize the entity buffer for the engine, memory "
           "failure.\n");
    modsys_Exit();
    return FAILURE;
  }
  tmpl_buffer = malloc(sizeof(TmplDefineBuffer));
  if (!tmpl_buffer) {
    printf("Can not initialize the tmpl buffer for the engine, memory "
           "failure.\n");
    modsys_Exit();
    return FAILURE;
  }

  return SUCCESS;
}

StatusCode modsys_Exit(void) {
  if (ecs) {
    hm_IntKeyDelete(ecs, DeleteEcsTmplCallback);
    ecs = NULL;
  }
  if (builtin_props_metadata) {
    free(builtin_props_metadata);
    builtin_props_metadata = NULL;
  }
  if (entity_buffer) {
    free(entity_buffer);
    entity_buffer = NULL;
  }
  if (tmpl_buffer) {
    free(tmpl_buffer);
    tmpl_buffer = NULL;
  }

  return SUCCESS;
}
