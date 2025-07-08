#include "modsys.h"
#include "../utils/mem.h"

#define INTERNALS
/*
 * All the properties included here.
 */
#undef INTERNALS

#define PROP_ARR_CAP (8)

typedef struct ModSysChunk {
  void *props_data;
  u64 occupied_count;
  struct ModSysChunk *next;
} ModSysChunk;

struct __ModSysTemplate {
  ModSysChunk *template_data;
  ModSysProps prop;
  u64 prop_count;
  struct __ModSysTemplate *next;
};

/*
 * This points to the exact memory where that particular enmtity is stored.
 *
 * After deleting an entity it is curropted to prevent use after free.
 *
 * The delete function should take a ModSyshandle **, and then *handle = NULL to
 * completely nullify the address making it safe.
 *
 * Use a dynamically growing pool allocator to store these for faster
 * allocations, with virtually 0 cost.
 */
struct __ModSysHandle {
  ModSysTemplate *template_ptr;
  ModSysChunk *data_chunk_ptr;
  u64 tempalate_chunk_index;
};

/*
 * This represents our ecs system. Each new entity definition will be stored
 * here.
 *
 * Also the templates users make, will be stored here and a pointer to the
 * template will be given to the user, to let them keep it as they want.
 *
 * Each template will be identified by the props it holds and each template
 * holds unique set of props.
 */
ModSysTemplate *__template_definition_state = NULL;

ModSysChunk *Add

StatusCode modsys_Init() {

    return SUCCESS;
}
