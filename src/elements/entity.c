#include "../../include/elements/entity.h"
#include "../../include/utils/arena.h"
#include "../../include/utils/yaml_parser.h"
#include <SDL2/SDL_image.h>

#define ENTITY_PROPERTIES_PATH ("gamedata/properties/entity_properties.yaml")
#define MIN_PROPERTIES_SLOTS (8)

/*
 * Used to efficiently determine which property of a certain entity is defined.
 * Using this allows us to no deal with string ids at all.
 * Once the flag variable has all of the following flags, we have determined
 * that its now fine to move forward. It can also stop from double definitions
 * that can corrupt the array size sync.
 */
typedef PACKED_ENUM{
    SPEED_DEFINED = 1 << 0,       HP_DEFINED = 1 << 1,
    INTRACTABLE_DEFINED = 1 << 2, ASSET_DEFINED = 1 << 3,
    WIDTH_DEFINED = 1 << 4,       HEIGHT_DEFINED = 1 << 5,
} DefinedFlags;

#define FULL_DEFINITION_MASK                                                   \
  (SPEED_DEFINED | HP_DEFINED | INTRACTABLE_DEFINED | ASSET_DEFINED |          \
   WIDTH_DEFINED | HEIGHT_DEFINED)

#define TRY_PUSH(array, val_ptr, flag_bit)                                     \
  do {                                                                         \
    if (arr_AppendArrayPush((array), (val_ptr), NULL) == RESOURCE_EXHAUSTED) { \
      LOG("Memory error during property assignment.");                         \
      return FAILURE;                                                          \
    }                                                                          \
    SET_FLAG(*pDefinition_flags, (flag_bit));                                  \
  } while (0)

static StatusCode AllocateInitialPropertyMemory(EntityProps *props);
static SDL_Texture *LoadTexture(const CharBuffer texture_path);
static StatusCode AllocateProperties(void *dest, const CharBuffer key,
                                     const CharBuffer val, const CharBuffer id,
                                     void *definition_flags);

static StatusCode AllocateInitialPropertyMemory(EntityProps *props) {
  props->speeds = arr_AppendArrayCreate(sizeof(uint16_t));
  props->hps = arr_AppendArrayCreate(sizeof(uint16_t));
  props->is_intractable = arr_AppendArrayCreate(sizeof(bool));
  props->assets = arr_AppendArrayCreate(sizeof(SDL_Texture *));
  props->widths = arr_AppendArrayCreate(sizeof(uint16_t));
  props->heights = arr_AppendArrayCreate(sizeof(uint16_t));

  if (!props->speeds || !props->hps || !props->is_intractable ||
      !props->assets || !props->widths || !props->heights) {
    LOG("Error allocating EntityProps struct.");
    return RESOURCE_EXHAUSTED;
  }

  return SUCCESS;
}

static SDL_Texture *LoadTexture(const CharBuffer texture_path) {
  SDL_Surface *surface = IMG_Load(texture_path);
  if (!surface) {
    LOG("IMG_Load Error: %s", IMG_GetError());
    return NULL;
  }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(common_renderer, surface);
  if (!texture) {
    LOG("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
    SDL_FreeSurface(surface);
    return NULL;
  }

  SDL_FreeSurface(surface);
  return texture;
}

static StatusCode AllocateProperties(void *dest, const CharBuffer key,
                                     const CharBuffer val, const CharBuffer id,
                                     void *definition_flags) {
  (void)id;

  EntityProps *props = dest;
  DefinedFlags *pDefinition_flags = definition_flags;

  if (HAS_FLAG(*pDefinition_flags, FULL_DEFINITION_MASK)) {
    *pDefinition_flags = 0;
  }

  if (CHAR_BUFFER_IS(key, "speed") &&
      !HAS_FLAG(*pDefinition_flags, SPEED_DEFINED)) {
    uint16_t speed = atoi(val);
    TRY_PUSH(props->speeds, &speed, SPEED_DEFINED);
  } else if (CHAR_BUFFER_IS(key, "hp") &&
             !HAS_FLAG(*pDefinition_flags, HP_DEFINED)) {
    uint16_t hp = atoi(val);
    TRY_PUSH(props->hps, &hp, HP_DEFINED);
  } else if (CHAR_BUFFER_IS(key, "is_intractable") &&
             !HAS_FLAG(*pDefinition_flags, INTRACTABLE_DEFINED)) {
    bool is_intractable = STR_TO_BOOL(val);
    TRY_PUSH(props->is_intractable, &is_intractable, INTRACTABLE_DEFINED);
  } else if (CHAR_BUFFER_IS(key, "asset_path") &&
             !HAS_FLAG(*pDefinition_flags, ASSET_DEFINED)) {
    // Special handling, needs to construct the texture here.
    SDL_Texture *asset = LoadTexture(val);
    if (!asset) {
      LOG("Can't create asset for the entity props");
      return FAILURE;
    }
    TRY_PUSH(props->assets, &asset, ASSET_DEFINED);
  } else if (CHAR_BUFFER_IS(key, "width") &&
             !HAS_FLAG(*pDefinition_flags, WIDTH_DEFINED)) {
    uint16_t width = atoi(val);
    TRY_PUSH(props->widths, &width, WIDTH_DEFINED);
  } else if (CHAR_BUFFER_IS(key, "height") &&
             !HAS_FLAG(*pDefinition_flags, HEIGHT_DEFINED)) {
    uint16_t height = atoi(val);
    TRY_PUSH(props->heights, &height, HEIGHT_DEFINED);
  }

  return SUCCESS;
}

static void ShrinkFitProps(EntityProps *props) {
  arr_AppendArrayShrinkToFit(props->speeds);
  arr_AppendArrayShrinkToFit(props->hps);
  arr_AppendArrayShrinkToFit(props->is_intractable);
  arr_AppendArrayShrinkToFit(props->assets);
  arr_AppendArrayShrinkToFit(props->widths);
  arr_AppendArrayShrinkToFit(props->heights);
}

StatusCode entity_InitProperties(EntityProps **pProps) {
  *pProps = (EntityProps *)arena_Alloc(sizeof(EntityProps));
  if (!*pProps) {
    LOG("Error originated from entity_InitProperties.");
    return FATAL_ERROR;
  }
  CharBuffer prev_id;

  if (AllocateInitialPropertyMemory(*pProps) == RESOURCE_EXHAUSTED) {
    entity_DeleteProperties(pProps);
    return FATAL_ERROR;
  }

  if (yaml_ParserParse(ENTITY_PROPERTIES_PATH, AllocateProperties, *pProps,
                       prev_id) == FAILURE) {
    entity_DeleteProperties(pProps);
    return FATAL_ERROR;
  }

  ShrinkFitProps(*pProps);

  return SUCCESS;
}

void entity_DeleteProperties(EntityProps **pProps) {
  if (pProps && *pProps) {
    arr_AppendArrayDelete((*pProps)->speeds);
    arr_AppendArrayDelete((*pProps)->hps);
    arr_AppendArrayDelete((*pProps)->is_intractable);
    arr_AppendArrayDelete((*pProps)->assets);
    arr_AppendArrayDelete((*pProps)->widths);
    arr_AppendArrayDelete((*pProps)->heights);
    (*pProps)->speeds = NULL;
    (*pProps)->hps = NULL;
    (*pProps)->is_intractable = NULL;
    (*pProps)->assets = NULL;
    (*pProps)->widths = NULL;
    (*pProps)->heights = NULL;
    arena_Dealloc(*pProps, sizeof(EntityProps));
    *pProps = NULL;
  }
}

void entity_DumpProperties(EntityProps *props) {
  uint16_t *speeds = arr_GetAppendArrayRawData(props->speeds);
  uint16_t *hps = arr_GetAppendArrayRawData(props->hps);
  bool *is_intractable = arr_GetAppendArrayRawData(props->is_intractable);
  SDL_Texture **asset = arr_GetAppendArrayRawData(props->assets);
  uint16_t *widhts = arr_GetAppendArrayRawData(props->widths);
  uint16_t *heights = arr_GetAppendArrayRawData(props->heights);

  for (size_t i = 0; i < arr_GetAppendArrayLen(props->speeds); i++) {
    printf("\n\nEntity Number: %zu\n", i + 1);
    printf("Speed: %d\n", speeds[i]);
    printf("HP: %d\n", hps[i]);
    printf("Is intractable: %d\n", is_intractable[i]);
    printf("Asset: %p\n", asset[i]);
    printf("Width: %d\n", widhts[i]);
    printf("Height: %d\n", heights[i]);
  }
}

/*
 * #include "../../include/elements/entities.h"
 #include "../../include/utility/yaml_parser.h"

 #define ENTITY_PROPERTIES_MIN_SIZE (8)
 #define BOUNDING_BOXES_PROPS_MIN_SIZE \ (sizeof(SDL_FRect) *
 ENTITY_PROPERTIES_MIN_SIZE) #define SPEEDS_PROPS_MIN_SIZE (sizeof(float) *
 ENTITY_PROPERTIES_MIN_SIZE)
 #define ENTITY_IDS_PROPS_MIN_SIZE \ (sizeof(char) * CHAR_BUFFER_SIZE
 * ENTITY_PROPERTIES_MIN_SIZE)
 #define ASSET_PATHS_PROPS_MIN_SIZE \ (sizeof(char) *
 CHAR_BUFFER_SIZE * ENTITY_PROPERTIES_MIN_SIZE)
 #define ASSETS_PROPS_MIN_SIZE \ (sizeof(SDL_Texture *) *
 ENTITY_PROPERTIES_MIN_SIZE)

 #define ENTITY_PROPERTIES_PATH
 ("gamedata/properties/entity_properties.yaml")

 typedef struct {
   SDL_FRect *bounding_boxes;
   float *speeds;
   char **entity_ids;
   char **asset_paths;
   SDL_Texture **assets;
   size_t size;
   size_t len;
 } EntityProperties;

 static EntityProperties entity_props;

 These are just the initial default conditions and could change as needed
 later. #define PLAYER_INITIAL_SPEED 150
 #define PLAYER_INITIAL_HEALTH(pPlayer_health_meter) \
   (entity_CreateMeter(pPlayer_health_meter, 100, NULL, NULL))
 #define PLAYER_INITIAL_DIMENSION {0, 0, 50, 50}
 #define PLAYER_COLOR {BLUISH}
 // #define PLAYER_INITIAL_PHYSICAL_DATA (entity_CreatePhysicalData(1))

 static bool InitEntityProps();
 static bool EntityPropertyAllocator(void *dest, const char *key,
                                     const char *val, const char *id);

 static void InitEntities(Entities *entities);

 static void InsertionSortWRTDimensionX(Entities *entities);
 static void ResolveCollision(SDL_FRect *dimension1, SDL_FRect *dimension2);

 void entity_CreateMeter(Entity_Meter *pMeter, int32_t max, int32_t *pCurr,
                         int32_t *pMin) {
   // If the pCurr is Null means that by default the meter will be full.
   // Same if no min value is given. by default its 0.
   *pMeter = (Entity_Meter){
       .max = max, .curr = (pCurr) ? *pCurr : max, .min = (pMin) ? *pMin :
 0};
 }

 void entity_ChangeMeterMax(Entity_Meter *pMeter, int32_t new_max) {
   if (pMeter->min >= new_max) {
 #ifdef DEBUG
     fprintf(stderr, "Can't set the new maximum to a meter as/lower than the
 " "min value.\n"); #endif return;
   }
   pMeter->max = new_max;
   if (pMeter->curr >= new_max) {
     // Clamping the curr value to the max
     pMeter->curr = new_max;
   }
 }

 void entity_ChangeMeterMin(Entity_Meter *pMeter, int32_t new_min) {
   if (new_min >= pMeter->max) {
 #ifdef DEBUG
     fprintf(stderr, "Can't set the new minimum to a meter as/higher than
 the " "max value.\n"); #endif return;
   }
   pMeter->min = new_min;
   if (new_min >= pMeter->curr) {
     pMeter->curr = new_min;
   }
 }

 void entity_ChangeMeterCurr(Entity_Meter *pMeter, int32_t new_curr) {
   if (new_curr >= pMeter->max || new_curr <= pMeter->min) {
 #ifdef DEBUG
     fprintf(stderr, "Can't set the current value of a meter beyond the
 defined " "min and max range.\n"); #endif return;
   }
   pMeter->curr = new_curr;
 }

 static bool InitEntityProps() {
   size_t bounding_boxes_offset =
 arena_AllocData(BOUNDING_BOXES_PROPS_MIN_SIZE), speeds_offset =
 arena_AllocData(SPEEDS_PROPS_MIN_SIZE), entity_ids_offset =
 arena_AllocData(ENTITY_IDS_PROPS_MIN_SIZE), asset_paths_offset =
 arena_AllocData(ASSET_PATHS_PROPS_MIN_SIZE), assets_offset =
 arena_AllocData(ASSETS_PROPS_MIN_SIZE);

   if (bounding_boxes_offset == INVALID_OFFSET ||
       speeds_offset == INVALID_OFFSET || entity_ids_offset ==
 INVALID_OFFSET || asset_paths_offset == INVALID_OFFSET || assets_offset ==
 INVALID_OFFSET) {
     // Freeing up corrupted data. This is a crashing state for the program.
     arena_ReallocData(bounding_boxes_offset, BOUNDING_BOXES_PROPS_MIN_SIZE,
 0); arena_ReallocData(speeds_offset, SPEEDS_PROPS_MIN_SIZE, 0);
     arena_ReallocData(entity_ids_offset, ENTITY_IDS_PROPS_MIN_SIZE, 0);
     arena_ReallocData(asset_paths_offset, ASSET_PATHS_PROPS_MIN_SIZE, 0);
     arena_ReallocData(assets_offset, ASSETS_PROPS_MIN_SIZE, 0);
     return false;
   }

   entity_props.bounding_boxes = (SDL_FRect *)arena_FetchData(
       bounding_boxes_offset, BOUNDING_BOXES_PROPS_MIN_SIZE);
   entity_props.speeds =
       (float *)arena_FetchData(speeds_offset, SPEEDS_PROPS_MIN_SIZE);
   entity_props.entity_ids =
       (char **)arena_FetchData(entity_ids_offset,
 ENTITY_IDS_PROPS_MIN_SIZE); entity_props.asset_paths = (char
 **)arena_FetchData(asset_paths_offset, ASSET_PATHS_PROPS_MIN_SIZE);
   entity_props.assets =
       (SDL_Texture **)arena_FetchData(assets_offset,
 ASSETS_PROPS_MIN_SIZE);

   entity_props.size = ENTITY_PROPERTIES_MIN_SIZE;
   entity_props.len = 0;

   return true;
 }

 static bool EntityPropertyAllocator(void *dest, const char *key,
                                     const char *val, const char *id) {
   MATCH_CHAR_BUFFER(key, "speed") { return true; }
   else MATCH_CHAR_BUFFER(key, "bounding_box") {
     return true;
   }
   else MATCH_CHAR_BUFFER(key, "asset_path") {
     return true;
   }
   else {
     return false;
   }
 }

 static void InitEntities(Entities *entities) {
   entities->bounding_boxes[PLAYER_INDEX] =
 (SDL_FRect)PLAYER_INITIAL_DIMENSION; entities->colors[PLAYER_INDEX] =
 (SDL_Color)PLAYER_COLOR;
   PLAYER_INITIAL_HEALTH(&entities->health_meters[PLAYER_INDEX]);
   entities->speeds[PLAYER_INDEX] = PLAYER_INITIAL_SPEED;
   entities->types[PLAYER_INDEX] = PLAYER;

   for (int32_t i = PLAYER_INDEX + 1; i < ENTITY_POOL_SIZE; i++) {
     /*
     This ensure the the entity_empty_slots is in the order {n, n-1, ...,
 1}; So that when time comes to populate, the initial indices get populated
     first.
     entities->empty_slots.arr[i] = ENTITY_POOL_SIZE - 1 - i;
     entities->types[i] = NO_ENTITY;
   }
   entities->empty_slots.len = ENTITY_POOL_SIZE - 1;

   entities->occupied_slots.len = 1;
   entities->occupied_slots.arr[0] = PLAYER_INDEX;
 }

 bool entity_InitEntitiesHeap(Entities **pEntities) {
   *pEntities = NULL;
   size_t entities_heap_offset = arena_AllocData(sizeof(Entities));

   if (entities_heap_offset == INVALID_OFFSET) {
 #ifdef DEBUG
     fprintf(stderr, "This condition shall never be reached unless the "
                     "DEFAULT_ARENA_SIZE is set too small. Originating from
 " "entity_InitEntities.\n"); #endif return false;
   }

   *pEntities =
       (Entities *)arena_FetchData(entities_heap_offset, sizeof(Entities));
   InitEntities(*pEntities);

   return true;
 }

 void entity_SpawnEntity(Entities *entities, EntityType type,
                         SDL_FRect dimension, SDL_Color color,
                         int32_t health_max, float speed) {
   if (type == PLAYER) {
 #ifdef DEBUG
     fprintf(stderr, "Can't spawn a player.\n");
 #endif
     return;
   }
   if (!entities->empty_slots.len) {
 #ifdef DEBUG
     fprintf(stderr, "No free space to spawn a new entity.\n");
 #endif
     return;
   }

   // Pop a free index from the empty pool
   uint32_t selected_index =
       entities->empty_slots.arr[--entities->empty_slots.len];
   // Push the selected index into the occupied list
   entities->occupied_slots.arr[entities->occupied_slots.len++] =
 selected_index;

   entities->types[selected_index] = type;
   entities->bounding_boxes[selected_index] = dimension;
   entities->speeds[selected_index] = speed;
   entities->colors[selected_index] = color;
   entity_CreateMeter(&entities->health_meters[selected_index], health_max,
 NULL, NULL);
 }

 void entity_DespawnEntity(Entities *entities, int32_t despawn_index) {
   if (entities->types[despawn_index] == NO_ENTITY) {
 #ifdef DEBUG
     fprintf(stderr, "Can't despawn an entity that doesn't exist.\n");
 #endif
     return;
   }
   if (despawn_index == PLAYER_INDEX) {
 #ifdef DEBUG
     fprintf(stderr, "Can't despawn the player man.\n");
 #endif
     return;
   }

   entities->types[despawn_index] = NO_ENTITY;
   entities->empty_slots.arr[entities->empty_slots.len++] = despawn_index;

   for (size_t i = 0; i < entities->occupied_slots.len; i++) {
     if (entities->occupied_slots.arr[i] == despawn_index) {
       /*
       Shifting the last element of the arr to the now freed up index, this
 is an efficient way to resize the array without shifting the elements in
       the array.
       entities->occupied_slots.arr[i] =
           entities->occupied_slots.arr[--entities->occupied_slots.len];
       break;
     }
   }
 }

 static void InsertionSortWRTDimensionX(Entities *entities) {
   /*
   This is insertion sort. Due to the nature of how the entities will move,
   i.e., Not that much per frame maintaining the almost-sorted order.
   This means for about 95%(I Guess) times this insertion sort will be O(n)
   time complexity, which is drastically better than even quick sort which
 is O(n log(n)). int32_t occupied1_index, occupied2_index;

   for (size_t i = 1; i < entities->occupied_slots.len; i++) {
     for (size_t j = i - 1; j >= 0; j--) {
       occupied1_index = entities->occupied_slots.arr[j];
       occupied2_index = entities->occupied_slots.arr[j + 1];
       if (entities->bounding_boxes[occupied1_index].x <
           entities->bounding_boxes[occupied2_index].x)
         break;
       entities->occupied_slots.arr[j] = occupied2_index;
       entities->occupied_slots.arr[j + 1] = occupied1_index;
     }
   }
 }

 static void ResolveCollision(SDL_FRect *dimension1, SDL_FRect *dimension2)
 {
   /*
   Due to the insertion sort we did wrt minimum x position the minimum
 vector in the x direction is predetermined and we don't need to use a
 ternary operator like in the y direction. float overlap_x = (dimension1->x
 + dimension1->w) - dimension2->x;

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

 void entity_HandleCollision(Entities *entities) {
   InsertionSortWRTDimensionX(entities);
   /*
     Used in collision detection using a trick called:
     Sort Sweep and Prune.
     Reference Article: https://leanrada.com/notes/sweep-and-prune/
   int32_t occupied1_index, occupied2_index;
   for (size_t i = 0; i < entities->occupied_slots.len; i++) {
     occupied1_index = entities->occupied_slots.arr[i];
     for (size_t j = i + 1; j < entities->occupied_slots.len; j++) {
       occupied2_index = entities->occupied_slots.arr[j];

       /*
       Not colliding in X so no way they are colliding so we just break out
 of the loop. We can also do this as the array is sorted. if
 (entities->bounding_boxes[occupied2_index].x >
           entities->bounding_boxes[occupied1_index].x +
               entities->bounding_boxes[occupied1_index].w) {
         break;
       }

       // Checking y collision and resolving it also.
       if ((entities->bounding_boxes[occupied1_index].y <
            entities->bounding_boxes[occupied2_index].y +
                entities->bounding_boxes[occupied2_index].h) &&
           (entities->bounding_boxes[occupied2_index].y <
            entities->bounding_boxes[occupied1_index].y +
                entities->bounding_boxes[occupied1_index].h)) {
         ResolveCollision(&entities->bounding_boxes[occupied1_index],
                          &entities->bounding_boxes[occupied2_index]);
       }
     }
   }
 }

 */
