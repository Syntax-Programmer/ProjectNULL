#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"
#include "../utils/status.h"

typedef struct __HmIntKey Hm_IntKey;

typedef enum { HM_ADD_OVERWRITE, HM_ADD_FAIL, HM_ADD_PRESERVE } HmAddModes;

Hm_IntKey *hm_IntKeyCreate(void);
StatusCode hm_IntKeyDelete(Hm_IntKey *hm,
                                  StatusCode (*val_delete_callback)(void *val));
StatusCode hm_IntKeyAddEntry(Hm_IntKey *hm, u64 key, void *val,
                                    HmAddModes mode);
void *hm_IntKeyFetchEntry(const Hm_IntKey *hm, u64 key);
StatusCode
hm_IntKeyDeleteEntry(Hm_IntKey *hm, u64 key,
                     StatusCode (*val_delete_callback)(void *val));
u64 hm_IntKeyGetLen(const Hm_IntKey *hm);
StatusCode hm_IntKeyForEach(Hm_IntKey *hm,
                                   void (*foreach_callback)(const u64 key,
                                                            void *val));

#ifdef __cplusplus
}
#endif
