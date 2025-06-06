#pragma once

#include "../common.h"

#define INVALID_OFFSET ((size_t)-1)

typedef struct Arena Arena;

extern StatusCode arena_Create(void);
extern void arena_Delete(void);
extern uint8_t *arena_AllocAndFetch(size_t data_size);
extern uint8_t *arena_ReallocAndFetch(uint8_t *old_data,
                                      size_t old_size, size_t new_size);
extern void arena_Reset();
extern void arena_Dump();
