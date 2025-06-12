#pragma once

#include "../common.h"

#define INVALID_OFFSET ((size_t)-1)

#define arena_Dealloc(data, size) (arena_Realloc(data, size, 0))

extern StatusCode arena_Init(void);
extern void arena_Delete(void);
extern void *arena_Alloc(size_t data_size);
extern void *arena_Realloc(void *old_data, size_t old_size, size_t new_size);
extern void arena_Reset();
extern void arena_Dump();
