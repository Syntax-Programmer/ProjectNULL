#include "../../include/utils/array.h"
#include "../../include/utils/arena.h"

#define MIN_ARRAY_SLOTS (16)

struct AppendArr {
  size_t data_size, capacity, len;
  void *data;
};

struct FlexArr {
  size_t data_size, capacity;
  void *data;
};

/* ----  APPEND ARRAY  ---- */

AppendArr *arr_AppendArrCreate(size_t data_size) {
  AppendArr *append_array = arena_Alloc(sizeof(AppendArr));
  if (!append_array) {
    LOG("Can not create an append_array due to memory failure.");
    return NULL;
  }

  append_array->data_size = data_size;
  append_array->capacity = MIN_ARRAY_SLOTS;
  append_array->len = 0;

  append_array->data = arena_Alloc(data_size * MIN_ARRAY_SLOTS);
  if (!(append_array->data)) {
    arr_AppendArrDelete(append_array);
    LOG("Can not create an append_array due to memory failure.");
    return NULL;
  }

  return append_array;
}

AppendArr *arr_SizedAppendArrCreate(size_t data_size, size_t capacity) {
  AppendArr *append_array = arena_Alloc(sizeof(AppendArr));
  if (!append_array) {
    LOG("Can not create an append_array due to memory failure.");
    return NULL;
  }

  append_array->data_size = data_size;
  append_array->capacity = capacity;
  append_array->len = 0;

  append_array->data = arena_Alloc(data_size * capacity);
  if (!(append_array->data)) {
    arr_AppendArrDelete(append_array);
    LOG("Can not create an append_array due to memory failure.");
    return NULL;
  }

  return append_array;
}

void arr_AppendArrDelete(AppendArr *append_array) {
  if (append_array) {
    if (append_array->data) {
      arena_Dealloc(append_array->data,
                    append_array->data_size * append_array->capacity);
      append_array->data = NULL;
    }
    append_array->capacity = append_array->data_size = append_array->len = 0;
    arena_Dealloc(append_array, sizeof(AppendArr));
  }
}

size_t arr_GetAppendArrLen(AppendArr *append_array) {
  assert(append_array);
  return append_array->len;
}

size_t arr_GetAppendArrCapacity(AppendArr *append_array) {
  assert(append_array);
  return append_array->capacity;
}

void *arr_GetAppendArrIndexValue(AppendArr *append_array, size_t index) {
  assert(append_array);
  if (index >= append_array->len) {
    LOG("Can not get append_array index beyond available indices.");
    return NULL;
  }

  return (uint8_t *)append_array->data + (append_array->data_size * index);
}

StatusCode arr_SetAppendArrIndexValue(AppendArr *append_array, void *value,
                                        size_t index) {
  assert(append_array);
  if (index >= append_array->len) {
    LOG("Can not set append_array index beyond available indices.");
    return WARNING;
  }

  void *dest =
      (uint8_t *)append_array->data + (append_array->data_size * index);
  memcpy(dest, value, append_array->data_size);

  return SUCCESS;
}

StatusCode arr_AppendArrPush(AppendArr *append_array, void *value,
                               size_t (*grow_callback)(size_t old_capacity)) {
  assert(append_array);
  if (append_array->len == append_array->capacity) {
    size_t new_capacity =
        (grow_callback)
            ? grow_callback(append_array->capacity)
            : append_array->capacity + (append_array->capacity >> 1);
    if (new_capacity <= append_array->capacity) {
      LOG("AppendArr size increase callback producing faulty results. New "
          "size Can not be less than original. Reverting to default resizing "
          "scheme.");
      new_capacity = append_array->capacity + (append_array->capacity >> 1);
    }
    void *data = arena_Realloc(
        append_array->data, (append_array->data_size * append_array->capacity),
        (append_array->data_size * new_capacity));
    if (!data) {
      LOG("Can not add any more elements to append_array, memory allocation "
          "failure.");
      return RESOURCE_EXHAUSTED;
    }
    append_array->data = data;
    append_array->capacity = new_capacity;
  }

  // Since this Can not ever fail in this case, we need not check for errors.
  arr_SetAppendArrIndexValue(append_array, value, append_array->len++);

  return SUCCESS;
}

void arr_AppendArrPop(AppendArr *append_array) {
  assert(append_array);
  if (append_array->len == 0) {
    LOG("Empty append_array, Can not pop more.");
    return;
  }
  append_array->len--;
}

StatusCode arr_AppendArrShrinkToFit(AppendArr *append_array) {
  assert(append_array);
  if (arena_Realloc(append_array->data,
                    (append_array->data_size * append_array->capacity),
                    (append_array->data_size * append_array->len)) == NULL) {
    return RESOURCE_EXHAUSTED;
  }
  append_array->capacity = append_array->len;

  return SUCCESS;
}

void arr_Reset(AppendArr *append_array) {
  assert(append_array);
  append_array->len = 0;
}

void *arr_GetAppendArrRawData(AppendArr *append_array) {
  assert(append_array);
  return append_array->data;
}

/* ----  FLEX ARRAY  ---- */

FlexArr *arr_FlexArrCreate(size_t data_size, size_t initial_capacity) {
  FlexArr *flex_array = arena_Alloc(sizeof(FlexArr));
  if (!flex_array) {
    LOG("Can not create an flex_array due to memory failure.");
    return NULL;
  }

  flex_array->capacity = MIN_ARRAY_SLOTS;
  flex_array->data_size = data_size;

  flex_array->data = arena_Alloc(data_size * initial_capacity);
  if (!(flex_array->data)) {
    arr_FlexArrDelete(flex_array);
    LOG("Can not create an append_array due to memory failure.");
    return NULL;
  }

  return flex_array;
}

void arr_FlexArrDelete(FlexArr *flex_array) {
  if (flex_array) {
    if (flex_array->data) {
      arena_Dealloc(flex_array->data,
                    flex_array->data_size * flex_array->capacity);
      flex_array->data = NULL;
    }
    flex_array->data_size = flex_array->capacity = 0;
    arena_Dealloc(flex_array, sizeof(FlexArr));
  }
}

size_t arr_GetFlexArrCapacity(FlexArr *flex_array) {
  assert(flex_array);
  return flex_array->capacity;
}

void *arr_GetFlexArrIndexValue(FlexArr *flex_array, size_t index) {
  assert(flex_array);
  if (index >= flex_array->capacity) {
    LOG("Can not get append_array index beyond available capacity.");
    return NULL;
  }
  return (uint8_t *)flex_array->data + (flex_array->data_size * index);
}

StatusCode arr_SetFlexArrIndexValue(FlexArr *flex_array, void *value,
                                      size_t index) {
  assert(flex_array);
  if (index >= flex_array->capacity) {
    LOG("Can not get append_array index beyond available capacity.");
    return WARNING;
  }
  void *dest = (uint8_t *)flex_array->data + (flex_array->data_size * index);
  memcpy(dest, value, flex_array->data_size);

  return SUCCESS;
}

void arr_FlexArrReset(FlexArr *flex_array) {
  assert(flex_array);
  memset(flex_array->data, 0, flex_array->data_size * flex_array->capacity);
}

void *arr_GetFlexArrRawData(FlexArr *flex_array) {
  assert(flex_array);
  return flex_array->data;
}

StatusCode arr_GrowFlexArr(FlexArr *flex_array,
                             size_t (*grow_callback)(size_t old_capacity)) {
  size_t new_capacity =
      (grow_callback) ? grow_callback(flex_array->capacity)
                      : flex_array->capacity + (flex_array->capacity >> 1);
  if (new_capacity <= flex_array->capacity) {
    LOG("FlexArr size increase callback producing faulty results. New "
        "size Can not be less than original. Reverting to default resizing "
        "scheme.");
    new_capacity = flex_array->capacity + (flex_array->capacity >> 1);
  }

  void *data = arena_Realloc(flex_array->data,
                             (flex_array->data_size * flex_array->capacity),
                             (flex_array->data_size * new_capacity));
  if (!data) {
    LOG("Can not add any more elements to append_array, memory allocation "
        "failure.");
    return RESOURCE_EXHAUSTED;
  }
  flex_array->data = data;
  flex_array->capacity = new_capacity;

  return SUCCESS;
}
