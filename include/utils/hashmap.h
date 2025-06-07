#pragma once

#include "../common.h"

typedef char FixedSizeString[DEFAULT_STR_BUFFER_SIZE];
typedef struct StrHashmap StrHashmap;

extern StrHashmap *hash_InitStrHashMap();
extern StatusCode hash_AddStrToMap(StrHashmap *hashmap, FixedSizeString key);
extern uint64_t hash_FetchHashIndexFromMap(StrHashmap *hashmap,
                                           FixedSizeString key);
extern StatusCode hash_DeleteStrFromMap(StrHashmap *hashmap, FixedSizeString key);
