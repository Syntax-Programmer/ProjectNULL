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
  if (!arr) {
    printf("Can not create a new vector, memory failure.\n");
    return NULL;
  }

  arr->mem = malloc(cap * elem_size);
  if (!(arr->mem)) {
    free(arr);
    printf("Can not create a new vector, memory failure.\n");
    return NULL;
  }

  arr->len = 0;
  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_VectorDelete(Vector *arr) {
  if (!arr) {
    printf("Can not delete an invalid vector.\n");
    return FAILURE;
  }

  free(arr->mem);
  free(arr);

  return SUCCESS;
}

StatusCode arr_VectorGet(const Vector *arr, u64 i, void *dest) {
  if (!arr || !dest) {
    printf("Invalid arguments for arr_VectorGet function.\n");
    return FAILURE;
  }
  if (i >= arr->len) {
    printf("Can not access vector array beyond its len.\n");
    return FAILURE;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorSet(Vector *arr, u64 i, const void *data) {
  if (!arr || !data) {
    printf("Invalid arguments for arr_VectorSet function.\n");
    return FAILURE;
  }
  if (i >= arr->len) {
    printf("Can not access vector array beyond its len.\n");
    return FAILURE;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorPush(Vector *arr, const void *data,
                          const u64 (*grow_callback)(u64 old_cap)) {
  if (!arr || !data) {
    printf("Invalid arguments for arr_VectorPush function.\n");
    return FAILURE;
  }
  if (arr->len == arr->cap) {
    u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
    if (new_cap < arr->cap) {
      printf("Can not grow push array, faulty grow callback. "
             "Default grow strat to be used.");
      new_cap = arr->cap * 2;
    }
    void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
    if (!new_mem) {
      printf("Can not push more into vector, memory reallocation failed.\n");
    }
    arr->mem = new_mem;
    arr->cap = new_cap;
  }

  memcpy(MEM_OFFSET(arr->mem, (arr->len++) * arr->elem_size), data,
         arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorPop(Vector *arr, void *dest) {
  if (!arr || !dest) {
    printf("Invalid arguments for arr_VectorPush function.\n");
    return FAILURE;
  }
  if (!arr->len) {
    printf("Can not pop more from the vector array.\n");
    return FAILURE;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, --(arr->len) * arr->elem_size),
         arr->elem_size);

  return SUCCESS;
}

u64 arr_VectorLen(const Vector *arr) {
  if (!arr) {
    printf("Can not get len of an invalid vector.\n");
    return (u64)-1;
  }

  return arr->len;
}

StatusCode arr_VectorFit(Vector *arr) {
  if (!arr) {
    printf("Can not shrink fit an invalid vector.\n");
    return FAILURE;
  }

  void *new_mem = realloc(arr->mem, arr->len * arr->elem_size);
  if (!new_mem) {
    printf("Can not shrink fit the vector, memory reallocation failed.\n");
    return FAILURE;
  }

  arr->cap = arr->len;
  arr->mem = new_mem;

  return SUCCESS;
}

StatusCode arr_VectorReset(Vector *arr) {
  if (!arr) {
    printf("Can not reset an invalid vector.\n");
    return FAILURE;
  }

  memset(arr->mem, 0, arr->cap * arr->elem_size);
  arr->len = 0;

  return SUCCESS;
}

void *arr_VectorRaw(const Vector *arr) {
  if (!arr) {
    printf("Can not get raw data of an invalid vector.\n");
    return NULL;
  }

  return arr->mem;
}

StatusCode arr_VectorForEach(Vector *arr,
                             const void (*foreach_callback)(void *val)) {
  if (!arr || !foreach_callback) {
    printf("Invalid arguments for arr_VectorForEach function.\n");
    return FAILURE;
  }

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
  if (!arr) {
    printf("Can not create a new buffer array, memory failure.\n");
    return NULL;
  }

  arr->mem = calloc(cap, elem_size);
  if (!(arr->mem)) {
    free(arr);
    printf("Can not create a new buffer array, memory failure.\n");
    return NULL;
  }

  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_BuffArrDelete(BuffArr *arr) {
  if (!arr) {
    printf("Can not delete an invalid buffer array.\n");
    return FAILURE;
  }

  free(arr->mem);
  free(arr);

  return SUCCESS;
}

StatusCode arr_BuffArrGet(const BuffArr *arr, u64 i, void *dest) {
  if (!arr || !dest) {
    printf("Invalid arguments for arr_BuffArrGet function.\n");
    return FAILURE;
  }
  if (i >= arr->cap) {
    printf("Can not access buffer array beyond its cap.\n");
    return FAILURE;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_BuffArrSet(BuffArr *arr, u64 i, const void *data) {
  if (!arr || !data) {
    printf("Invalid arguments for arr_BuffArrSet function.\n");
    return FAILURE;
  }
  if (i >= arr->cap) {
    printf("Can not access buffer array beyond its cap.\n");
    return FAILURE;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

u64 arr_BuffArrCap(const BuffArr *arr) {
  if (!arr) {
    printf("Can not get cap of an invalid buffer array.\n");
    return FAILURE;
  }

  return arr->cap;
}

StatusCode arr_BuffArrGrow(BuffArr *arr,
                           const u64 (*grow_callback)(u64 old_cap)) {
  if (!arr) {
    printf("Can not grow an invalid buffer array.\n");
    return FAILURE;
  }

  u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
  if (new_cap < arr->cap) {
    printf("Can not grow buffer array, faulty grow callback. "
           "Default grow strat to be used.");
    new_cap = arr->cap * 2;
  }
  void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
  if (!new_mem) {
    printf("Can not grow buffer array, memory reallocation failed.\n");
    return FAILURE;
  }
  arr->mem = new_mem;
  arr->cap = new_cap;

  return SUCCESS;
}

StatusCode arr_BuffArrReset(BuffArr *arr) {
  if (!arr) {
    printf("Can not reset an invalid buffer array.\n");
    return FAILURE;
  }

  memset(arr->mem, 0, arr->cap * arr->elem_size);

  return SUCCESS;
}

void *arr_BuffArrRaw(const BuffArr *arr) {
  if (!arr) {
    printf("Can not get raw data an invalid buffer array.\n");
    return NULL;
  }

  return arr->mem;
}

StatusCode arr_BuffArrForEach(BuffArr *arr,
                              const void (*foreach_callback)(void *val)) {
  if (!arr || !foreach_callback) {
    printf("Invalid arguments for arr_BuffArrForEach function.\n");
    return FAILURE;
  }

  for (u64 i = 0; i < arr->cap; i++) {
    foreach_callback(MEM_OFFSET(arr->mem, i * arr->elem_size));
  }

  return SUCCESS;
}
