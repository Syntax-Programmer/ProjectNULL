#pragma once

#include "../common.h"

#define INVALID_HASHMAP_VALUE (INT64_MIN)

typedef struct StrIntHashmap StrIntHashmap;

extern StrIntHashmap *hm_Create(void);
extern void hm_Delete(StrIntHashmap *hashmap);
extern StatusCode hm_AddEntry(StrIntHashmap *hashmap, CharBuffer key,
                              int64_t val);
extern int64_t hm_FetchValue(StrIntHashmap *hashmap, CharBuffer key);
extern StatusCode hm_DeleteEntry(StrIntHashmap *hashmap, CharBuffer key);
extern size_t hm_GetLen(StrIntHashmap *hashmap);
extern void hm_ForEach(StrIntHashmap *hashmap,
                       void (*foreach_callback)(const CharBuffer key,
                                                int64_t *pVal));
