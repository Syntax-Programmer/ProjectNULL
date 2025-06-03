#pragma once

#include "../common.h"

#define INVALID_OFFSET ((size_t) -1)

extern bool arena_Init();
extern size_t arena_AllocData(size_t data_size);
extern bool arena_SetData(uint8_t *data, size_t data_offset, size_t data_size);
extern size_t arena_ReallocData(size_t original_data_offset, size_t data_size,
                                size_t new_size);
extern uint8_t *arena_FetchData(size_t data_offset, size_t data_size);
extern void arena_Free();
