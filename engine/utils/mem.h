#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/* ----  BUMP ARENA  ---- */

typedef struct __BumpArena BumpArena;

extern BumpArena *mem_BumpArenaCreate(u64 size);
extern StatusCode mem_BumpArenaDelete(BumpArena *arena);
extern void *mem_BumpArenaAlloc(BumpArena *arena, u64 size);
extern StatusCode mem_BumpArenaReset(BumpArena *arena);

/* ----  POOL ARENA  ---- */

typedef struct __PoolArena PoolArena;

#ifdef __cplusplus
}
#endif
