#include "../../include/elements/entities.h"

typedef struct {
  SDL_FRect *bounding_boxes;
  float *speeds;
  char **entity_ids;
  char **asset_paths;
  SDL_Texture **assets;
} EntityProperties;

/*
These are just the initial default conditions and could change as needed later.
*/
#define PLAYER_INITIAL_SPEED 150
#define PLAYER_INITIAL_HEALTH(pPlayer_health_meter)                            \
  (entity_CreateMeter(pPlayer_health_meter, 100, NULL, NULL))
#define PLAYER_INITIAL_DIMENSION {0, 0, 50, 50}
#define PLAYER_COLOR {BLUISH}
// #define PLAYER_INITIAL_PHYSICAL_DATA (entity_CreatePhysicalData(1))

static Entities InitEntities();

static void InsertionSortWRTDimensionX(Entities *entities);
static void ResolveCollision(SDL_FRect *dimension1, SDL_FRect *dimension2);

void entity_CreateMeter(Entity_Meter *pMeter, int32_t max, int32_t *pCurr,
                        int32_t *pMin) {
  // If the pCurr is Null means that by default the meter will be full.
  // Same if no min value is given. by default its 0.
  *pMeter = (Entity_Meter){
      .max = max, .curr = (pCurr) ? *pCurr : max, .min = (pMin) ? *pMin : 0};
}

void entity_ChangeMeterMax(Entity_Meter *pMeter, int32_t new_max) {
  if (pMeter->min >= new_max) {
#ifdef DEBUG
    fprintf(stderr, "Can't set the new maximum to a meter as/lower than the "
                    "min value.\n");
#endif
    return;
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
    fprintf(stderr, "Can't set the new minimum to a meter as/higher than the "
                    "max value.\n");
#endif
    return;
  }
  pMeter->min = new_min;
  if (new_min >= pMeter->curr) {
    pMeter->curr = new_min;
  }
}

void entity_ChangeMeterCurr(Entity_Meter *pMeter, int32_t new_curr) {
  if (new_curr >= pMeter->max || new_curr <= pMeter->min) {
#ifdef DEBUG
    fprintf(stderr, "Can't set the current value of a meter beyond the defined "
                    "min and max range.\n");
#endif
    return;
  }
  pMeter->curr = new_curr;
}

static Entities InitEntities() {
  Entities entities = {0};

  entities.bounding_boxes[PLAYER_INDEX] = (SDL_FRect)PLAYER_INITIAL_DIMENSION;
  entities.colors[PLAYER_INDEX] = (SDL_Color)PLAYER_COLOR;
  PLAYER_INITIAL_HEALTH(&entities.health_meters[PLAYER_INDEX]);
  entities.speeds[PLAYER_INDEX] = PLAYER_INITIAL_SPEED;
  entities.types[PLAYER_INDEX] = PLAYER;

  for (int32_t i = PLAYER_INDEX + 1; i < ENTITY_POOL_SIZE; i++) {
    /*
    This ensure the the entity_empty_slots is in the order {n, n-1, ..., 1};
    So that when time comes to populate, the initial indices get populated
    first.
    */
    entities.empty_slots.arr[i] = ENTITY_POOL_SIZE - 1 - i;
    entities.types[i] = NO_ENTITY;
  }
  entities.empty_slots.len = ENTITY_POOL_SIZE - 1;

  entities.occupied_slots.len = 1;
  entities.occupied_slots.arr[0] = PLAYER_INDEX;

  return entities;
}

bool entity_InitEntitiesHeap(Entities **pEntities) {
  *pEntities = NULL;
  size_t entities_heap_offset = arena_AllocData(sizeof(Entities));

  if (entities_heap_offset == (size_t)-1) {
#ifdef DEBUG
    fprintf(stderr, "This condition shall never be reached unless the "
                    "DEFAULT_ARENA_SIZE is set too small. Originating from "
                    "entity_InitEntities.\n");
#endif
    return false;
  }

  Entities entities = InitEntities();

  if (!arena_SetData((uint8_t *)(&entities), entities_heap_offset,
                      sizeof(Entities))) {
#ifdef DEBUG
    fprintf(stderr, "This condition shall never be reached unless the "
                    "DEFAULT_ARENA_SIZE is set too small. Originating from "
                    "entity_InitEntities.\n");
#endif
    return false;
  }
  *pEntities =
      (Entities *)arena_FetchData(entities_heap_offset, sizeof(Entities));

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

  for (int32_t i = 0; i < entities->occupied_slots.len; i++) {
    if (entities->occupied_slots.arr[i] == despawn_index) {
      /*
      Shifting the last element of the arr to the now freed up index, this is
      an efficient way to resize the array without shifting the elements in
      the array.
      */
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
  time complexity, which is drastically better than even quick sort which is O(n
  log(n)).
  */
  int32_t occupied1_index, occupied2_index;

  for (int32_t i = 1; i < entities->occupied_slots.len; i++) {
    for (int32_t j = i - 1; j >= 0; j--) {
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

static void ResolveCollision(SDL_FRect *dimension1, SDL_FRect *dimension2) {
  /*
  Due to the insertion sort we did wrt minimum x position the minimum vector in
  the x direction is predetermined and we don't need to use a ternary operator
  like in the y direction.
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

void entity_HandleCollision(Entities *entities) {
  InsertionSortWRTDimensionX(entities);
  /*
    Used in collision detection using a trick called:
    Sort Sweep and Prune.
    Reference Article: https://leanrada.com/notes/sweep-and-prune/
  */
  int32_t occupied1_index, occupied2_index;
  for (int32_t i = 0; i < entities->occupied_slots.len; i++) {
    occupied1_index = entities->occupied_slots.arr[i];
    for (int32_t j = i + 1; j < entities->occupied_slots.len; j++) {
      occupied2_index = entities->occupied_slots.arr[j];

      /*
      Not colliding in X so no way they are colliding so we just break out of
      the loop. We can also do this as the array is sorted.
      */
      if (entities->bounding_boxes[occupied2_index].x >
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
