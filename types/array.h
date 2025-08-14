#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"
#include "../utils/status.h"

/* ----  VECTOR  ---- */

typedef struct __Vector Vector;

Vector *arr_VectorCreate(u64 elem_size);
Vector *arr_VectorCustomCreate(u64 elem_size, u64 cap);
StatusCode arr_VectorDelete(Vector *arr);
StatusCode arr_VectorGet(const Vector *arr, u64 i, void *dest);
StatusCode arr_VectorSet(Vector *arr, u64 i, const void *data);
StatusCode arr_VectorPush(Vector *arr, const void *data,
                          u64 (*grow_callback)(u64 old_cap));
StatusCode arr_VectorPushEmpty(Vector *arr, u64 (*grow_callback)(u64 old_cap),
                               bool memset_zero);
StatusCode arr_VectorPop(Vector *arr, void *dest);
u64 arr_VectorLen(const Vector *arr);
StatusCode arr_VectorFit(Vector *arr);
StatusCode arr_VectorReset(Vector *arr);
void *arr_VectorRaw(const Vector *arr);
StatusCode arr_VectorForEach(Vector *arr,
                             StatusCode (*foreach_callback)(void *val));

/* ----  BUFFER ARRAY  ---- */

typedef struct __BuffArr BuffArr;

BuffArr *arr_BuffArrCreate(u64 elem_size, u64 cap);
StatusCode arr_BuffArrDelete(BuffArr *arr);
StatusCode arr_BuffArrGet(const BuffArr *arr, u64 i, void *dest);
StatusCode arr_BuffArrSet(BuffArr *arr, u64 i, const void *data);
u64 arr_BuffArrCap(const BuffArr *arr);
StatusCode arr_BuffArrGrow(BuffArr *arr, u64 new_cap);
StatusCode arr_BuffArrGrowWCallback(BuffArr *arr,
                                    u64 (*grow_callback)(u64 old_cap));
StatusCode arr_BuffArrReset(BuffArr *arr);
void *arr_BuffArrRaw(const BuffArr *arr);
StatusCode arr_BuffArrForEach(BuffArr *arr,
                              StatusCode (*foreach_callback)(void *val));
bool arr_BuffArrCmp(const BuffArr *arr1, const BuffArr *arr2);

#ifdef __cplusplus
}
#endif
