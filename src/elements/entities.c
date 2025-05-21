#include "../../include/elements/entities.h"

/*
These are just the initial default conditions and could change as needed later.
*/
#define PLAYER_INITIAL_SPEED 150
#define PLAYER_INITIAL_HEALTH(pPlayer_health_meter)                            \
  (entity_CreateMeter(pPlayer_health_meter, 100, NULL, NULL))
#define PLAYER_INITIAL_DIMENSION {0, 0, 50, 50}
#define PLAYER_COLOR {BLUISH}
// #define PLAYER_INITIAL_PHYSICAL_DATA (entity_CreatePhysicalData(1))

static void InsertionSortWRTDimensionX(Entities *pEntities);
static void ResolveCollision(SDL_FRect *dimension1, SDL_FRect *dimension2);

void entity_CreateMeter(Meter *pMeter, int32_t max, int32_t *pCurr,
                        int32_t *pMin) {
  // If the pCurr is Null means that by default the meter will be full.
  // Same if no min value is given. by default its 0.
  *pMeter = (Meter){
      .max = max, .curr = (pCurr) ? *pCurr : max, .min = (pMin) ? *pMin : 0};
}

void entity_ChangeMeterMax(Meter *pMeter, int32_t new_max) {
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

void entity_ChangeMeterMin(Meter *pMeter, int32_t new_min) {
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

void entity_ChangeMeterCurr(Meter *pMeter, int32_t new_curr) {
  if (new_curr >= pMeter->max || new_curr <= pMeter->min) {
#ifdef DEBUG
    fprintf(stderr, "Can't set the current value of a meter beyond the defined "
                    "min and max range.\n");
#endif
    return;
  }
  pMeter->curr = new_curr;
}

void entity_InitEntities(Entities *pEntities) {
  pEntities->entity_bounding_boxes[PLAYER_INDEX] =
      (SDL_FRect)PLAYER_INITIAL_DIMENSION;
  pEntities->entity_colors[PLAYER_INDEX] = (SDL_Color)PLAYER_COLOR;
  PLAYER_INITIAL_HEALTH(&pEntities->entity_health_meters[PLAYER_INDEX]);
  pEntities->entity_speeds[PLAYER_INDEX] = PLAYER_INITIAL_SPEED;
  pEntities->entity_types[PLAYER_INDEX] = PLAYER;

  for (int32_t i = PLAYER_INDEX + 1; i < ENTITY_POOL_SIZE; i++) {
    /*
    This ensure the the entity_empty_slots is in the order {n, n-1, ..., 1};
    So that when time comes to populate, the initial indices get populated
    first.
    */
    pEntities->entity_empty_slots.arr[i] = ENTITY_POOL_SIZE - 1 - i;
    pEntities->entity_types[i] = NO_ENTITY;
  }
  pEntities->entity_empty_slots.len = ENTITY_POOL_SIZE - 1;

  pEntities->entity_occupied_slots.len = 1;
  pEntities->entity_occupied_slots.arr[0] = PLAYER_INDEX;
}

void entity_SpawnEntity(Entities *pEntities, EntityType type,
                        SDL_FRect dimension, SDL_Color color,
                        int32_t health_max, float speed) {
  if (type == PLAYER) {
#ifdef DEBUG
    fprintf(stderr, "Can't spawn a player.\n");
#endif
    return;
  }
  if (!pEntities->entity_empty_slots.len) {
#ifdef DEBUG
    fprintf(stderr, "No free space to spawn a new entity.\n");
#endif
    return;
  }
  // Pop a free index from the empty pool
  uint32_t selected_index =
      pEntities->entity_empty_slots.arr[--pEntities->entity_empty_slots.len];
  // Push the selected index into the occupied list
  pEntities->entity_occupied_slots.arr[pEntities->entity_occupied_slots.len++] =
      selected_index;

  pEntities->entity_types[selected_index] = type;
  pEntities->entity_bounding_boxes[selected_index] = dimension;
  pEntities->entity_speeds[selected_index] = speed;
  pEntities->entity_colors[selected_index] = color;
  entity_CreateMeter(&pEntities->entity_health_meters[selected_index],
                     health_max, NULL, NULL);
}

void entity_DespawnEntity(Entities *pEntities, int32_t despawn_index) {
  if (pEntities->entity_types[despawn_index] == NO_ENTITY) {
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

  pEntities->entity_types[despawn_index] = NO_ENTITY;
  pEntities->entity_empty_slots.arr[pEntities->entity_empty_slots.len++] =
      despawn_index;

  for (int32_t i = 0; i < pEntities->entity_occupied_slots.len; i++) {
    if (pEntities->entity_occupied_slots.arr[i] == despawn_index) {
      /*
      Shifting the last element of the arr to the now freed up index, this is an
      efficient way to resize the array without shifting the elements in the
      array.
      */
      pEntities->entity_occupied_slots.arr[i] =
          pEntities->entity_occupied_slots
              .arr[--pEntities->entity_occupied_slots.len];
      break;
    }
  }
}

static void InsertionSortWRTDimensionX(Entities *pEntities) {
  /*
  This is insertion sort. Due to the nature of how the entities will move,
  i.e., Not that much per frame maintaining the almost-sorted order.
  This means for about 95%(I Guess) times this insertion sort will be O(n)
  time complexity, which is drastically better than even quick sort which is O(n
  log(n)).
  */
  int32_t occupied1_index, occupied2_index;

  for (int32_t i = 1; i < pEntities->entity_occupied_slots.len; i++) {
    for (int32_t j = i - 1; j >= 0; j--) {
      occupied1_index = pEntities->entity_occupied_slots.arr[j];
      occupied2_index = pEntities->entity_occupied_slots.arr[j + 1];
      if (pEntities->entity_bounding_boxes[occupied1_index].x <
          pEntities->entity_bounding_boxes[occupied2_index].x)
        break;
      pEntities->entity_occupied_slots.arr[j] = occupied2_index;
      pEntities->entity_occupied_slots.arr[j + 1] = occupied1_index;
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

void entity_HandleCollision(Entities *pEntities) {
  InsertionSortWRTDimensionX(pEntities);

  /*
    Used in collision detection using a trick called:
    Sort Sweep and Prune.
    Reference Article: https://leanrada.com/notes/sweep-and-prune/
  */
  int32_t occupied1_index, occupied2_index;
  for (int32_t i = 0; i < pEntities->entity_occupied_slots.len; i++) {
    occupied1_index = pEntities->entity_occupied_slots.arr[i];
    for (int32_t j = i + 1; j < pEntities->entity_occupied_slots.len; j++) {
      occupied2_index = pEntities->entity_occupied_slots.arr[j];

      /*
      Not colliding in X so no way they are colliding so we just break out of
      the loop. We can also do this as the array is sorted.
      */
      if (pEntities->entity_bounding_boxes[occupied2_index].x >
          pEntities->entity_bounding_boxes[occupied1_index].x +
              pEntities->entity_bounding_boxes[occupied1_index].w) {
        break;
      }

      // Checking y collisiong and resolving it also.
      if ((pEntities->entity_bounding_boxes[occupied1_index].y <
           pEntities->entity_bounding_boxes[occupied2_index].y +
               pEntities->entity_bounding_boxes[occupied2_index].h) &&
          (pEntities->entity_bounding_boxes[occupied2_index].y <
           pEntities->entity_bounding_boxes[occupied1_index].y +
               pEntities->entity_bounding_boxes[occupied1_index].h)) {
        ResolveCollision(&pEntities->entity_bounding_boxes[occupied1_index],
                         &pEntities->entity_bounding_boxes[occupied2_index]);
      }
    }
  }
}
