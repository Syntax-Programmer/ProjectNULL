#include "ecs.h"
#include "../types/array.h"
#include "../types/hm.h"
#include "../utils/mem.h"
#include <time.h>

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

#define CHUNK_ARR_CAP (8)
#define U64_BIT_COUNT (sizeof(u64) * 8)

struct __PropsSignature {
  // Its an array of u64, where each u64 holds 64 props as for of bitset.
  BuffArr *id_bitset;
};

struct __Layout {
  /*
   * This is an array of mem block, and each mem block holds arrays for 8
   * entities of the defined props the layout belongs to. This allows for super
   * local data with one continuous buffer for all data.
   */
  // Chose Vector as it auto grows.😁
  Vector *data;
  Vector *data_free_indices; // array of u64
  PropsSignature *layout_signature;
};

struct __Entity {
  Layout *layout;
  u64 index;
};

typedef struct {
  Vector *size;
} PropsMetadata;

typedef struct {
  PoolArena *layout_arena;
  PoolArena *entity_arena;
  PoolArena *props_signature_arena;
  // Seed used to hash the prop signature for ecs hashmap.
  time_t signature_hash_seed;
  /*
   * This represents the entire ECS, where the key is PropSignature, and the
   * values are Layout. A hashmap for O(1) access and manipulation made
   for
   * performant entity creation.
   */
  Hm *ecs;
  /*
   * Defines the metadata for each component, mapping the props enum to its
   * frequently used props. This is to reduce the time that it may take
   * determining metadata from props.
   */
  PropsMetadata props_metadata_table;
} EcsState;

static EcsState *ecs_state = NULL;
#define ECS_STATE_MISSING_LOG                                                  \
  ("Modsys functions called without initializing modsys.")
#define CHECK_VALID_ECS_STATE(ret_val)                                         \
  NULL_EXCEPTION_ROUTINE(ecs_state, ret_val, ECS_STATE_MISSING_LOG)

/* ----  UTILITY FUNCTIONS   ---- */

static u64 PropsMetadataTableIndex(u64 prop_bitset, u64 int_index);

/* ----  PROPS METADATA RELATED FUNCTIONS  ---- */

static StatusCode PropsMetadataCreate(void);
static StatusCode PropsMetadataDelete(void);
static StatusCode PopulateBuiltinPropsMetadata(void);

/* ----  PROP RELATED FUNCTIONS  ---- */

static inline u64 mix64(u64 x);
static u64 PropsSignatureHashFunc(const void *signature);
static bool PropsSignatureCmpFunc(const void *signature,
                                  const void *the_other_one);
static StatusCode PropSignatureDeleteCallback(void *signature);

/* ----  LAYOUT RELATED FUNCTIONS  ---- */

static u64 LayoutDataGrowCallback(u64 size);
static StatusCode AddLayoutMem(Layout *layout);
static StatusCode LayoutDeleteCallback(void *layout);

/* ----  UTILITY FUNCTIONS   ---- */

static u64 PropsMetadataTableIndex(u64 prop_bitset, u64 int_index) {
  if (prop_bitset == 0) {
    return INVALID_INDEX; // Undefined for zero
  }

  u64 bit_position = INVALID_INDEX;

#if defined(__GNUC__) || defined(__clang__)
  bit_position = __builtin_ctzll(prop_bitset);

#elif defined(_MSC_VER)
  unsigned long index;
  _BitScanForward64(&index, prop);
  table_index = (u64)index;

#else
  // Portable fallback
  u64 n = 0;
  while ((prop & 1) == 0) {
    prop >>= 1;
    n++;
  }
  table_index = n;
#endif

  return bit_position + (int_index * U64_BIT_COUNT);
}

/* ----  PROPS METADATA RELATED FUNCTIONS  ---- */

static StatusCode PropsMetadataCreate(void) {
  ecs_state->props_metadata_table.size = arr_VectorCreate(sizeof(u64));
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(ecs_state->props_metadata_table.size,
                                       CREATION_FAILURE);

  return SUCCESS;
}

static StatusCode PropsMetadataDelete(void) {
  if (ecs_state->props_metadata_table.size) {
    arr_VectorDelete(ecs_state->props_metadata_table.size);
  }

  return SUCCESS;
}

static StatusCode PopulateBuiltinPropsMetadata(void) {
  /*
   * TODO:
   * builtin_props_metadata.size[PROP1] = sizeof(Prop1)
   * ...
   */

  return SUCCESS;
}

/* ----  PROP RELATED FUNCTIONS  ---- */

PropId ecs_PropIdCreate(u64 prop_struct_size) {
  CHECK_VALID_ECS_STATE(INVALID_PROP_ID);

  PropId id = arr_VectorLen(ecs_state->props_metadata_table.size);
  IF_FUNC_FAILED(arr_VectorPush(ecs_state->props_metadata_table.size,
                                &prop_struct_size, NULL)) {
    STATUS_LOG(FAILURE, "Unable to create new PropId. Internal failure.");
    return INVALID_PROP_ID;
  }

  return id;
}

PropsSignature *ecs_PropSignatureCreate(void) {
  CHECK_VALID_ECS_STATE(NULL);

  PropsSignature *signature =
      mem_PoolArenaAlloc(ecs_state->props_signature_arena);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(signature, NULL);
  /*
   * The signature preallocates for all the props that exists. This shall not be
   * a problem as the load is 8 bytes per 64 props, so its super lean.
   *
   * This also means that there is chance that two signatures can have different
   * allocated sizes if they were created at different times, but thats also
   * okay and not undefined state.
   */
  u64 props_count = arr_VectorLen(ecs_state->props_metadata_table.size);
  // Taking the ceil.
  u64 cap = (props_count + U64_BIT_COUNT - 1) / U64_BIT_COUNT;

  signature->id_bitset = arr_BuffArrCreate(sizeof(u64), cap);
  IF_NULL(signature->id_bitset) {
    ecs_PropsSignatureDelete(signature);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(signature->id_bitset, NULL);
  }

  return signature;
}

StatusCode ecs_PropsSignatureDelete(PropsSignature *signature) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(signature, NULL_EXCEPTION);

  // This public function checks for invalid state on top of static func which
  // doesn't check anything.
  return PropSignatureDeleteCallback(signature);
}

StatusCode ecs_HandlePropIdToPropSignatures(PropsSignature *signature,
                                            PropId id,
                                            PropsSignatureHandleMode mode) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(signature, NULL_EXCEPTION);

  if (mode != PROP_SIGNATURE_ATTACH && mode != PROP_SIGNATURE_DETACH) {
    STATUS_LOG(FAILURE, "Invalid mode provided.");
  }
  char *log_str = (mode == PROP_SIGNATURE_DETACH) ? "detach" : "attach";

  u64 props_count = arr_VectorLen(ecs_state->props_metadata_table.size);
  if (id >= props_count) {
    STATUS_LOG(FAILURE, "Invalid PropId: %zu provided to %s.", id, log_str);
    return FAILURE;
  }
  /*
   * required_signature_cap - 1 also doubles as the index of the int where to
   * write/unwrite the PropId bit.
   */
  u64 required_signature_cap = (id + U64_BIT_COUNT) / U64_BIT_COUNT;
  u64 curr_signature_cap = arr_BuffArrCap(signature->id_bitset);
  if (required_signature_cap > curr_signature_cap) {
    if (mode == PROP_SIGNATURE_DETACH) {
      STATUS_LOG(FAILURE,
                 "Tried to detach a prop that hadn't yet been attached.");
      return FAILURE;
    } else {
      IF_FUNC_FAILED(
          arr_BuffArrGrow(signature->id_bitset, required_signature_cap)) {
        STATUS_LOG(FAILURE, "Cannot add PropId to PropsSignature, signature "
                            "coundn't create space to store it.");
        return FAILURE;
      }
    }
  }
  /*
   * Using raw data here as at this point we are sure required_signature_cap - 1
   * is a valid index in the array. and using the getters and setter will just
   * bloat the function.
   */
  u64 *bitset_raw = arr_BuffArrRaw(signature->id_bitset);
  NULL_EXCEPTION_ROUTINE(bitset_raw, NULL_EXCEPTION,
                         "Cannot access signature internals to %s prop id.",
                         log_str);
  if (mode == PROP_SIGNATURE_DETACH) {
    CLEAR_FLAG(bitset_raw[required_signature_cap - 1],
               1UL << (id % U64_BIT_COUNT));
  } else {
    bitset_raw[required_signature_cap - 1] |= 1UL << (id % U64_BIT_COUNT);
  }

  return SUCCESS;
}

StatusCode ecs_PropSignatureClear(PropsSignature *signature) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(signature, NULL_EXCEPTION);

  arr_BuffArrReset(signature->id_bitset);

  return SUCCESS;
}

// 64-bit finalizer (from MurmurHash3)
static inline u64 mix64(u64 x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

static u64 PropsSignatureHashFunc(const void *signature) {
  const PropsSignature *sign = signature;

  const u64 *data = arr_BuffArrRaw(sign->id_bitset);
  size_t cap = arr_BuffArrCap(sign->id_bitset);

  u64 hash = ecs_state->signature_hash_seed;

  for (size_t i = 0; i < cap; i++) {
    u64 k = mix64(data[i]); // pre-mix each element
    hash ^=
        k + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2); // Jenkins mix
  }
  hash ^= cap; // cap affects result

  return mix64(hash); // final avalanche
}

static bool PropsSignatureCmpFunc(const void *signature,
                                  const void *the_other_one) {
  const PropsSignature *sign1 = signature, *sign2 = the_other_one;

  return arr_BuffArrCmp(sign1->id_bitset, sign2->id_bitset);
}

static StatusCode PropSignatureDeleteCallback(void *signature) {
  PropsSignature *sign = signature;

  if (sign->id_bitset) {
    arr_BuffArrDelete(sign->id_bitset);
  }
  mem_PoolArenaFree(ecs_state->props_signature_arena, sign);

  return SUCCESS;
}

/* ----  LAYOUT RELATED FUNCTIONS  ---- */

static u64 LayoutDataGrowCallback(u64 size) { return ++size; }

static StatusCode AddLayoutMem(Layout *layout) {
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

Layout *ecs_LayoutCreate(PropsSignature *signature) {
  CHECK_VALID_ECS_STATE(NULL);
  NULL_FUNC_ARG_ROUTINE(signature, NULL);

  Layout *layout = NULL;
  if ((layout = hm_GetEntry(ecs_state->ecs, signature)) != NULL) {
    return layout;
  }

  // Size of one of each attached components.
  u64 props_struct_size = 0;
  u64 *size_raw = arr_VectorRaw(ecs_state->props_metadata_table.size);
  u64 *prop_signature_raw = arr_BuffArrRaw(signature->id_bitset);
  u64 prop_signature_cap = arr_BuffArrCap(signature->id_bitset);

  // Checking if the layout contains any props at all or not.
  for (u64 i = 0; i < props_struct_size; i++) {
    if (prop_signature_raw[i] != 0) {
      break;
    }
    if (i == prop_signature_cap - 1) {
      STATUS_LOG(
          FAILURE,
          "Attach props to the signature before trying to create layout.");
      return NULL;
    }
  }

  for (u64 i = 0; i < prop_signature_cap; i++) {
    u64 bitset_int = prop_signature_raw[i];
    // This lets us decompose prop bitflags into individual props.
    while (bitset_int) {
      // Extract the lowest most set bit.
      u64 prop_bitset = bitset_int & -bitset_int;

      u64 index = PropsMetadataTableIndex(prop_bitset, i);
      if (index != INVALID_INDEX) {
        props_struct_size += size_raw[index];
      }

      // Clearing the lowest set bit from the props;
      bitset_int ^= prop_bitset;
    }
  }

  layout = mem_PoolArenaCalloc(ecs_state->layout_arena);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(layout, NULL);
  /*
   * We create the array with props_struct_size * CHUNK_ARR_CAP, but each
   * entry in itself be multiple arrays of capacity CHUNK_ARR_CAP.
   *
   * The arrangement will be like:
   * data-> [[Index0: [comp1_arr][comp2_arr]...[comp-n_arr]], [Index1: ...]]
   *
   * The props_struct_size * CHUNK_ARR_CAP is just a size calculation and
   * doesn't reflect internal memory structure.
   */
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
  layout->layout_signature = signature;

  IF_FUNC_FAILED(hm_AddEntry(ecs_state->ecs, layout->layout_signature, layout,
                             HM_ADD_FAIL)) {
    LayoutDeleteCallback(layout);
    STATUS_LOG(FAILURE, "Cannot add layout to ecs.");
    return NULL;
  }

  return layout;
}

static StatusCode LayoutDeleteCallback(void *layout) {
  Layout *to_delete = layout;
  if (to_delete->data) {
    arr_VectorDelete(to_delete->data);
  }
  if (to_delete->data_free_indices) {
    arr_VectorDelete(to_delete->data_free_indices);
  }
  /*
   * We don't free to_delete->layout_signature, as the ecs hashmap handles it
   * in the key delete callback. This is true as the PropsSignature is also
   * stored separately in the hashmap, and we let it free the signature rather
   * than we free it along side value.
   */
  mem_PoolArenaFree(ecs_state->layout_arena, to_delete);

  return SUCCESS;
}

StatusCode ecs_LayoutDelete(Layout *layout) {
  CHECK_VALID_ECS_STATE(NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(layout, NULL_EXCEPTION);

  return hm_DeleteEntry(ecs_state->ecs, layout->layout_signature);
}

Entity *ecs_CreateEntityFromLayout(Layout *layout) {
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

  if (!entity->layout || entity->index == INVALID_INDEX) {
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
  entity->index = INVALID_INDEX;

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

  ecs_state->layout_arena = mem_PoolArenaCreate(sizeof(Layout));
  INIT_FAILED_ROUTINE(ecs_state->layout_arena);

  ecs_state->entity_arena = mem_PoolArenaCreate(sizeof(Entity));
  INIT_FAILED_ROUTINE(ecs_state->entity_arena);

  ecs_state->props_signature_arena =
      mem_PoolArenaCreate(sizeof(PropsSignature));
  INIT_FAILED_ROUTINE(ecs_state->props_signature_arena);

  /*
   * Hashmap maps PropSignature to Layout. The layout will also be stored
   * inside the layout as well as the hashmap stores the same. We store the
   * signature inside the layout, as we return the layout to the user and we
   * need a way to check what type of layout it is.
   */
  ecs_state->ecs = hm_Create(PropsSignatureHashFunc, PropsSignatureCmpFunc,
                             PropSignatureDeleteCallback, LayoutDeleteCallback);
  INIT_FAILED_ROUTINE(ecs_state->ecs);

  IF_FUNC_FAILED(PropsMetadataCreate()) {
    ecs_Exit();
    STATUS_LOG(CREATION_FAILURE,
               "Failed to create metadata table for the ecs.");
    return CREATION_FAILURE;
  }
  PopulateBuiltinPropsMetadata();

  ecs_state->signature_hash_seed = time(NULL);

  return SUCCESS;
}

StatusCode ecs_Exit(void) {
  IF_NULL(ecs_state) { return SUCCESS; }

  if (ecs_state->entity_arena) {
    mem_PoolArenaDelete(ecs_state->entity_arena);
  }
  if (ecs_state->ecs) {
    // If ecs is created means that the layout arena was created first, and
    // the callbacks are valid.
    hm_Delete(ecs_state->ecs);
  }
  /*
   * layout and signature arenas will be freed after attempting the ecs
   * deletion, as ecs deletion will free internal allocations from these
   * arenas.
   */
  if (ecs_state->layout_arena) {
    mem_PoolArenaDelete(ecs_state->layout_arena);
  }
  if (ecs_state->props_signature_arena) {
    mem_PoolArenaDelete(ecs_state->props_signature_arena);
  }
  PropsMetadataDelete();

  free(ecs_state);
  ecs_state = NULL;

  return SUCCESS;
}
