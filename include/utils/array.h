#pragma once

#include "../common.h"

// This is a sort of pythonic list. If you know what I mean.
typedef struct Array Array;

extern Array *array_Create(size_t data_size);
extern void array_Delete(Array *array);
extern size_t array_GetLen(Array *array);
size_t array_GetCapacity(Array *array);
extern void *array_GetIndexValue(Array *array, size_t index);
extern StatusCode array_SetIndexValue(Array *array, void *value, size_t index);
extern StatusCode array_Push(Array *array, void *value,
                             size_t (*grow_callback)(size_t old_capacity));
extern void array_Pop(Array *array);
extern StatusCode array_ShrinkToFit(Array *array);
extern void array_Reset(Array *array);
/*
 * NOTE: Only use this in internal modules with proper bounds checking. This is
 * only meant for code where you are sure what you are doing and want to
 * optimize for performance. This is a dangerous function if mistakes happen
 * with the ret value usage.
 */
extern void *array_RawData(Array *array);
