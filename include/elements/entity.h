#pragma once

#include "../../include/utils/hashmap.h"
#include "../common.h"
#include <SDL2/SDL.h>

#define MAX_SPAWN_COUNT (48)

/*
 * This will have initial properties of the entities.
 * This will contain both constant and variable properties.
 *
 * Constant properties will remain in this struct enclosed. While the variable
 * properties will also be present in the Entities struct to reveal to other
 * modules.
 *`
 * An Id will link the element of Entities to this struct.
 */
typedef struct EntityProps EntityProps;

typedef struct {
  // This will also be the len of MAX_SPAWN_COUNT.
  StrIntHashmap *names;
  SDL_FRect bounding_boxes[MAX_SPAWN_COUNT];
  uint16_t hp[MAX_SPAWN_COUNT];
  struct {
    uint16_t index[MAX_SPAWN_COUNT];
    size_t len;
  } empty_slots, occupied_slots;
} Entities;

/*
 * #pragma once

 #include "../common.h"
 #include "../utility/arena.h"
 #include <SDL2/SDL.h>

 #define PLAYER_INDEX 0
 #define ENTITY_POOL_SIZE 51

 typedef struct {
   int32_t curr, max, min;
 } Entity_Meter;

 TODO: This needs to go after the introduction of yaml.
 typedef COMMON_PACKED_ENUM{NO_ENTITY, PLAYER, NPC} EntityType;

 typedef struct {
   int32_t arr[ENTITY_POOL_SIZE];
   size_t len;
 } Entity_SlotsArray;

 typedef struct {
   /*
   The 0th index is reserved for the player and shall not ever turn in the
   entity_empty_slots array.
   SDL_FRect bounding_boxes[ENTITY_POOL_SIZE];
   SDL_Color colors[ENTITY_POOL_SIZE];
   EntityType types[ENTITY_POOL_SIZE];
   float speeds[ENTITY_POOL_SIZE];
   // TODO: This is unused
   Entity_Meter health_meters[ENTITY_POOL_SIZE];
   /*
   Indices which can new entities spawn into.
   This is a stack data structure of the empty indices.
   This follows the LIFO principle.
   Entity_SlotsArray empty_slots;
   // Used to selectively access indices that are occupied.
   Entity_SlotsArray occupied_slots;
 } Entities;

 void entity_CreateMeter(Entity_Meter *pMeter, int32_t max, int32_t *pCurr,
                         int32_t *pMin);
 extern void entity_ChangeMeterMax(Entity_Meter *pMeter, int32_t new_max);
 extern void entity_ChangeMeterMin(Entity_Meter *pMeter, int32_t new_min);
 extern void entity_ChangeMeterCurr(Entity_Meter *pMeter, int32_t new_curr);

 extern bool entity_InitEntitiesHeap(Entities **pEntities);

 extern void entity_SpawnEntity(Entities *entities, EntityType type,
                                SDL_FRect dimension, SDL_Color color,
                                int32_t health_max, float base_speed);
 extern void entity_DespawnEntity(Entities *entities, int32_t despawn_index);

 extern void entity_HandleCollision(Entities *entities);

 */
