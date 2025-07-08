#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef struct __BumpArena BumpArena;

extern BumpArena *mem_BumpArenaCreate(u64 size);
extern StatusCode mem_BumpArenaDelete(BumpArena *arena);
extern void *mem_BumpArenaAlloc(BumpArena *arena, u64 size);
extern StatusCode mem_BumpArenaReset(BumpArena *arena);

#ifdef __cplusplus
}
#endif
