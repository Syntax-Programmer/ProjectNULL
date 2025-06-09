#pragma once

#include "../common.h"

#define INVALID_HASHMAP_VALUE (INT64_MIN)

typedef struct StrIntHashmap StrIntHashmap;

extern StrIntHashmap *hashmap_Init();
extern StatusCode hashmap_AddEntry(StrIntHashmap *hashmap, String key,
                                   int64_t val);
extern int64_t hashmap_FetchValue(StrIntHashmap *hashmap, String key);
extern StatusCode hashmap_DeleteEntry(StrIntHashmap *hashmap,
                                      String key);
extern uint64_t hashmap_GetLen(StrIntHashmap *hashmap);
