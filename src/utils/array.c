#include "../../include/utils/array.h"
#include "../../include/utils/arena.h"

struct Array {
  size_t data_size, capacity, len;
  void *data;
};

#define MIN_ARRAY_SLOTS (16)

Array *array_Create(size_t data_size) {
  Array *array = arena_Alloc(sizeof(Array));
  if (!array) {
    LOG("Can't create an array due to memory failure.");
    return NULL;
  }

  array->data_size = data_size;
  array->capacity = MIN_ARRAY_SLOTS;
  array->len = 0;

  array->data = arena_Alloc(data_size * MIN_ARRAY_SLOTS);
  if (!(array->data)) {
    array_Delete(array);
    LOG("Can't create an array due to memory failure.");
    return NULL;
  }

  return array;
}

void array_Delete(Array *array) {
  if (array) {
    if (array->data) {
      arena_Dealloc(array->data, array->data_size * array->capacity);
      array->data = NULL;
    }
    array->capacity = array->data_size = array->len = 0;
  }
}

size_t array_GetLen(Array *array) {
  assert(array);
  return array->len;
}

size_t array_GetCapacity(Array *array) {
  assert(array);
  return array->capacity;
}

void *array_GetIndexValue(Array *array, size_t index) {
  assert(array);
  if (index >= array->len) {
    LOG("Can't get array index beyond available indices.");
    return NULL;
  }

  return (uint8_t *)array->data + (array->data_size * index);
}

StatusCode array_SetIndexValue(Array *array, void *value, size_t index) {
  assert(array);
  if (index >= array->len) {
    LOG("Can't set array index beyond available indices.");
    return FAILURE;
  }

  void *dest = (uint8_t *)array->data + (array->data_size * index);
  memcpy(dest, value, array->data_size);

  return SUCCESS;
}

StatusCode array_Push(Array *array, void *value,
                      size_t (*grow_callback)(size_t old_capacity)) {
  assert(array);
  if (array->len == array->capacity) {
    size_t new_capacity = (grow_callback)
                              ? grow_callback(array->capacity)
                              : array->capacity + (array->capacity >> 1);
    if (new_capacity <= array->capacity) {
      LOG("Array size increase callback producing faulty results. New size "
          "can't be less than original");
      return FAILURE;
    }
    void *data =
        arena_Realloc(array->data, (array->data_size * array->capacity),
                      (array->data_size * new_capacity));
    if (!data) {
      LOG("Can't add any more elements, memory allocation failure.");
      return FAILURE;
    }
    array->data = data;
    array->capacity = new_capacity;
  }

  // Since this can't ever fail in this case, we need not check for errors.
  array_SetIndexValue(array, value, array->len++);

  return SUCCESS;
}

void *array_Pop(Array *array) {
  assert(array);
  if (array->len == 0) {
    LOG("Empty array, can't pop more.");
    return NULL;
  }
  void *data = array_GetIndexValue(array, (array->len - 1));
  array->len--;
  /*
   * This is a temporary view into array's buffer, which array considers out of
   * reach. This is just a view you can see but it will be overwritten so it is
   * not recommended to store it.
   */
  return data;
}

StatusCode array_ShrinkToFit(Array *array) {
  assert(array);
  /*
   * This operation is guaranteed to return back the old ptr. So no assignment
   * needed
   */
  arena_Realloc(array->data, (array->data_size * array->capacity),
                (array->data_size * array->len));
  array->capacity = array->len;

  return SUCCESS;
}

void array_Reset(Array *array) {
  assert(array);
  array->len = 0;
}
