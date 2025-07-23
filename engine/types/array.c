#include "array.h"

#define STD_ARR_SIZE (16)

/* ----  VECTOR  ---- */

struct __Vector {
  void *mem;
  u64 cap;
  u64 len;
  u64 elem_size;
};

Vector *arr_VectorCreate(u64 elem_size) {
  return arr_VectorCustomCreate(elem_size, STD_ARR_SIZE);
}

Vector *arr_VectorCustomCreate(u64 elem_size, u64 cap) {
  Vector *arr = malloc(sizeof(Vector));
  CHECK_ALLOC_FAILURE(arr, NULL);

  arr->mem = malloc(cap * elem_size);
  CHECK_ALLOC_FAILURE(arr->mem, NULL, arr_VectorDelete(arr));

  arr->len = 0;
  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_VectorDelete(Vector *arr) {
  CHECK_NULL_ARG(arr, FAILURE);

  free(arr->mem);
  free(arr);

  return SUCCESS;
}

StatusCode arr_VectorGet(const Vector *arr, u64 i, void *dest) {
  CHECK_NULL_ARG(arr, FAILURE);
  CHECK_NULL_ARG(dest, FAILURE);

  if (i >= arr->len) {
    printf("Can not access vector array beyond its len.\n");
    return FAILURE;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorSet(Vector *arr, u64 i, const void *data) {
  CHECK_NULL_ARG(arr, FAILURE);
  CHECK_NULL_ARG(data, FAILURE);

  if (i >= arr->len) {
    printf("Can not access vector array beyond its len.\n");
    return FAILURE;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorPush(Vector *arr, const void *data,
                          u64 (*grow_callback)(u64 old_cap)) {
  CHECK_NULL_ARG(arr, FAILURE);
  CHECK_NULL_ARG(data, FAILURE);

  if (arr->len == arr->cap) {
    u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
    if (new_cap < arr->cap) {
      printf("Can not grow push array, faulty grow callback. "
             "Default grow strat to be used.");
      new_cap = arr->cap * 2;
    }
    void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
    CHECK_ALLOC_FAILURE(new_cap, FAILURE);

    arr->mem = new_mem;
    arr->cap = new_cap;
  }

  memcpy(MEM_OFFSET(arr->mem, (arr->len++) * arr->elem_size), data,
         arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorPop(Vector *arr, void *dest) {
  CHECK_NULL_ARG(arr, FAILURE);

  if (!arr->len) {
    printf("Can not pop more from the vector array.\n");
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
  CHECK_NULL_ARG(arr, -1);

  return arr->len;
}

StatusCode arr_VectorFit(Vector *arr) {
  CHECK_NULL_ARG(arr, FAILURE);

  void *new_mem = realloc(arr->mem, arr->len * arr->elem_size);
  CHECK_ALLOC_FAILURE(new_mem, FAILURE);

  arr->cap = arr->len;
  arr->mem = new_mem;

  return SUCCESS;
}

StatusCode arr_VectorReset(Vector *arr) {
  CHECK_NULL_ARG(arr, FAILURE);

  memset(arr->mem, 0, arr->cap * arr->elem_size);
  arr->len = 0;

  return SUCCESS;
}

void *arr_VectorRaw(const Vector *arr) {
  CHECK_NULL_ARG(arr, NULL);

  return arr->mem;
}

StatusCode arr_VectorForEach(Vector *arr,
                             StatusCode (*foreach_callback)(void *val)) {
  CHECK_NULL_ARG(arr, FAILURE);
  CHECK_NULL_ARG(foreach_callback, FAILURE);

  for (u64 i = 0; i < arr->len; i++) {
    foreach_callback(MEM_OFFSET(arr->mem, i * arr->elem_size));
  }

  return SUCCESS;
}

/* ----  BUFFER ARRAY  ---- */

struct __BuffArr {
  void *mem;
  u64 cap;
  u64 elem_size;
};

BuffArr *arr_BuffArrCreate(u64 cap, u64 elem_size) {
  BuffArr *arr = malloc(sizeof(BuffArr));
  CHECK_ALLOC_FAILURE(arr, NULL);

  arr->mem = calloc(cap, elem_size);
  CHECK_ALLOC_FAILURE(arr->mem, NULL, arr_BuffArrDelete(arr));

  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_BuffArrDelete(BuffArr *arr) {
  CHECK_NULL_ARG(arr, FAILURE);

  free(arr->mem);
  free(arr);

  return SUCCESS;
}

StatusCode arr_BuffArrGet(const BuffArr *arr, u64 i, void *dest) {
  CHECK_NULL_ARG(arr, FAILURE);
  CHECK_NULL_ARG(dest, FAILURE);

  if (i >= arr->cap) {
    printf("Can not access buffer array beyond its cap.\n");
    return FAILURE;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_BuffArrSet(BuffArr *arr, u64 i, const void *data) {
  CHECK_NULL_ARG(arr, FAILURE);
  CHECK_NULL_ARG(data, FAILURE);

  if (i >= arr->cap) {
    printf("Can not access buffer array beyond its cap.\n");
    return FAILURE;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

u64 arr_BuffArrCap(const BuffArr *arr) {
  CHECK_NULL_ARG(arr, FAILURE);

  return arr->cap;
}

StatusCode arr_BuffArrGrow(BuffArr *arr, u64 (*grow_callback)(u64 old_cap)) {
  CHECK_NULL_ARG(arr, FAILURE);

  u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
  if (new_cap < arr->cap) {
    printf("Can not grow buffer array, faulty grow callback. "
           "Default grow strat to be used.");
    new_cap = arr->cap * 2;
  }
  void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
  CHECK_ALLOC_FAILURE(new_cap, FAILURE);

  arr->mem = new_mem;
  arr->cap = new_cap;

  return SUCCESS;
}

StatusCode arr_BuffArrReset(BuffArr *arr) {
  CHECK_NULL_ARG(arr, FAILURE);

  memset(arr->mem, 0, arr->cap * arr->elem_size);

  return SUCCESS;
}

void *arr_BuffArrRaw(const BuffArr *arr) {
  CHECK_NULL_ARG(arr, NULL);

  return arr->mem;
}

StatusCode arr_BuffArrForEach(BuffArr *arr,
                              StatusCode (*foreach_callback)(void *val)) {
  CHECK_NULL_ARG(arr, FAILURE);
  CHECK_NULL_ARG(foreach_callback, FAILURE);

  for (u64 i = 0; i < arr->cap; i++) {
    foreach_callback(MEM_OFFSET(arr->mem, i * arr->elem_size));
  }

  return SUCCESS;
}
