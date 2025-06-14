#pragma once

#include "../../include/utils/array.h"
#include "../common.h"
#include <SDL2/SDL.h>

#define MAX_SPAWN_COUNT (48)

/*
 * These are immutable properties of the every type of entity there can be.
 * After the initialization phase they can never change.
 */
typedef struct {
  AppendArray *speeds;         // uint16_t array
  AppendArray *hps;            // uint16_t array
  AppendArray *is_intractable; // bool array
  AppendArray *assets;         // SDL_Texture* array
  AppendArray *widths;         // uint16_t array
  AppendArray *heights;        // uint16_t array
  // void (**on_interact)(char name[DEFAULT_CHAR_BUFF_SIZE]);
} EntityProps;

typedef struct {
  /*
   * NOTE: Since it is a fixed size buffer, it doesn't make sense to use dynamic
   * array definitions. So using plain arrays instead.
   */
  /*
   * During spawning once we can do a lookup of the index into the
   * props struct, and use the index each time we need data from the props
   * struct.
   */
  uint32_t props_index[MAX_SPAWN_COUNT];
  SDL_FRect bounding_boxes[MAX_SPAWN_COUNT];
  uint16_t hps[MAX_SPAWN_COUNT];
  bool is_moving[MAX_SPAWN_COUNT];
  // Sort of a stack data structure.
  struct {
    // MAX_SPAWN_COUNT, shall very rarely exceed 255.
    uint8_t index[MAX_SPAWN_COUNT];
    uint8_t len;
  } empty_slots, occupied_slots;
} Entities;

extern StatusCode entity_InitProperties(EntityProps **pProps);
extern void entity_DeleteProperties(EntityProps **pProps);
extern void entity_DumpProperties(EntityProps *props);

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
 typedef PACKED_ENUM{NO_ENTITY, PLAYER, NPC} EntityType;

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
