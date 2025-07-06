#pragma once

#include "../common.h"
#include "../elements/entities.h"
#include "../elements/world.h"

typedef PACKED_ENUM{
    UP = 1 << 0, DOWN = 1 << 1, LEFT = 1 << 2, RIGHT = 1 << 3, QUIT = 1 << 4,
} InputFlags;

extern void state_HandleState(InputFlags input_flags, double delta_time);
extern InputFlags GetInput(void);
