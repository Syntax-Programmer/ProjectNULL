#pragma once

#include "../common.h"

#define INVALID_OFFSET ((size_t)-1)

typedef struct Arena Arena;

extern Arena *arena_Create(void);
extern void arena_Delete(Arena *arena);
extern uint8_t *arena_AllocAndFetch(Arena *arena, size_t data_size);
extern uint8_t *arena_ReallocAndFetch(Arena *arena, uint8_t *old_data,
                                      size_t old_size, size_t new_size);
extern void arena_Reset(Arena *arena);
