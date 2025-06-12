#pragma once

#include "../common.h"

#define INVALID_HASHMAP_VALUE (INT64_MIN)

typedef struct StrIntHashmap StrIntHashmap;

extern StrIntHashmap *hashmap_Create();
extern void hashmap_Delete(StrIntHashmap *hashmap);
extern StatusCode hashmap_AddEntry(StrIntHashmap *hashmap, String key,
                                   int64_t val);
extern int64_t hashmap_FetchValue(StrIntHashmap *hashmap, String key);
extern StatusCode hashmap_DeleteEntry(StrIntHashmap *hashmap,
                                      String key);
extern size_t hashmap_GetLen(StrIntHashmap *hashmap);
