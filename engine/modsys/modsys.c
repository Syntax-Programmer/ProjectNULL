#include "modsys.h"
#include "../utils/mem.h"

#define INTERNALS
/*
 * All the properties included here.
 */
#undef INTERNALS

#define PROP_ARR_CAP (16)

typedef struct __ModSysChunk {
  void *props_data;
  u64 occupied_count;
  struct __ModSysChunk *next;
} ModSysChunk;

struct __ModSysTemplate {
  ModSysChunk *template_data;
  ModSysProps props;
  u64 props_count;
  // Sum of sizes of each individual props we have defined.
  u64 total_prop_size;
  struct __ModSysTemplate *next;
};

/*
 * This points to the exact memory where that particular enmtity is stored.
 *
 * After deleting an entity it is curropted to prevent use after free.
 *
 * The delete function should take a ModSyshandle **, and then *handle = NULL to
 * completely nullify the address making it safe.
 */
struct __ModSysHandle {
  ModSysTemplate *template_ptr;
  ModSysChunk *data_chunk_ptr;
  u64 template_chunk_index;
};

typedef struct {
  /*
   * Prop1 prop1;
   * Prop2 prop2;
   * ...
   */
  ModSysProps props;
} ModSysPreLockBuffer;
/*
 * Entity defs are written here before copying it into the actual ecs.
 * This reduces the number of defintion reallocation, search time and overall
 * memory handling. After definiton we just push the buffer into the ecs in one
 * copy operation.
 */
ModSysPreLockBuffer *__pre_lock_buffer = NULL;
/*
 * This allows for us to only malloc at runtime, the actual memory chunk whose
 * size is unknown. Otherwise, the chunk struct itself is constant memory size
 * so using pool arena is suitable.
 */
PoolArena *__chunk_alloc_arena = NULL;
/*
 * In this pool, the ecs lives. Since ecs is just ModSysTemplate *, allocating
 * ModSysTemplate inside a pool is a performance optimization.
 */
PoolArena *__template_alloc_arena = NULL;
/*
 * Since we don't know how many handles will be created, but we know each handle
 * is 24 bytes each, allocating it inside a pool allows for us to not deal with
 * constant malloc.
 */
PoolArena *__handle_alloc_arena = NULL;
/*
 * This represents our ecs system. Each new entity definition will be stored
 * here.
 *
 * Also the templates users make, will be stored here and a pointer to the
 * template will be given to the user, to let them keep it as they want.
 *
 * Each template will be identified by the props it holds and each template
 * holds unique set of props.
 *
 * This ECS is allocated inside the __template_alloc_arena.
 */
ModSysTemplate *__template_definition_state = NULL;

static StatusCode ModSysChunkAdd(ModSysTemplate *template);
static StatusCode ModSysTemplateAdd(ModSysProps props);

static StatusCode ModSysChunkAdd(ModSysTemplate *template) {
  assert(template && __chunk_alloc_arena);

  ModSysChunk *new_chunk = mem_PoolArenaAlloc(__chunk_alloc_arena);
  if (!new_chunk) {
    printf("Can not create a new modsys chunk, memory failure.\n");
    return FAILURE;
  }

  new_chunk->occupied_count = 0;
  new_chunk->props_data = malloc(template->total_prop_size * PROP_ARR_CAP);
  if (!new_chunk->props_data) {
    mem_PoolArenaFree(__chunk_alloc_arena, new_chunk);
    printf("Can not create a new modsys chunk, memory failure.\n");
    return FAILURE;
  }

  new_chunk->next = template->template_data;
  template->template_data = new_chunk;

  return SUCCESS;
}

static StatusCode ModSysTemplateAdd(ModSysProps props) {
  assert(__template_alloc_arena && __template_definition_state);

  if (props == NO_PROPS) {
    printf("Can not allocate a template for props = NO_PROPS.\n");
    return FAILURE;
  }

  ModSysTemplate *new_template = mem_PoolArenaAlloc(__template_alloc_arena);
  if (!new_template) {
    printf("Can not create a new modsys template, memory failure.\n");
    return FAILURE;
  }

  new_template->props = props;
  new_template->props_count = __builtin_popcount((i32)props);
  new_template->total_prop_size = 0;
  /*
   * if (HAS_FLAG(props, PROP1)) {
   *   new_template->total_prop_size += sizeof(prop1);
   * }
   */

  new_template->template_data = NULL;
  if (ModSysChunkAdd(new_template) != SUCCESS) {
    mem_PoolArenaFree(__template_alloc_arena, new_template);
    printf("Can not add initial chunk to modsys template of props = %d", props);
    return FAILURE;
  }

  new_template->next = __template_definition_state;
  __template_definition_state = new_template;

  return SUCCESS;
}

StatusCode modsys_Init() {
  __chunk_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysChunk));
  __template_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysTemplate));
  __handle_alloc_arena = mem_PoolArenaCreate(sizeof(ModSysHandle));
  if (!__chunk_alloc_arena || !__template_alloc_arena ||
      !__handle_alloc_arena) {
    modsys_Exit();
    return FAILURE;
  }
  __pre_lock_buffer = malloc(sizeof(ModSysPreLockBuffer));
  if (!__pre_lock_buffer) {
    printf("Can not allocate memory for ECS pre lock buffer.\n");
    modsys_Exit();
    return FAILURE;
  }

  return SUCCESS;
}

StatusCode modsys_Exit() {
  if (__chunk_alloc_arena) {
    mem_PoolArenaDelete(__chunk_alloc_arena);
    __chunk_alloc_arena = NULL;
  }
  if (__template_alloc_arena) {
    mem_PoolArenaDelete(__template_alloc_arena);
    __template_alloc_arena = NULL;
  }
  if (__handle_alloc_arena) {
    mem_PoolArenaDelete(__handle_alloc_arena);
    __handle_alloc_arena = NULL;
  }
  if (__pre_lock_buffer) {
    free(__pre_lock_buffer);
    __pre_lock_buffer = NULL;
  }

  __template_definition_state = NULL;

  return SUCCESS;
}
