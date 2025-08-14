#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"
#include "../utils/status.h"

typedef struct __Hm Hm;

typedef enum { HM_ADD_OVERWRITE, HM_ADD_FAIL, HM_ADD_PRESERVE } HmAddModes;

Hm *hm_Create(u64 (*hash_func)(const void *key),
              bool (*cmp_func)(const void *key, const void *compare_key),
              StatusCode (*key_delete_callback)(void *key),
              StatusCode (*val_delete_callback)(void *val));
StatusCode hm_Delete(Hm *hm);
StatusCode hm_AddEntry(Hm *hm, void *key, void *val, HmAddModes mode);
void *hm_GetEntry(const Hm *hm, void *key);
StatusCode hm_DeleteEntry(Hm *hm, void *key);
u64 hm_GetLen(const Hm *hm);
StatusCode hm_ForEach(Hm *hm, void (*foreach_callback)(void *key, void *val));

#ifdef __cplusplus
}
#endif
