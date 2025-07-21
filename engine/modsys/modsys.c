#include "modsys.h"
#include "../types/hm.h"
#include "../utils/mem.h"

#define PROPS_INTERNALS
/*
 * All the properties included here.
 */
#undef PROPS_INTERNALS

#define PROP_ARR_CAP (24)

// /*
//  * This points to the exact memory where that particular enmtity is stored.
//  *
//  * After deleting an entity it is curropted to prevent use after free.
//  *
//  * The delete function should take a ModSyshandle **, and then *handle = NULL
//  to
//  * completely nullify the address making it safe.
//  */
// struct __ModSysHandle {
//   ModSysTemplate *template_ptr;
//   ModSysChunk *data_chunk_ptr;
//   u64 template_chunk_index;
// };

// typedef struct {
//   /*
//    * Prop1 prop1;
//    * Prop2 prop2;
//    * ...
//    */
//   ModSysProps props;
//   bool in_use;
// } ModSysPreLockBuffer;

// /*
//  * Entity defs are written here before copying it into the actual ecs.
//  * This reduces the number of definition reallocation, search time and
//  overall
//  * memory handling. After definition we just push the buffer into the ecs in
//  one
//  * copy operation.
//  */
// ModSysPreLockBuffer *__pre_lock_buffer = NULL;

// StatusCode modsys_CreateEntity() {
//   if (!__pre_lock_buffer) {
//     printf("__pre_lock_buffer not properly initialized that didin't trigger "
//            "crash in initialization steps.\n");
//     return FAILURE;
//   }
//   if (__pre_lock_buffer->in_use) {
//     printf("An entity definition is active, but not completed. Complete that
//     "
//            "before creating new entity.\n");
//     return FAILURE;
//   }

//   __pre_lock_buffer->in_use = true;

//   return SUCCESS;
// }

// StatusCode modsys_AddProp(ModSysProps prop, void *data) {
//   if (!__pre_lock_buffer->in_use) {
//     printf("Create an entity before trying to add props to it.\n");
//     return FAILURE;
//   }
//   if (!IS_POWER_OF_TWO(prop)) {
//     printf("Please try to add one prop at a time with its respective
//     data.\n"); return FAILURE;
//   }

//   return SUCCESS;
// }

typedef struct {
  u64 size;
  bool is_serializable;
} PropsMetadata;

const PropsMetadata builtin_props_metadata[MODSYS_PROPS_COUNT] = {};

typedef struct __ModSysChunk {
  void *data;
  u64 len;
  struct __ModSysChunk *next;
} ModSysChunk;

struct __ModSysTemplate {
  ModSysChunk *chunk;
  // Size of one of each prop's struct for quick chunk creation.
  u64 props_struct_size;
  ModSysProps props;
};

/*
 * This represents the entire ECS, where the key is ModSysProps, and the values
 * are ModSysTemplate.
 * A hashmap for O(1) access and manipulation made for performant entity
 * creation.
 */
Hm_IntKey *ecs = NULL;

struct __ModSysHandle {
  ModSysChunk *chunk;
  /*
   * Not storing the props, but the template itself. This is to boost
   * performance since we don't have to re fetch from hashmap each time.
   */
  ModSysTemplate *template;
  u64 entry_index;
};

typedef struct {
  /*
   * Prop1 prop1;
   * ...
   */
  ModSysProps props;
  bool in_use;
} EntityDefineBuffer;

/*
 * Entity defs are written here before copying it into the actual ecs. This
 * reduces the number of definitions reallocation and search time.
 * This also significantly simplifies the amount of memory handling.
 */
EntityDefineBuffer *entity_buffer = NULL;

static StatusCode ModSysChunkAdd(ModSysTemplate *template);
static StatusCode ModSysChunkDelete(ModSysTemplate *template);

static StatusCode AddECSTemplate(ModSysProps props);
static StatusCode DeleteEcsTemplateCallback(void *template);
static StatusCode DeleteEcsTemplate(ModSysTemplate *template);

static StatusCode ModSysChunkAdd(ModSysTemplate *template) {
  assert(ecs && template);
  /*
   * This method of malloc prevents heap fragmentation, and also allows us to
   * only free ModSysChunk, and the chunk->data memory will be freed itself.
   *
   * Its a variation of struct hack.
   */
  void *ptr =
      malloc(sizeof(ModSysChunk) + template->props_struct_size * PROP_ARR_CAP);
  if (!ptr) {
    printf("Can not create a new modsys chunk, memory failure.\n");
    return FAILURE;
  }

  ModSysChunk *chunk = ptr;
  chunk->data = MEM_OFFSET(ptr, sizeof(ModSysChunk));
  chunk->len = 0;

  chunk->next = template->chunk;
  template->chunk = chunk;

  return SUCCESS;
}

static StatusCode ModSysChunkDelete(ModSysTemplate *template) {
  assert(ecs && template);

  ModSysChunk *curr = template->chunk, *next = NULL;
  while (curr) {
    next = curr->next;

    // Only need to free curr since the struct was created using struct hack.
    free(curr);

    curr = next;
  }

  return SUCCESS;
}

static StatusCode AddECSTemplate(ModSysProps props) {
  assert(ecs);

  if (props == 0) {
    printf("Can not add a template with no/invalid props.\n");
    return FAILURE;
  }

  if (hm_IntKeyFetchEntry(ecs, props)) {
    // The template already exists.
    return SUCCESS;
  }

  ModSysTemplate *template = malloc(sizeof(ModSysTemplate));
  if (!template) {
    printf("Can not create memory for new template with props: %ld.\n", props);
    return FAILURE;
  }
  template->props = props;
  template->props_struct_size = 0;
  /*
   * TODO: Make the size when we have some props defined.
   */
  if (ModSysChunkAdd(template) != SUCCESS) {
    free(template);
    printf("Can not create initial memory chunk for the template with props: "
           "%ld.\n",
           props);
    return FAILURE;
  }

  hm_IntKeyAddEntry(ecs, props, template, false);

  return SUCCESS;
}

static StatusCode DeleteEcsTemplateCallback(void *template) {
  assert(ecs && template);

  ModSysTemplate *tmp = template;

  ModSysChunkDelete(tmp);
  free(tmp);

  return SUCCESS;
}

static StatusCode DeleteEcsTemplate(ModSysTemplate *template) {
  assert(ecs);

  return hm_IntKeyDeleteEntry(ecs, template->props, DeleteEcsTemplateCallback);
}

StatusCode modsys_Init(void) {
  ecs = hm_IntKeyCreate();
  if (!ecs) {
    printf("Can not initialize ECS for the engine.\n");
    return NULL;
  }

  return SUCCESS;
}

StatusCode modsys_Exit(void) {
  if (ecs) {
    hm_IntKeyDelete(ecs, DeleteEcsTemplateCallback);
    ecs = NULL;
  }

  return SUCCESS;
}
