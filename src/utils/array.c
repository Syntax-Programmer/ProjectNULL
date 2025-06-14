#include "../../include/utils/array.h"
#include "../../include/utils/arena.h"
#include <string.h>

#define MIN_ARRAY_SLOTS (16)

struct AppendArray {
  size_t data_size, capacity, len;
  void *data;
};

struct FlexArray {
  size_t data_size, capacity;
  void *data;
};

/* ----  APPEND ARRAY  ---- */

AppendArray *arr_AppendArrayCreate(size_t data_size) {
  AppendArray *append_array = arena_Alloc(sizeof(AppendArray));
  if (!append_array) {
    LOG("Can't create an append_array due to memory failure.");
    return NULL;
  }

  append_array->data_size = data_size;
  append_array->capacity = MIN_ARRAY_SLOTS;
  append_array->len = 0;

  append_array->data = arena_Alloc(data_size * MIN_ARRAY_SLOTS);
  if (!(append_array->data)) {
    arr_AppendArrayDelete(append_array);
    LOG("Can't create an append_array due to memory failure.");
    return NULL;
  }

  return append_array;
}

void arr_AppendArrayDelete(AppendArray *append_array) {
  if (append_array) {
    if (append_array->data) {
      arena_Dealloc(append_array->data,
                    append_array->data_size * append_array->capacity);
      append_array->data = NULL;
    }
    append_array->capacity = append_array->data_size = append_array->len = 0;
    arena_Dealloc(append_array, sizeof(AppendArray));
  }
}

size_t arr_GetAppendArrayLen(AppendArray *append_array) {
  assert(append_array);
  return append_array->len;
}

size_t arr_GetAppendArrayCapacity(AppendArray *append_array) {
  assert(append_array);
  return append_array->capacity;
}

void *arr_GetAppendArrayIndexValue(AppendArray *append_array, size_t index) {
  assert(append_array);
  if (index >= append_array->len) {
    LOG("Can't get append_array index beyond available indices.");
    return NULL;
  }

  return (uint8_t *)append_array->data + (append_array->data_size * index);
}

StatusCode arr_SetAppendArrayIndexValue(AppendArray *append_array, void *value,
                                        size_t index) {
  assert(append_array);
  if (index >= append_array->len) {
    LOG("Can't set append_array index beyond available indices.");
    return WARNING;
  }

  void *dest =
      (uint8_t *)append_array->data + (append_array->data_size * index);
  memcpy(dest, value, append_array->data_size);

  return SUCCESS;
}

StatusCode arr_AppendArrayPush(AppendArray *append_array, void *value,
                               size_t (*grow_callback)(size_t old_capacity)) {
  assert(append_array);
  if (append_array->len == append_array->capacity) {
    size_t new_capacity =
        (grow_callback)
            ? grow_callback(append_array->capacity)
            : append_array->capacity + (append_array->capacity >> 1);
    if (new_capacity <= append_array->capacity) {
      LOG("AppendArray size increase callback producing faulty results. New "
          "size can't be less than original. Reverting to default resizing "
          "scheme.");
      new_capacity = append_array->capacity + (append_array->capacity >> 1);
    }
    void *data = arena_Realloc(
        append_array->data, (append_array->data_size * append_array->capacity),
        (append_array->data_size * new_capacity));
    if (!data) {
      LOG("Can't add any more elements to append_array, memory allocation "
          "failure.");
      return RESOURCE_EXHAUSTED;
    }
    append_array->data = data;
    append_array->capacity = new_capacity;
  }

  // Since this can't ever fail in this case, we need not check for errors.
  arr_SetAppendArrayIndexValue(append_array, value, append_array->len++);

  return SUCCESS;
}

void arr_AppendArrayPop(AppendArray *append_array) {
  assert(append_array);
  if (append_array->len == 0) {
    LOG("Empty append_array, can't pop more.");
    return;
  }
  append_array->len--;
}

StatusCode arr_AppendArrayShrinkToFit(AppendArray *append_array) {
  assert(append_array);
  /*
   * This operation is guaranteed to return back the old ptr. So no assignment
   * needed
   */
  if (arena_Realloc(append_array->data,
                    (append_array->data_size * append_array->capacity),
                    (append_array->data_size * append_array->len)) == NULL) {
    return RESOURCE_EXHAUSTED;
  }
  append_array->capacity = append_array->len;

  return SUCCESS;
}

void arr_Reset(AppendArray *append_array) {
  assert(append_array);
  append_array->len = 0;
}

void *arr_GetAppendArrayRawData(AppendArray *append_array) {
  assert(append_array);
  return append_array->data;
}

/* ----  FLEX ARRAY  ---- */

FlexArray *arr_FlexArrayCreate(size_t data_size, size_t initial_capacity) {
  FlexArray *flex_array = arena_Alloc(sizeof(FlexArray));
  if (!flex_array) {
    LOG("Can't create an flex_array due to memory failure.");
    return NULL;
  }

  flex_array->capacity = MIN_ARRAY_SLOTS;
  flex_array->data_size = data_size;

  flex_array->data = arena_Alloc(data_size * initial_capacity);
  if (!(flex_array->data)) {
    arr_FlexArrayDelete(flex_array);
    LOG("Can't create an append_array due to memory failure.");
    return NULL;
  }

  return flex_array;
}

void arr_FlexArrayDelete(FlexArray *flex_array) {
  if (flex_array) {
    if (flex_array->data) {
      arena_Dealloc(flex_array->data,
                    flex_array->data_size * flex_array->capacity);
      flex_array->data = NULL;
    }
    flex_array->data_size = flex_array->capacity = 0;
    arena_Dealloc(flex_array, sizeof(FlexArray));
  }
}

size_t arr_GetFlexArrayCapacity(FlexArray *flex_array) {
  assert(flex_array);
  return flex_array->capacity;
}

void *arr_GetFlexArrayIndexValue(FlexArray *flex_array, size_t index) {
  assert(flex_array);
  if (index >= flex_array->capacity) {
    LOG("Can't get append_array index beyond available capacity.");
    return NULL;
  }
  return (uint8_t *)flex_array->data + (flex_array->data_size * index);
}

StatusCode arr_SetFlexArrayIndexValue(FlexArray *flex_array, void *value,
                                      size_t index) {
  assert(flex_array);
  if (index >= flex_array->capacity) {
    LOG("Can't get append_array index beyond available capacity.");
    return WARNING;
  }
  void *dest = (uint8_t *)flex_array->data + (flex_array->data_size * index);
  memcpy(dest, value, flex_array->data_size);

  return SUCCESS;
}

void arr_FlexArrayReset(FlexArray *flex_array) {
  assert(flex_array);
  memset(flex_array->data, 0, flex_array->data_size * flex_array->capacity);
}

void *arr_GetFlexArrayRawData(FlexArray *flex_array) {
  assert(flex_array);
  return flex_array->data;
}

StatusCode arr_GrowFlexArray(FlexArray *flex_array,
                             size_t (*grow_callback)(size_t old_capacity)) {
  size_t new_capacity =
      (grow_callback) ? grow_callback(flex_array->capacity)
                      : flex_array->capacity + (flex_array->capacity >> 1);
  if (new_capacity <= flex_array->capacity) {
    LOG("FlexArray size increase callback producing faulty results. New "
        "size can't be less than original. Reverting to default resizing "
        "scheme.");
    new_capacity = flex_array->capacity + (flex_array->capacity >> 1);
  }

  void *data = arena_Realloc(flex_array->data,
                             (flex_array->data_size * flex_array->capacity),
                             (flex_array->data_size * new_capacity));
  if (!data) {
    LOG("Can't add any more elements to append_array, memory allocation "
        "failure.");
    return RESOURCE_EXHAUSTED;
  }
  flex_array->data = data;
  flex_array->capacity = new_capacity;

  return SUCCESS;
}
