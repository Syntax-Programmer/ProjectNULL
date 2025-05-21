#pragma once

#include "../common.h"
#include <SDL2/SDL_rect.h>

#define PLAYER_INDEX 0
#define ENTITY_POOL_SIZE 51

typedef struct {
  int32_t curr, max, min;
} Meter;

typedef COMMON_PACKED_ENUM { NO_ENTITY, PLAYER, NPC } EntityType;

typedef struct IntArr {
  int32_t arr[ENTITY_POOL_SIZE];
  int32_t len;
} IntArr;

typedef struct {
  /*
  The 0th index is reserved for the player and shall not ever turn in the
  entity_empty_slots array.
  */
  SDL_FRect entity_bounding_boxes[ENTITY_POOL_SIZE];
  SDL_Color entity_colors[ENTITY_POOL_SIZE];
  EntityType entity_types[ENTITY_POOL_SIZE];
  float entity_speeds[ENTITY_POOL_SIZE];
  Meter entity_health_meters[ENTITY_POOL_SIZE];
  /*
  Indices which can new entities spawn into.
  This is a stack data structure of the empty indices.
  This follows the LIFO principle.
  */
  IntArr entity_empty_slots;
  /*
  Used to selectively access indices that are occupied
  Used in collision detection using a trick called:
  Sort Sweep and Prune.
  Reference Article: https://leanrada.com/notes/sweep-and-prune/
  */
  IntArr entity_occupied_slots;
} Entities;

void entity_CreateMeter(Meter *pMeter, int32_t max, int32_t *pCurr,
                        int32_t *pMin);
extern void entity_ChangeMeterMax(Meter *pMeter, int32_t new_max);
extern void entity_ChangeMeterMin(Meter *pMeter, int32_t new_min);
extern void entity_ChangeMeterCurr(Meter *pMeter, int32_t new_curr);

extern void entity_InitEntities(Entities *pEntities);

extern void entity_SpawnEntity(Entities *pEntities, EntityType type,
                               SDL_FRect dimension, SDL_Color color,
                               int32_t health_max, float base_speed);
extern void entity_DespawnEntity(Entities *pEntities, int32_t despawn_index);

void entity_HandleCollision(Entities *pEntities);
