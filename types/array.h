#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"
#include "../utils/status.h"

/* ----  VECTOR  ---- */

typedef struct __Vector Vector;

extern Vector *arr_VectorCreate(u64 elem_size);
extern Vector *arr_VectorCustomCreate(u64 elem_size, u64 cap);
extern StatusCode arr_VectorDelete(Vector *arr);
extern StatusCode arr_VectorGet(const Vector *arr, u64 i, void *dest);
extern StatusCode arr_VectorSet(Vector *arr, u64 i, const void *data);
extern StatusCode arr_VectorPush(Vector *arr, const void *data,
                                 u64 (*grow_callback)(u64 old_cap));
StatusCode arr_VectorPushEmpty(Vector *arr, u64 (*grow_callback)(u64 old_cap),
                               bool memset_zero);
extern StatusCode arr_VectorPop(Vector *arr, void *dest);
extern u64 arr_VectorLen(const Vector *arr);
extern StatusCode arr_VectorFit(Vector *arr);
extern StatusCode arr_VectorReset(Vector *arr);
extern void *arr_VectorRaw(const Vector *arr);
extern StatusCode arr_VectorForEach(Vector *arr,
                                    StatusCode (*foreach_callback)(void *val));

/* ----  BUFFER ARRAY  ---- */

typedef struct __BuffArr BuffArr;

extern BuffArr *arr_BuffArrCreate(u64 cap, u64 elem_size);
extern StatusCode arr_BuffArrDelete(BuffArr *arr);
extern StatusCode arr_BuffArrGet(const BuffArr *arr, u64 i, void *dest);
extern StatusCode arr_BuffArrSet(BuffArr *arr, u64 i, const void *data);
extern u64 arr_BuffArrCap(const BuffArr *arr);
extern StatusCode arr_BuffArrGrow(BuffArr *arr,
                                  u64 (*grow_callback)(u64 old_cap));
extern StatusCode arr_BuffArrReset(BuffArr *arr);
extern void *arr_BuffArrRaw(const BuffArr *arr);
extern StatusCode arr_BuffArrForEach(BuffArr *arr,
                                     StatusCode (*foreach_callback)(void *val));

#ifdef __cplusplus
}
#endif
