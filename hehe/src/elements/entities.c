#include "../../include/elements/entities.h"
#include "../../include/utils/arena.h"
#include "../../include/utils/array.h"
#include "../../include/utils/yaml_parser.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_surface.h>

#define TYPEOF_SPEED_COMPONENT uint16_t
#define TYPEOF_HP_COMPONENT uint16_t
#define TYPEOF_ASSET_COMPONENT SDL_Texture *
#define TYPEOF_BBOX_COMPONENT SDL_FRect
#define TYPEOF_SLOTS_ARRAY uint32_t
#define TYPEOF_DIMENSION_COMPONENT uint16_t

/*
 * This defines, what masks a particular entity has, it also has bool flag
 * components to optimize for memory efficiency by not defining a separate array
 * for those.
 */
typedef enum {
  /* ----  IS ATTR DEFINED MASKS  ---- */
  BBOX = 1 << 0,
  HP_COMPONENT = 1 << 1,
  ASSET_COMPONENT = 1 << 2,
  SPEED_COMPONENT = 1 << 3,
  /* ----  BOOL ATTRS DEFINITIONS  ---- */
  INTRACTABLE_COMPONENT = 1 << 4,
  COLLIDABLE_COMPONENT = 1 << 5,
  /* ----  LAYOUT_SPECIFIC_FLAGS  ---- */
  WIDTH_COMPONENT = 1 << 6,
  HEIGHT_COMPONENT = 1 << 7,
} ComponentMasks;

#define MANDATORY_LAYOUT_COMPONENTS                                            \
  (WIDTH_COMPONENT | HEIGHT_COMPONENT | HP_COMPONENT | ASSET_COMPONENT)

struct EntityLayout {
  AppendArr *speeds;
  AppendArr *hps;
  AppendArr *assets;
  AppendArr *widths;
  AppendArr *heights;
  AppendArr *comp_masks;
};

struct EntityPool {
  FlexArr *bboxes;
  FlexArr *hps;
  FlexArr *assets;
  FlexArr *speeds;
  FlexArr *comp_masks;
  AppendArr *empty_slots;
  AppendArr *occupied_slots;
};

struct EntityModule {
  EntityLayout *entity_layout;
  EntityPool *entity_pool;
};

/* ----  ENTITY_LAYOUT  ---- */

static StatusCode AllocateEntityLayout(EntityLayout **pEntity_layout);
// It has a char buffer path, due to the yaml parser return type.
static TYPEOF_ASSET_COMPONENT LoadTexture(const CharBuffer texture_path,
                                          SDL_Renderer *renderer);

typedef struct {
  CharBuffer curr_id;
  ComponentMasks curr_mask;
  // TO make the assets.
  SDL_Renderer *renderer;
} ParsingExtra;

#define TRY_PUSH(array, val_ptr, flag_bit, mask)                               \
  do {                                                                         \
    if (arr_AppendArrPush((array), (val_ptr), NULL) == RESOURCE_EXHAUSTED) {   \
      LOG("Can't grow entity layout arrays anymore. Memory exhausted.");       \
      return FATAL_ERROR;                                                      \
    }                                                                          \
    SET_FLAG(mask, (flag_bit));                                                \
  } while (0)

static StatusCode PadNonMandatoryComponents(ComponentMasks mask,
                                            EntityLayout *entity_layout);
static StatusCode ParseIntoEntityLayout(void *dest, const CharBuffer key,
                                        const CharBuffer val,
                                        const CharBuffer id, void *extra);
static void ShrinkFitEntityLayout(EntityLayout *entity_layout);

/* ----  ENTITY POOL  ---- */

static StatusCode AllocateEntityPool(EntityPool **pEntity_pool,
                                     size_t pool_capacity);

/* ----  LOGIC  ---- */

static void InsertionSortWRTDimensionX(EntityPool *entity_pool);
static void ResolveCollision(SDL_FRect *dimension1, SDL_FRect *dimension2);

/* ----  ENTITY_LAYOUT  ---- */

static StatusCode AllocateEntityLayout(EntityLayout **pEntity_layout) {
  *pEntity_layout = arena_Alloc(sizeof(EntityLayout));
  if (!(*pEntity_layout)) {
    LOG("Can't allocate memory for the entity layout.");
    return RESOURCE_EXHAUSTED;
  }

  EntityLayout *layout = *pEntity_layout;
  layout->speeds = arr_AppendArrCreate(sizeof(TYPEOF_SPEED_COMPONENT));
  layout->hps = arr_AppendArrCreate(sizeof(TYPEOF_HP_COMPONENT));
  layout->assets = arr_AppendArrCreate(sizeof(TYPEOF_ASSET_COMPONENT));
  layout->widths = arr_AppendArrCreate(sizeof(TYPEOF_DIMENSION_COMPONENT));
  layout->heights = arr_AppendArrCreate(sizeof(TYPEOF_DIMENSION_COMPONENT));
  layout->comp_masks = arr_AppendArrCreate(sizeof(ComponentMasks));

  if (!layout->speeds || !layout->hps || !layout->assets || !layout->widths ||
      !layout->heights || !layout->comp_masks) {
    LOG("Can't allocate memory for the entity layout.");
    return RESOURCE_EXHAUSTED;
  }

  return SUCCESS;
}

static TYPEOF_ASSET_COMPONENT LoadTexture(const CharBuffer texture_path,
                                          SDL_Renderer *renderer) {
  SDL_Surface *surface = IMG_Load(texture_path);
  if (!surface) {
    LOG("IMG_Load Error: %s", IMG_GetError());
    return NULL;
  }

  TYPEOF_ASSET_COMPONENT texture =
      SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    LOG("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
    SDL_FreeSurface(surface);
    return NULL;
  }

  SDL_FreeSurface(surface);
  return texture;
}

static StatusCode PadNonMandatoryComponents(ComponentMasks mask,
                                            EntityLayout *entity_layout) {
  if (!HAS_FLAG(mask, SPEED_COMPONENT)) {
    TYPEOF_SPEED_COMPONENT speed = 0;
    if (arr_AppendArrPush(entity_layout->speeds, &speed, NULL) ==
        RESOURCE_EXHAUSTED) {
      LOG("Can't grow entity layout arrays anymore. Memory exhausted.");
      return RESOURCE_EXHAUSTED;
    }
  }

  return SUCCESS;
}

static StatusCode ParseIntoEntityLayout(void *dest, const CharBuffer key,
                                        const CharBuffer val,
                                        const CharBuffer id, void *extra) {
  EntityLayout *entity_layout = dest;
  ParsingExtra *parsing_extra = extra;

  if (parsing_extra->curr_id[0] == '\0') {
    snprintf(parsing_extra->curr_id, CHAR_BUFFER_SIZE, "%s", id);
  }

  /*
   * This means current id definition is complete and a new id definition is
   * starting. Performing safety checks on the currently defined id.
   */
  IF_CHARBUFF_NOT_EQUALS(parsing_extra->curr_id, id) {
    if (!HAS_ALL_FLAGS(parsing_extra->curr_mask, MANDATORY_LAYOUT_COMPONENTS)) {
      LOG("The id, %s, does not contain the all mandatory components of "
          "entities.",
          parsing_extra->curr_id);
      return FATAL_ERROR;
    }
    if (arr_AppendArrPush(entity_layout->comp_masks, &parsing_extra->curr_mask,
                          NULL) == RESOURCE_EXHAUSTED) {
      LOG("Can't grow entity layout arrays anymore. Memory exhausted.");
      return FATAL_ERROR;
    }
    if (PadNonMandatoryComponents(parsing_extra->curr_mask, entity_layout) ==
        RESOURCE_EXHAUSTED) {
      return FATAL_ERROR;
    }
    snprintf(parsing_extra->curr_id, CHAR_BUFFER_SIZE, "%s", id);
    parsing_extra->curr_mask = 0;
  }

  if (CHARBUFF_EQUALS(key, "speed") &&
      !HAS_FLAG(parsing_extra->curr_mask, SPEED_COMPONENT)) {
    TYPEOF_SPEED_COMPONENT speed =
        (TYPEOF_SPEED_COMPONENT)strtoul(val, NULL, 10);
    TRY_PUSH(entity_layout->speeds, &speed, SPEED_COMPONENT,
             parsing_extra->curr_mask);
  } else if (CHARBUFF_EQUALS(key, "hp") &&
             !HAS_FLAG(parsing_extra->curr_mask, HP_COMPONENT)) {
    TYPEOF_HP_COMPONENT hp = (TYPEOF_HP_COMPONENT)strtoul(val, NULL, 10);
    TRY_PUSH(entity_layout->hps, &hp, HP_COMPONENT, parsing_extra->curr_mask);
  } else if (CHARBUFF_EQUALS(key, "asset_path") &&
             !HAS_FLAG(parsing_extra->curr_mask, ASSET_COMPONENT)) {
    // Special handling, needs to construct the texture here.
    TYPEOF_ASSET_COMPONENT asset = LoadTexture(val, parsing_extra->renderer);
    if (!asset) {
      LOG("Can't create asset for the entity layout");
      return FAILURE;
    }
    TRY_PUSH(entity_layout->assets, &asset, ASSET_COMPONENT,
             parsing_extra->curr_mask);
  } else if (CHARBUFF_EQUALS(key, "width") &&
             !HAS_FLAG(parsing_extra->curr_mask, WIDTH_COMPONENT)) {
    TYPEOF_DIMENSION_COMPONENT width =
        (TYPEOF_DIMENSION_COMPONENT)strtoul(val, NULL, 10);
    TRY_PUSH(entity_layout->widths, &width, WIDTH_COMPONENT,
             parsing_extra->curr_mask);
  } else if (CHARBUFF_EQUALS(key, "height") &&
             !HAS_FLAG(parsing_extra->curr_mask, HEIGHT_COMPONENT)) {
    TYPEOF_DIMENSION_COMPONENT height =
        (TYPEOF_DIMENSION_COMPONENT)strtoul(val, NULL, 10);
    TRY_PUSH(entity_layout->heights, &height, HEIGHT_COMPONENT,
             parsing_extra->curr_mask);
  }

  return SUCCESS;
}

static void ShrinkFitEntityLayout(EntityLayout *entity_layout) {
  /*
   * Realloc failure(which the following function call internally) is not a
   * concern. If this function is called, that means the data is already
   * defined. Shrinking is just a courtesy to save memory and if it fails its
   * not a big deal.
   */
  arr_AppendArrShrinkToFit(entity_layout->speeds);
  arr_AppendArrShrinkToFit(entity_layout->hps);
  arr_AppendArrShrinkToFit(entity_layout->assets);
  arr_AppendArrShrinkToFit(entity_layout->widths);
  arr_AppendArrShrinkToFit(entity_layout->heights);
  arr_AppendArrShrinkToFit(entity_layout->comp_masks);
}

EntityLayout *ent_CreateEntityLayout(const char *layout_path,
                                     SDL_Renderer *renderer) {
  EntityLayout *entity_layout;
  if (AllocateEntityLayout(&entity_layout) == RESOURCE_EXHAUSTED) {
    ent_DeleteEntityLayout(entity_layout);
    return NULL;
  }

  ParsingExtra extra = {.curr_id = "\0", .curr_mask = 0, .renderer = renderer};
  if (yaml_ParserParse(layout_path, ParseIntoEntityLayout, entity_layout,
                       &extra) == FAILURE) {
    ent_DeleteEntityLayout(entity_layout);
    return NULL;
  }
  if (arr_GetAppendArrLen(entity_layout->comp_masks) == 0) {
    LOG("No entity definitions in: %s", layout_path);
    ent_DeleteEntityLayout(entity_layout);
    return NULL;
  }

  ShrinkFitEntityLayout(entity_layout);

  return entity_layout;
}

void ent_DeleteEntityLayout(EntityLayout *entity_layout) {
  if (entity_layout) {
    arr_AppendArrDelete(entity_layout->speeds);
    arr_AppendArrDelete(entity_layout->hps);
    arr_AppendArrDelete(entity_layout->assets);
    arr_AppendArrDelete(entity_layout->widths);
    arr_AppendArrDelete(entity_layout->heights);
    arr_AppendArrDelete(entity_layout->comp_masks);

    arena_Dealloc(entity_layout, sizeof(EntityLayout));
  }
}

/* ----  ENTITY POOL  ---- */

static StatusCode AllocateEntityPool(EntityPool **pEntity_pool,
                                     size_t pool_capacity) {
  *pEntity_pool = arena_Alloc(sizeof(EntityPool));
  if (!(*pEntity_pool)) {
    LOG("Can't allocate memory for the entity pool.");
    return RESOURCE_EXHAUSTED;
  }

  EntityPool *entity_pool = *pEntity_pool;
  entity_pool->bboxes =
      arr_FlexArrCreate(sizeof(TYPEOF_BBOX_COMPONENT), pool_capacity);
  entity_pool->hps =
      arr_FlexArrCreate(sizeof(TYPEOF_HP_COMPONENT), pool_capacity);
  entity_pool->assets =
      arr_FlexArrCreate(sizeof(TYPEOF_ASSET_COMPONENT), pool_capacity);
  entity_pool->speeds =
      arr_FlexArrCreate(sizeof(TYPEOF_SPEED_COMPONENT), pool_capacity);
  entity_pool->comp_masks =
      arr_FlexArrCreate(sizeof(ComponentMasks), pool_capacity);

  // Sized create due to pre knowledge of max capacity.
  entity_pool->empty_slots =
      arr_SizedAppendArrCreate(sizeof(TYPEOF_SLOTS_ARRAY), pool_capacity);
  entity_pool->occupied_slots =
      arr_SizedAppendArrCreate(sizeof(TYPEOF_SLOTS_ARRAY), pool_capacity);

  if (!entity_pool->bboxes || !entity_pool->hps || !entity_pool->assets ||
      !entity_pool->speeds || !entity_pool->comp_masks ||
      !entity_pool->empty_slots || !entity_pool->occupied_slots) {
    LOG("Can't allocate memory for the entity pool.");
    return RESOURCE_EXHAUSTED;
  }

  return SUCCESS;
}

EntityPool *ent_CreateEntityPool(size_t pool_capacity) {
  EntityPool *entity_pool;
  if (AllocateEntityPool(&entity_pool, pool_capacity) == RESOURCE_EXHAUSTED) {
    ent_DeleteEntityPool(entity_pool);
    return NULL;
  }

  for (TYPEOF_SLOTS_ARRAY i = 0; i < pool_capacity; i++) {
    /*
     * Since both empty and occupied arrays were defined with the capacity
     * preset, this really can't every fail. Also with this, no need to shrink
     * fit.
     */
    arr_AppendArrPush(entity_pool->empty_slots, &i, NULL);
  }

  return entity_pool;
}

void ent_DeleteEntityPool(EntityPool *entity_pool) {
  if (entity_pool) {
    arr_FlexArrDelete(entity_pool->bboxes);
    arr_FlexArrDelete(entity_pool->hps);
    arr_FlexArrDelete(entity_pool->assets);
    arr_FlexArrDelete(entity_pool->speeds);
    arr_FlexArrDelete(entity_pool->comp_masks);

    arr_AppendArrDelete(entity_pool->empty_slots);
    arr_AppendArrDelete(entity_pool->occupied_slots);

    arena_Dealloc(entity_pool, sizeof(EntityPool));
  }
}

/* ----  ENTITY_MODULE  ---- */

EntityModule *ent_CreateEntityModule() {
  EntityModule *entity_module = arena_Alloc(sizeof(EntityModule));
  if (!entity_module) {
    LOG("Can't allocate memory for entity module.");
    return NULL;
  }

  return entity_module;
}

EntityModule *ent_CreateFullEntityModule(size_t pool_capacity,
                                         const char *layout_path,
                                         SDL_Renderer *renderer) {
  EntityModule *entity_module = ent_CreateEntityModule();
  if (!entity_module) {
    return NULL;
  }

  entity_module->entity_layout = ent_CreateEntityLayout(layout_path, renderer);
  if (!entity_module->entity_layout) {
    ent_DeleteEntityModule(entity_module);
    return NULL;
  }

  entity_module->entity_pool = ent_CreateEntityPool(pool_capacity);
  if (!entity_module->entity_pool) {
    ent_DeleteEntityModule(entity_module);
    return NULL;
  }

  return entity_module;
}

void ent_DeleteEntityModule(EntityModule *entity_module) {
  if (entity_module) {
    if (entity_module->entity_layout) {
      ent_DeleteEntityLayout(entity_module->entity_layout);
    }
    if (entity_module->entity_pool) {
      ent_DeleteEntityPool(entity_module->entity_pool);
    }
    arena_Dealloc(entity_module, sizeof(EntityModule));
  }
}

void ent_AttachEntityPoolToModule(EntityModule *entity_module,
                                  EntityPool *entity_pool) {
  assert(entity_module && entity_pool);
  entity_module->entity_pool = entity_pool;
}

void ent_AttachEntityLayoutToModule(EntityModule *entity_module,
                                    EntityLayout *entity_layout) {
  assert(entity_module && entity_layout);
  entity_module->entity_layout = entity_layout;
}

EntityPool *ent_DetachEntityPoolFromModule(EntityModule *entity_module) {
  assert(entity_module);
  EntityPool *entity_pool = entity_module->entity_pool;
  entity_module->entity_pool = NULL;

  return entity_pool;
}

EntityLayout *ent_DetachEntityLayoutFromModule(EntityModule *entity_module) {
  assert(entity_module);
  EntityLayout *entity_layout = entity_module->entity_layout;
  entity_module->entity_layout = NULL;

  return entity_layout;
}

/* ----  LOGIC  ---- */

StatusCode ent_SpawnEntityRandom(EntityModule *entity_module, float spawn_x,
                                 float spawn_y) {
  assert(entity_module);
  EntityPool *entity_pool = entity_module->entity_pool;
  EntityLayout *entity_layout = entity_module->entity_layout;

  size_t empty_spot_len = arr_GetAppendArrLen(entity_pool->empty_slots);

  if (empty_spot_len == 0) {
    LOG("Can't spawn any more entities, limit reached.");
    return CAN_NOT_EXECUTE;
  }

  size_t layout_len = arr_GetAppendArrLen(entity_layout->comp_masks);
  size_t rand_layout = rand() % layout_len;

  // Popping from empty index into occupied index.
  void *value =
      arr_GetAppendArrIndexValue(entity_pool->empty_slots, empty_spot_len - 1);
  arr_AppendArrPush(entity_pool->occupied_slots, value, NULL);
  size_t spawn_index = *((size_t *)value);
  arr_AppendArrPop(entity_pool->empty_slots);

  arr_SetFlexArrIndexValue(
      entity_pool->comp_masks,
      arr_GetAppendArrIndexValue(entity_layout->comp_masks, rand_layout),
      spawn_index);
  arr_SetFlexArrIndexValue(
      entity_pool->assets,
      arr_GetAppendArrIndexValue(entity_layout->assets, rand_layout),
      spawn_index);
  arr_SetFlexArrIndexValue(
      entity_pool->hps,
      arr_GetAppendArrIndexValue(entity_layout->hps, rand_layout), spawn_index);
  arr_SetFlexArrIndexValue(
      entity_pool->speeds,
      arr_GetAppendArrIndexValue(entity_layout->speeds, rand_layout),
      spawn_index);

  TYPEOF_BBOX_COMPONENT bbox = {
      .x = spawn_x,
      .y = spawn_y,
      .w = *((TYPEOF_DIMENSION_COMPONENT *)arr_GetAppendArrIndexValue(
          entity_layout->widths, rand_layout)),
      .h = *((TYPEOF_DIMENSION_COMPONENT *)arr_GetAppendArrIndexValue(
          entity_layout->heights, rand_layout))};
  arr_SetFlexArrIndexValue(entity_pool->bboxes, &bbox, spawn_index);

  return SUCCESS;
}

StatusCode ent_DespawnEntity(EntityModule *entity_module, uint32_t index) {
  assert(entity_module);
  EntityPool *entity_pool = entity_module->entity_pool;

  size_t entity_pool_cap = arr_GetFlexArrCapacity(entity_pool->comp_masks);
  ComponentMasks *comp_masks = arr_GetFlexArrRawData(entity_pool->comp_masks);

  if (entity_pool_cap < index || comp_masks[index] == 0) {
    LOG("Can't despawn a non-existing entity.");
    return CAN_NOT_EXECUTE;
  }

  comp_masks[index] = 0;
  // This can't really fail.
  arr_AppendArrPush(entity_pool->empty_slots, &index, NULL);

  TYPEOF_SLOTS_ARRAY *occupied_slots =
      arr_GetAppendArrRawData(entity_pool->occupied_slots);
  size_t occupied_slots_len = arr_GetAppendArrLen(entity_pool->occupied_slots);
  for (size_t i = 0; i < occupied_slots_len; i++) {
    if (occupied_slots[i] == index) {
      // Unordered array packing.
      occupied_slots[i] = occupied_slots[occupied_slots_len - 1];
      arr_AppendArrPop(entity_pool->occupied_slots);
      break;
    }
  }

  return SUCCESS;
}

static void InsertionSortWRTDimensionX(EntityPool *entity_pool) {
  /*
   * This is insertion sort. Due to the nature of how the entities will move,
   * i.e., Not that much per frame maintaining the almost-sorted order.
   * This means for about 95%(I Guess) times this insertion sort will be O(n)
   * time complexity, which is drastically better than even quick sort which
   * is O(n log(n)).
   */
  size_t occupied1_index, occupied2_index;

  // Since no removing/adding data is happening, using raw data is fine.
  TYPEOF_SLOTS_ARRAY *occupied_slots =
      arr_GetAppendArrRawData(entity_pool->occupied_slots);
  TYPEOF_BBOX_COMPONENT *bboxes = arr_GetFlexArrRawData(entity_pool->bboxes);
  size_t occupied_slots_len = arr_GetAppendArrLen(entity_pool->occupied_slots);

  for (size_t i = 1; i < occupied_slots_len; i++) {
    for (int64_t j = i - 1; j >= 0; j--) {
      occupied1_index = occupied_slots[j];
      occupied2_index = occupied_slots[j + 1];
      if (bboxes[occupied1_index].x < bboxes[occupied2_index].x)
        break;
      occupied_slots[j] = occupied2_index;
      occupied_slots[j + 1] = occupied1_index;
    }
  }
}

static void ResolveCollision(SDL_FRect *dimension1, SDL_FRect *dimension2) {
  /*
   * Due to the insertion sort we did wrt minimum x position the minimum
   * vector in the x direction is predetermined and we don't need to use a
   * ternary operator like in the y direction.
   */
  float overlap_x = (dimension1->x + dimension1->w) - dimension2->x;

  float dy1 = (dimension1->y + dimension1->h) - dimension2->y;
  float dy2 = (dimension2->y + dimension2->h) - dimension1->y;
  float overlap_y = (dy1 < dy2) ? dy1 : -dy2;

  float push_distance;

  if (fabsf(overlap_x) < fabsf(overlap_y)) {
    push_distance = overlap_x / 2.0;
    dimension1->x -= push_distance;
    dimension2->x += push_distance;
  } else {
    push_distance = overlap_y / 2.0;
    dimension1->y -= push_distance;
    dimension2->y += push_distance;
  }
}

void ent_HandleCollision(EntityModule *entity_module) {
  assert(entity_module);
  EntityPool *entity_pool = entity_module->entity_pool;

  InsertionSortWRTDimensionX(entity_pool);
  /*
   * Used in collision detection using a trick called:
   * Sort Sweep and Prune.
   * Reference Article: https://leanrada.com/notes/sweep-and-prune/
   */

  // Since no removing/adding data is happening, using raw data is fine.
  TYPEOF_SLOTS_ARRAY *occupied_slots =
      arr_GetAppendArrRawData(entity_pool->occupied_slots);
  TYPEOF_BBOX_COMPONENT *bboxes = arr_GetFlexArrRawData(entity_pool->bboxes);
  size_t occupied_slots_len = arr_GetAppendArrLen(entity_pool->occupied_slots);

  size_t occupied1_index, occupied2_index;
  for (size_t i = 0; i < occupied_slots_len; i++) {
    occupied1_index = occupied_slots[i];
    for (size_t j = i + 1; j < occupied_slots_len; j++) {
      occupied2_index = occupied_slots[j];
      /*
       * Not colliding in X so no way they are colliding so we just break out
       * of the loop. We can also do this as the array is sorted.
       */
      if (bboxes[occupied2_index].x >
          bboxes[occupied1_index].x + bboxes[occupied1_index].w) {
        break;
      }

      // Checking y collision and resolving it also.
      if ((bboxes[occupied1_index].y <
           bboxes[occupied2_index].y + bboxes[occupied2_index].h) &&
          (bboxes[occupied2_index].y <
           bboxes[occupied1_index].y + bboxes[occupied1_index].h)) {
        ResolveCollision(&bboxes[occupied1_index], &bboxes[occupied2_index]);
      }
    }
  }
}
