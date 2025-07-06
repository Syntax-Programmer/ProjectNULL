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
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  arr->mem = malloc(cap * elem_size);
  if (!(arr->mem)) {
    free(arr);
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  arr->len = 0;
  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_VectorDelete(Vector *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  free(arr->mem);
  free(arr);

  return SUCCESS;
}

StatusCode arr_VectorGet(const Vector *arr, u64 i, void *dest) {
  if (!arr || !dest) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }
  if (i >= arr->len) {
    SetStatusCode(ATTEMPTED_INVALID_ACCESS);
    return ATTEMPTED_INVALID_ACCESS;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorSet(Vector *arr, u64 i, const void *data) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }
  if (i >= arr->len) {
    SetStatusCode(ATTEMPTED_INVALID_ACCESS);
    return ATTEMPTED_INVALID_ACCESS;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

StatusCode arr_VectorPush(Vector *arr, const void *data,
                         const u64 (*grow_callback)(u64 old_cap)) {
  if (!arr || !data) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }
  if (arr->len == arr->cap) {
    u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
    if (new_cap < arr->cap) {
      printf("Can't grow push array, faulty grow callback. "
             "Default grow strat to be used.");
      new_cap = arr->cap * 2;
    }
    void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
    if (!new_mem) {
      SetStatusCode(MEMORY_ALLOCATION_FAILURE);
      return MEMORY_ALLOCATION_FAILURE;
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
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }
  if (!arr->len) {
    SetStatusCode(CANNOT_EXECUTE);
    return CANNOT_EXECUTE;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, --(arr->len) * arr->elem_size),
         arr->elem_size);

  return SUCCESS;
}

u64 arr_VectorLen(const Vector *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return (u64)-1;
  }

  return arr->len;
}

StatusCode arr_VectorFit(Vector *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  void *new_mem = realloc(arr->mem, arr->len * arr->elem_size);
  if (!new_mem) {
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return MEMORY_ALLOCATION_FAILURE;
  }

  arr->cap = arr->len;
  arr->mem = new_mem;

  return SUCCESS;
}

StatusCode arr_VectorReset(Vector *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  memset(arr->mem, 0, arr->cap * arr->elem_size);
  arr->len = 0;

  return SUCCESS;
}

void *arr_VectorRaw(const Vector *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return NULL;
  }

  return arr->mem;
}

StatusCode arr_VectorForEach(Vector *arr,
                            const void (*foreach_callback)(void *val)) {
  if (!arr || !foreach_callback) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
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
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  arr->mem = calloc(cap, elem_size);
  if (!(arr->mem)) {
    free(arr);
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  arr->elem_size = elem_size;
  arr->cap = cap;

  return arr;
}

StatusCode arr_BuffArrDelete(BuffArr *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  free(arr->mem);
  free(arr);

  return SUCCESS;
}

StatusCode arr_BuffArrGet(const BuffArr *arr, u64 i, void *dest) {
  if (!arr || !dest) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }
  if (i >= arr->cap) {
    SetStatusCode(ATTEMPTED_INVALID_ACCESS);
    return ATTEMPTED_INVALID_ACCESS;
  }

  memcpy(dest, MEM_OFFSET(arr->mem, i * arr->elem_size), arr->elem_size);

  return SUCCESS;
}

StatusCode arr_BuffArrSet(BuffArr *arr, u64 i, const void *data) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }
  if (i >= arr->cap) {
    SetStatusCode(ATTEMPTED_INVALID_ACCESS);
    return ATTEMPTED_INVALID_ACCESS;
  }

  memcpy(MEM_OFFSET(arr->mem, i * arr->elem_size), data, arr->elem_size);

  return SUCCESS;
}

u64 arr_BuffArrCap(const BuffArr *arr) { return arr->cap; }

StatusCode arr_BuffArrGrow(BuffArr *arr,
                          const u64 (*grow_callback)(u64 old_cap)) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  u64 new_cap = (grow_callback) ? grow_callback(arr->cap) : arr->cap * 2;
  if (new_cap < arr->cap) {
    printf("Can't grow buffer array, faulty grow callback. "
           "Default grow strat to be used.");
    new_cap = arr->cap * 2;
  }
  void *new_mem = realloc(arr->mem, new_cap * arr->elem_size);
  if (!new_mem) {
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return MEMORY_ALLOCATION_FAILURE;
  }
  arr->mem = new_mem;
  arr->cap = new_cap;

  return SUCCESS;
}

StatusCode arr_BuffArrReset(BuffArr *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  memset(arr->mem, 0, arr->cap * arr->elem_size);

  return SUCCESS;
}

void *arr_BuffArrRaw(const BuffArr *arr) {
  if (!arr) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return NULL;
  }

  return arr->mem;
}

StatusCode arr_BuffArrForEach(BuffArr *arr,
                             const void (*foreach_callback)(void *val)) {
  if (!arr || !foreach_callback) {
    SetStatusCode(INVALID_FUNCTION_ARGUMENTS);
    return INVALID_FUNCTION_ARGUMENTS;
  }

  for (u64 i = 0; i < arr->cap; i++) {
    foreach_callback(MEM_OFFSET(arr->mem, i * arr->elem_size));
  }

  return SUCCESS;
}
