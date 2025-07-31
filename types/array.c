#include "array.h"

#define STD_ARR_SIZE (16)

/* ----  VECTOR  ---- */

struct __Vector {
  u64 cap;
  u64 len;
  u64 elem_size;
  void *mem;
};

Vector *arr_VectorCreate(u64 elem_size) {
  return arr_VectorCustomCreate(elem_size, STD_ARR_SIZE);
}

Vector *arr_VectorCustomCreate(u64 elem_size, u64 cap) {
  Vector *arr = malloc(sizeof(Vector));
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(arr, NULL);

  arr->mem = malloc(elem_size * cap);
  IF_NULL(arr->mem) {
    arr_VectorDelete(arr);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(arr->mem, NULL);
  }
  arr->len = 0;
  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_VectorDelete(Vector *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  if (arr->mem) {
    free(arr->mem);
  }
  free(arr);

  return SUCCESS;
}

StatusCode arr_VectorGet(const Vector *arr, u64 i, void *dest) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(dest, NULL_EXCEPTION);

  if (i >= arr->len) {
    STATUS_LOG(OUT_OF_BOUNDS_ACCESS,
               "Cannot access vector array beyond its len.");
    return OUT_OF_BOUNDS_ACCESS;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorSet(Vector *arr, u64 i, const void *data) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(data, NULL_EXCEPTION);

  if (i >= arr->len) {
    STATUS_LOG(OUT_OF_BOUNDS_ACCESS,
               "Cannot access vector array beyond its len.");
    return OUT_OF_BOUNDS_ACCESS;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorPush(Vector *arr, const void *data,
                          u64 (*grow_callback)(u64 old_cap)) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(data, NULL_EXCEPTION);

  if (arr->len == arr->cap) {
    u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
    if (new_cap < arr->cap) {
      STATUS_LOG(WARNING, "Cannot grow vector, faulty grow callback. Default "
                          "grow strat to be used.");
      new_cap = arr->cap * 2;
    }
    void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
    MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(new_mem, CREATION_FAILURE);

    arr->mem = new_mem;
    arr->cap = new_cap;
  }

  memcpy(MEM_OFFSET(arr->mem, (arr->len++) * arr->elem_size), data,
         arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorPushEmpty(Vector *arr, u64 (*grow_callback)(u64 old_cap),
                               bool memset_zero) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  if (arr->len == arr->cap) {
    u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
    if (new_cap < arr->cap) {
      STATUS_LOG(WARNING, "Cannot grow vector, faulty grow callback. Default "
                          "grow strat to be used.");
      new_cap = arr->cap * 2;
    }
    void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
    MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(new_mem, CREATION_FAILURE);

    arr->mem = new_mem;
    arr->cap = new_cap;
  }

  if (memset_zero) {
    memset(MEM_OFFSET(arr->mem, (arr->len++) * arr->elem_size), 0,
           arr->elem_size);
  }

  return SUCCESS;
}

StatusCode arr_VectorPop(Vector *arr, void *dest) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  if (!arr->len) {
    STATUS_LOG(FAILURE, "Can not pop more from the vector array.");
    return FAILURE;
  }

  // Allows for users who don't want the data.
  if (dest) {
    memcpy(dest, MEM_OFFSET(arr->mem, --(arr->len) * arr->elem_size),
           arr->elem_size);
  } else {
    --(arr->len);
  }

  return SUCCESS;
}

u64 arr_VectorLen(const Vector *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, INVALID_INDEX);

  return arr->len;
}

StatusCode arr_VectorFit(Vector *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  void *new_mem = realloc(arr->mem, arr->len * arr->elem_size);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(new_mem, CREATION_FAILURE);

  arr->cap = arr->len;
  arr->mem = new_mem;

  return SUCCESS;
}

StatusCode arr_VectorReset(Vector *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  memset(arr->mem, 0, arr->cap * arr->elem_size);
  arr->len = 0;

  return SUCCESS;
}

void *arr_VectorRaw(const Vector *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL);

  return arr->mem;
}

StatusCode arr_VectorForEach(Vector *arr,
                             StatusCode (*foreach_callback)(void *val)) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(foreach_callback, NULL_EXCEPTION);

  for (u64 i = 0; i < arr->len; i++) {
    foreach_callback(MEM_OFFSET(arr->mem, i * arr->elem_size));
  }

  return SUCCESS;
}

/* ----  BUFFER ARRAY  ---- */

struct __BuffArr {
  u64 cap;
  u64 elem_size;
  void *mem;
};

BuffArr *arr_BuffArrCreate(u64 cap, u64 elem_size) {
  BuffArr *arr = malloc(sizeof(BuffArr));
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(arr, NULL);

  arr->mem = malloc(elem_size * cap);
  IF_NULL(arr->mem) {
    arr_BuffArrDelete(arr);
    MEM_ALLOC_FAILURE_SUB_ROUTINE(arr->mem, NULL);
  }
  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_BuffArrDelete(BuffArr *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  if (arr->mem) {
    free(arr->mem);
  }
  free(arr);

  return SUCCESS;
}

StatusCode arr_BuffArrGet(const BuffArr *arr, u64 i, void *dest) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(dest, NULL_EXCEPTION);

  if (i >= arr->cap) {
    STATUS_LOG(OUT_OF_BOUNDS_ACCESS,
               "Cannot access buff array beyond its cap.");
    return OUT_OF_BOUNDS_ACCESS;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_BuffArrSet(BuffArr *arr, u64 i, const void *data) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(data, NULL_EXCEPTION);

  if (i >= arr->cap) {
    STATUS_LOG(OUT_OF_BOUNDS_ACCESS,
               "Cannot access buff array beyond its cap.");
    return OUT_OF_BOUNDS_ACCESS;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

u64 arr_BuffArrCap(const BuffArr *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  return arr->cap;
}

StatusCode arr_BuffArrGrow(BuffArr *arr, u64 (*grow_callback)(u64 old_cap)) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
  if (new_cap < arr->cap) {
    STATUS_LOG(WARNING, "Cannot grow vector, faulty grow callback. Default "
                        "grow strat to be used.");
    new_cap = arr->cap * 2;
  }
  void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
  MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(new_mem, CREATION_FAILURE);

  arr->mem = new_mem;
  arr->cap = new_cap;

  return SUCCESS;
}

StatusCode arr_BuffArrReset(BuffArr *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);

  memset(arr->mem, 0, arr->cap * arr->elem_size);

  return SUCCESS;
}

void *arr_BuffArrRaw(const BuffArr *arr) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL);

  return arr->mem;
}

StatusCode arr_BuffArrForEach(BuffArr *arr,
                              StatusCode (*foreach_callback)(void *val)) {
  NULL_FUNC_ARG_ROUTINE(arr, NULL_EXCEPTION);
  NULL_FUNC_ARG_ROUTINE(foreach_callback, NULL_EXCEPTION);

  for (u64 i = 0; i < arr->cap; i++) {
    foreach_callback(MEM_OFFSET(arr->mem, i * arr->elem_size));
  }

  return SUCCESS;
}
