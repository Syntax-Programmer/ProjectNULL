#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"

typedef struct __HmIntKey Hm_IntKey;

extern Hm_IntKey *hm_IntKeyCreate(void);
extern StatusCode hm_IntKeyDelete(Hm_IntKey *hm,
                                  StatusCode (*val_delete_callback)(void *val));
extern StatusCode hm_IntKeyAddEntry(Hm_IntKey *hm, i64 key, void *val,
                                    bool overwrite);
extern void *hm_IntKeyFetchEntry(const Hm_IntKey *hm, i64 key);
extern StatusCode
hm_IntKeyDeleteEntry(Hm_IntKey *hm, i64 key,
                     StatusCode (*val_delete_callback)(void *val));
extern u64 hm_IntKeyGetLen(const Hm_IntKey *hm);
extern StatusCode hm_IntKeyForEach(Hm_IntKey *hm,
                                   void (*foreach_callback)(const i64 key,
                                                            void *val));

#ifdef __cplusplus
}
#endif
