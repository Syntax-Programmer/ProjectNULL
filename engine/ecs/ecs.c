#include "ecs.h"
#include "../types/array.h"
#include "../types/hm.h"
#include "../utils/mem.h"
#include <stdbool.h>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

#define CHUNK_ARR_CAP (8)

struct __EntityLayout {
  /*
   * This is an array of mem block, and each mem block holds arrays for 8
   * entities of the defined props the layout belongs to. This allows for super
   * local data with one continuous buffer for all data.
   */
  Vector *data;
  Vector *data_free_indices; // array of u64
  EntityProps props;
};

struct __Entity {
  EntityLayout *layout;
  u64 index;
};

typedef struct {
  u64 size[ECS_PROPS_COUNT];
  bool is_serializable[ECS_PROPS_COUNT];
} PropsMetadata;

typedef struct {
  /*
   * Since all three LayoutMem, EntityLayout and Entity are 24 bytes
   fixed
   * size, and require frequent creation and deletion, it is natural for this
   * engine to prefer them to be allocated in pools to avoid that constant
   * malloc.
   *
   * layout_mem_arena, and layout_arena are also special as they are
   * values of the ecs hashmap. The Hm_IntKey implementation needs a
   * **val_delete_callback**. Will now also provide NULL to it(and let Hm
   * scream logs at us), which means the Hm won't attempt to free the values.
   We
   * can do it ourselves by purging the pool arenas in one go.
   */
  PoolArena *layout_arena;
  PoolArena *entity_arena;
  /*
   * This represents the entire ECS, where the key is EntityProps, and the
   * values are EntityLayout. A hashmap for O(1) access and manipulation made
   for
   * performant entity creation.
   */
  Hm_IntKey *ecs;
  /*
   * Defines the metadata for each component, mapping the props enum to its
   * frequently used props. This is to reduce the time that it may take
   * determining metadata from props.
   */
  PropsMetadata *builtin_props_metadata;
} EcsState;

static EcsState *ecs_state = NULL;
#define ECS_STATE_MISSING_LOG                                                  \
  ("Modsys functions called without initializing modsys.")
#define CHECK_VALID_ECS_STATE(ret_val)                                         \
  NULL_EXCEPTION_ROUTINE(ecs_state, ret_val, ECS_STATE_MISSING_LOG)

/* ----  UTILITY FUNCTIONS   ---- */

static u64 PropTableIndex(EntityProps prop);
static StatusCode PopulateBuiltinPropsMetadata(void);

/* ----  LAYOUT RELATED FUNCTIONS  ---- */

static u64 LayoutDataGrowCallback(u64 size);
static StatusCode AddLayoutMem(EntityLayout *layout);
static StatusCode LayoutDeleteCallback(void *layout);

/* ----  UTILITY FUNCTIONS   ---- */

static u64 PropTableIndex(EntityProps prop) {
  if (prop == 0)
    return -1; //  Undefined for zero

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

/* ----  PROPS RELATED FUNCTIONS  ---- */

/*
 * These wrapper functions are provided because `EntityProps` may not always be
 * a `u64`. In the future, it might evolve into a struct or array. By using
 * these wrappers, users won't need to change their code if the internal
 * representation changes.
 */

StatusCode ecs_AttachProp(EntityProps *props, EntityProps to_attach);
StatusCode ecs_DetachProp(EntityProps *props, EntityProps to_remove);
StatusCode ecs_ClearProps(EntityProps *props);

/* ----  LAYOUT RELATED FUNCTIONS  ---- */

static u64 LayoutDataGrowCallback(u64 size) { return size++; }

static StatusCode AddLayoutMem(EntityLayout *layout) {
  // Get curr max index before pushing new. Will account for next chunk later.
  u64 new_index = arr_VectorLen(layout->data) * CHUNK_ARR_CAP;

  IF_FUNC_FAILED(
      arr_VectorPushEmpty(layout->data, LayoutDataGrowCallback, false)) {
    STATUS_LOG(CREATION_FAILURE, "Unable to add memory to layout.");
    return CREATION_FAILURE;
  }

  for (u64 i = new_index; i < new_index + CHUNK_ARR_CAP; i++) {
    IF_FUNC_FAILED(arr_VectorPush(layout->data_free_indices, &i, NULL)) {
      STATUS_LOG(FAILURE, "Failed to enter free spots in the layout. Previous "
                          "data still persists.");
      return FAILURE;
    }
  }

  return SUCCESS;
}

EntityLayout *ecs_EntityLayoutCreate(EntityProps props) {
  CHECK_VALID_ECS_STATE(NULL);

  if (props == 0) {
    STATUS_LOG(FAILURE, "Cannot create a layout with no components.");
    return NULL;
  }

  EntityLayout *layout = NULL;
  if ((layout = hm_IntKeyFetchEntry(ecs_state->ecs, props)) != NULL) {
    return layout;
  }

  layout = mem_PoolArenaCalloc(ecs_state->layout_arena);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(layout, NULL);

  layout->props = props;
  // Size of one of each attached components.
  u64 props_struct_size = 0;
  // This lets us decompose prop bitflags into individual props.
  while (props) {
    // Extract the lowest most set bit.ModSysProps
    EntityProps prop = props & -props;

    u64 index = PropTableIndex(prop);
    props_struct_size += ecs_state->builtin_props_metadata->size[index];

    // Clearing the lowest set bit from the props;
    props ^= prop;
  }

  layout->data = arr_VectorCustomCreate(props_struct_size * CHUNK_ARR_CAP, 1);
  IF_NULL(layout->data) {
    LayoutDeleteCallback(layout);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(layout->data, NULL);
  }
  layout->data_free_indices =
      arr_VectorCustomCreate(sizeof(u64), CHUNK_ARR_CAP);
  IF_NULL(layout->data_free_indices) {
    LayoutDeleteCallback(layout);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(layout->data_free_indices, NULL);
  }

  IF_FUNC_FAILED(AddLayoutMem(layout)) {
    LayoutDeleteCallback(layout);
    STATUS_LOG(CREATION_FAILURE, "Cannot create initial memory for layout.");
    return NULL;
  }

  IF_FUNC_FAILED(
      hm_IntKeyAddEntry(ecs_state->ecs, layout->props, layout, HM_ADD_FAIL)) {
    LayoutDeleteCallback(layout);
    STATUS_LOG(FAILURE, "Cannot add layout to ecs.");
    return NULL;
  }

  return layout;
}

static StatusCode LayoutDeleteCallback(void *layout) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(layout, NULL_EXCEPTION);

  EntityLayout *to_delete = layout;
  if (to_delete->data) {
    arr_VectorDelete(to_delete->data);
  }
  if (to_delete->data_free_indices) {
    arr_VectorDelete(to_delete->data_free_indices);
  }
  mem_PoolArenaFree(ecs_state->layout_arena, to_delete);

  return SUCCESS;
}

StatusCode ecs_EntityLayoutDelete(EntityLayout *layout) {
  return hm_IntKeyDeleteEntry(ecs_state->ecs, layout->props,
                              LayoutDeleteCallback);
}

Entity *ecs_CreateEntityFromLayout(EntityLayout *layout) {
  CHECK_VALID_ECS_STATE(NULL);
  NULL_FUNC_ARG_ROUTINE(layout, NULL);

  if (arr_VectorLen(layout->data_free_indices) == 0) {
    IF_FUNC_FAILED(AddLayoutMem(layout)) {
      STATUS_LOG(CREATION_FAILURE,
                 "Failed to find valid spot to create entity in.");
      return NULL;
    }
  }

  Entity *entity = mem_PoolArenaAlloc(ecs_state->entity_arena);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(entity, NULL);

  entity->layout = layout;

  arr_VectorPop(layout->data_free_indices, &entity->index);

  return entity;
}

StatusCode ecs_DeleteEntityFromLayout(Entity *entity) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(entity, NULL_EXCEPTION);

  if (!entity->layout || entity->index == -1) {
    STATUS_LOG(FAILURE, "Cannot delete an already deleted entity.");
  }

  IF_FUNC_FAILED(
      /*
       * Since the layout and entity are opaque pointers, we trust
       * in the creation that no duplicate values to be sent down the stream.
       */
      arr_VectorPush(entity->layout->data_free_indices, &entity->index, NULL)) {
    STATUS_LOG(FAILURE, "Failed to delete entity from layout.");
    return FAILURE;
  }

  entity->layout = NULL;
  entity->index = -1;

  return SUCCESS;
}

/* ----  INIT/EXIT FUNCTIONS  ---- */

#define INIT_FAILED_ROUTINE(x)                                                 \
  do {                                                                         \
    IF_NULL((x)) {                                                             \
      ecs_Exit();                                                              \
      MEM_ALLOC_FAILURE_SUB_ROUTINE(x, CREATION_FAILURE);                      \
    }                                                                          \
  } while (0)

StatusCode ecs_Init(void) {
  ecs_state = calloc(1, sizeof(EcsState));
  INIT_FAILED_ROUTINE(ecs_state);

  ecs_state->layout_arena = mem_PoolArenaCreate(sizeof(EntityLayout));
  INIT_FAILED_ROUTINE(ecs_state->layout_arena);

  ecs_state->entity_arena = mem_PoolArenaCreate(sizeof(Entity));
  INIT_FAILED_ROUTINE(ecs_state->entity_arena);

  ecs_state->ecs = hm_IntKeyCreate();
  INIT_FAILED_ROUTINE(ecs_state->ecs);

  ecs_state->builtin_props_metadata = calloc(1, sizeof(PropsMetadata));
  INIT_FAILED_ROUTINE(ecs_state->builtin_props_metadata);
  PopulateBuiltinPropsMetadata();

  return SUCCESS;
}

StatusCode ecs_Exit(void) {
  if (!ecs_state) {
    return SUCCESS;
  }

  if (ecs_state->entity_arena) {
    mem_PoolArenaDelete(ecs_state->entity_arena);
  }
  if (ecs_state->ecs) {
    if (ecs_state->layout_arena) {
      hm_IntKeyDelete(ecs_state->ecs, LayoutDeleteCallback);
      mem_PoolArenaDelete(ecs_state->layout_arena);
    }
    hm_IntKeyDelete(ecs_state->ecs, NULL);
  }
  if (ecs_state->builtin_props_metadata) {
    free(ecs_state->builtin_props_metadata);
  }

  free(ecs_state);
  ecs_state = NULL;

  return SUCCESS;
}
