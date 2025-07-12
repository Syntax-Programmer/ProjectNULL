#include "modsys.h"
#include "../utils/mem.h"

#define INTERNALS
/*
 * All the properties included here.
 */
#undef INTERNALS

#define PROP_ARR_CAP (16)

typedef struct ModSysChunk {
  void *props_data;
  u64 occupied_count;
  struct ModSysChunk *next;
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
  u64 tempalate_chunk_index;
};

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

static StatusCode ModSysChunkAdd(ModSysChunk **pHead, u64 total_prop_size);

static StatusCode ModSysChunkAdd(ModSysChunk **pHead, u64 total_prop_size) {
  ModSysChunk *new_chunk = malloc(sizeof(ModSysChunk));
  if (!new_chunk) {
    printf("Can not create a new modsys chunk, memory failure.\n");
    return FAILURE;
  }

  new_chunk->props_data = malloc(total_prop_size * PROP_ARR_CAP);
  if (!(new_chunk->props_data)) {
    printf("Can not create a new modsys chunk, memory failure.\n");
    return FAILURE;
  }
  new_chunk->occupied_count = 0;

  new_chunk->next = *pHead;
  *pHead = new_chunk;

  return SUCCESS;
}

static StatusCode ModSysTemplateAdd(ModSysTemplate **pHead, ModSysProps props) {
  ModSysTemplate *template = malloc(sizeof(ModSysTemplate));
  if (!template) {
      printf("Can not create a new modsys template, memory failure.\n");
      return FAILURE;
  }
  template->props = props;
  template->props_count = __builtin_popcount((i32)props);
  template->total_prop_size = 0;

  /*
   * if (HAS_FLAG(props, PROP1)) {
   *   template->total_prop_size += sizeof(prop1);
   * }
   */

  template->template_data = NULL;
  ModSysChunkAdd(&template->template_data, template->total_prop_size);

  template->next = *pHead;
  *pHead = template;

  return SUCCESS;
}

StatusCode modsys_Init() { return SUCCESS; }
