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
extern void *mem_BumpArenaCalloc(BumpArena *arena, u64 size);
extern StatusCode mem_BumpArenaReset(BumpArena *arena);

/* ----  POOL ARENA  ---- */

typedef struct __PoolArena PoolArena;

PoolArena *mem_PoolArenaCreate(u64 block_size);
StatusCode mem_PoolArenaDelete(PoolArena *arena);
void *mem_PoolArenaAlloc(PoolArena *arena);
void *mem_PoolArenaCalloc(PoolArena *arena);
StatusCode mem_PoolArenaFree(PoolArena *arena, void *entry);
StatusCode mem_PoolArenaReset(PoolArena *arena);

#ifdef __cplusplus
}
#endif
