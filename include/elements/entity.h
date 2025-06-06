#pragma once

#include "../../include/utils/hashmap.h"
#include "../common.h"
#include <SDL2/SDL.h>

#define MAX_SPAWN_COUNT 48

/*
 * This will have initial properties of the entities.
 * This will contain both constant and variable properties.
 *
 * Constant properties will remain in this struct enclosed. While the variable
 * properties will also be present in the Entities struct to reveal to other
 * modules.
 *
 * An Id will link the element of Entities to this struct.
 */
typedef struct EntityProps EntityProps;

typedef struct {
  // This will also be the len of MAX_SPAWN_COUNT.
 // StrHashmap *names;
  SDL_FRect bounding_boxes[MAX_SPAWN_COUNT];
  uint16_t hp[MAX_SPAWN_COUNT];
  struct {
    uint16_t index[MAX_SPAWN_COUNT];
    size_t len;
  } empty_slots, occupied_slots;
} Entities;
