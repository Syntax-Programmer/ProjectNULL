#include "../../include/utils/array.h"
#include "../../include/utils/arena.h"

#define MIN_ARRAY_SLOTS (16)

struct AppendArray {
  size_t data_size, capacity, len;
  void *data;
};

struct FlexArray {
  size_t data_size, capacity, fill_count;
  void *data;
};

AppendArray *array_AppendArrayCreate(size_t data_size) {
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
    array_AppendArrayDelete(append_array);
    LOG("Can't create an append_array due to memory failure.");
    return NULL;
  }

  return append_array;
}

void array_AppendArrayDelete(AppendArray *append_array) {
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

size_t array_GetAppendArrayLen(AppendArray *append_array) {
  assert(append_array);
  return append_array->len;
}

size_t array_GetAppendArrayCapacity(AppendArray *append_array) {
  assert(append_array);
  return append_array->capacity;
}

void *array_GetAppendArrayIndexValue(AppendArray *append_array, size_t index) {
  assert(append_array);
  if (index >= append_array->len) {
    LOG("Can't get append_array index beyond available indices.");
    return NULL;
  }

  return (uint8_t *)append_array->data + (append_array->data_size * index);
}

StatusCode array_SetAppendArrayIndexValue(AppendArray *append_array,
                                          void *value, size_t index) {
  assert(append_array);
  if (index >= append_array->len) {
    LOG("Can't set append_array index beyond available indices.");
    return FAILURE;
  }

  void *dest =
      (uint8_t *)append_array->data + (append_array->data_size * index);
  memcpy(dest, value, append_array->data_size);

  return SUCCESS;
}

StatusCode array_AppendArrayPush(AppendArray *append_array, void *value,
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
      return FAILURE;
    }
    append_array->data = data;
    append_array->capacity = new_capacity;
  }

  // Since this can't ever fail in this case, we need not check for errors.
  array_SetAppendArrayIndexValue(append_array, value, append_array->len++);

  return SUCCESS;
}

void array_AppendArrayPop(AppendArray *append_array) {
  assert(append_array);
  if (append_array->len == 0) {
    LOG("Empty append_array, can't pop more.");
    return;
  }
  append_array->len--;
}

StatusCode array_AppendArrayShrinkToFit(AppendArray *append_array) {
  assert(append_array);
  /*
   * This operation is guaranteed to return back the old ptr. So no assignment
   * needed
   */
  arena_Realloc(append_array->data,
                (append_array->data_size * append_array->capacity),
                (append_array->data_size * append_array->len));
  append_array->capacity = append_array->len;

  return SUCCESS;
}

void array_Reset(AppendArray *append_array) {
  assert(append_array);
  append_array->len = 0;
}

void *array_GetAppendArrayRawData(AppendArray *append_array) {
  assert(append_array);
  return append_array->data;
}

FlexArray *array_FlexArrayCreate(size_t data_size, size_t initial_capacity) {
  FlexArray *flex_array = arena_Alloc(sizeof(FlexArray));
  if (!flex_array) {
    LOG("Can't create an flex_array due to memory failure.");
    return NULL;
  }

  flex_array->capacity = MIN_ARRAY_SLOTS;
  flex_array->data_size = data_size;
  flex_array->fill_count = 0;

  flex_array->data = arena_Alloc(data_size * initial_capacity);
  if (!(flex_array->data)) {
    array_FlexArrayDelete(flex_array);
    LOG("Can't create an append_array due to memory failure.");
    return NULL;
  }

  return flex_array;
}

void array_FlexArrayDelete(FlexArray *flex_array) {
  if (flex_array) {
    if (flex_array->data) {
      arena_Dealloc(flex_array->data,
                    flex_array->data_size * flex_array->capacity);
      flex_array->data = NULL;
    }
    flex_array->data_size = flex_array->capacity = flex_array->fill_count = 0;
    arena_Dealloc(flex_array, sizeof(FlexArray));
  }
}

size_t array_GetFlexArrayCapacity(FlexArray *flex_array) {
  assert(flex_array);
  return flex_array->capacity;
}

size_t array_GetFlexArrayFillCount(FlexArray *flex_array) {
  assert(flex_array);
  return flex_array->fill_count;
}

void *array_GetFlexArrayIndexValue(FlexArray *flex_array, size_t index) {
  assert(flex_array);
  if (index >= flex_array->capacity) {
    LOG("Can't get append_array index beyond available capacity.");
    return NULL;
  }
  return (uint8_t *)flex_array->data + (flex_array->data_size * index);
}

StatusCode array_SetFlexArrayIndexValue(FlexArray *flex_array, void *value,
                                        size_t index) {
  assert(flex_array);
  if (index >= flex_array->capacity) {
    LOG("Can't get append_array index beyond available capacity.");
    return FAILURE;
  }
  void *dest = (uint8_t *)flex_array->data + (flex_array->data_size * index);
  memcpy(dest, value, flex_array->data_size);

  flex_array->fill_count++;

  return SUCCESS;
}

void array_FlexArrayReset(FlexArray *flex_array) {
  assert(flex_array);
  flex_array->fill_count = 0;
  memset(flex_array->data, 0, flex_array->data_size * flex_array->capacity);
}

void *array_GetFlexArrayRawData(FlexArray *flex_array) {
  assert(flex_array);
  return flex_array->data;
}

StatusCode array_GrowFlexArray(FlexArray *flex_array,
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
    return FAILURE;
  }
  flex_array->data = data;
  flex_array->capacity = new_capacity;

  return SUCCESS;
}
