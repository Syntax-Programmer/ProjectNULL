#pragma once

#include "../common.h"

typedef char FixedSizeString[DEFAULT_STR_BUFFER_SIZE];
typedef struct StrIntHashmap StrIntHashmap;

extern StrIntHashmap *hashmap_Init();
extern StatusCode hashmap_AddEntry(StrIntHashmap *hashmap, FixedSizeString key,
                                   int64_t val);
extern int64_t hashmap_FetchValue(StrIntHashmap *hashmap, FixedSizeString key);
extern StatusCode hashmap_DeleteEntry(StrIntHashmap *hashmap,
                                      FixedSizeString key);
