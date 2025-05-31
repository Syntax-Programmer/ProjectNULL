#pragma once

#include "../common.h"

extern bool arena_Init();
extern size_t arena_AllocData(size_t data_size);
extern bool arena_SetData(uint8_t *data, size_t data_offset, size_t data_size);
extern uint8_t *arena_FetchData(size_t data_offset, size_t data_size);
extern void arena_Free();
