#pragma once

#include "../common.h"

// This is a sort of pythonic list. If you know what I mean.
typedef struct AppendArray AppendArray;

// This is C style resizable arrays.
typedef struct FlexArray FlexArray;

extern AppendArray *array_AppendArrayCreate(size_t data_size);
extern void array_AppendArrayDelete(AppendArray *append_array);
extern size_t array_GetAppendArrayLen(AppendArray *append_array);
extern size_t array_GetAppendArrayCapacity(AppendArray *append_array);
extern void *array_GetAppendArrayIndexValue(AppendArray *append_array,
                                            size_t index);
extern StatusCode array_SetAppendArrayIndexValue(AppendArray *append_array,
                                                 void *value, size_t index);
extern StatusCode
array_AppendArrayPush(AppendArray *append_array, void *value,
                      size_t (*grow_callback)(size_t old_capacity));
extern void array_AppendArrayPop(AppendArray *append_array);
extern StatusCode array_AppendArrayShrinkToFit(AppendArray *append_array);
extern void array_AppendArrayReset(AppendArray *append_array);
/*
 * NOTE: Only use this in internal modules with proper bounds checking. This is
 * only meant for code where you are sure what you are doing and want to
 * optimize for performance. This is a dangerous function if mistakes happen
 * with the ret value usage.
 */
extern void *array_GetAppendArrayRawData(AppendArray *append_array);

extern FlexArray *array_FlexArrayCreate(size_t data_size, size_t initial_capacity);
extern void array_FlexArrayDelete(FlexArray *flex_array);
extern size_t array_GetFlexArrayCapacity(FlexArray *flex_array);
extern size_t array_GetFlexArrayFillCount(FlexArray *flex_array);
extern void *array_GetFlexArrayIndexValue(FlexArray *flex_array, size_t index);
extern StatusCode array_SetFlexArrayIndexValue(FlexArray *flex_array,
                                               void *value, size_t index);
extern void array_FlexArrayReset(FlexArray *flex_array);
/*
 * NOTE: Only use this in internal modules with proper bounds checking. This is
 * only meant for code where you are sure what you are doing and want to
 * optimize for performance. This is a dangerous function if mistakes happen
 * with the ret value usage.
 */
extern void *array_GetFlexArrayRawData(FlexArray *flex_array);
extern StatusCode
array_GrowFlexArray(FlexArray *flex_array,
                    size_t (*grow_callback)(size_t old_capacity));
