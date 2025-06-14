#pragma once

#include "../common.h"

// This is a sort of pythonic list. If you know what I mean.
typedef struct AppendArray AppendArray;

// This is C style resizable arrays.
typedef struct FlexArray FlexArray;

/* ----  APPEND ARRAY  ---- */

extern AppendArray *arr_AppendArrayCreate(size_t data_size);
extern void arr_AppendArrayDelete(AppendArray *append_array);
extern size_t arr_GetAppendArrayLen(AppendArray *append_array);
extern size_t arr_GetAppendArrayCapacity(AppendArray *append_array);
extern void *arr_GetAppendArrayIndexValue(AppendArray *append_array,
                                          size_t index);
extern StatusCode arr_SetAppendArrayIndexValue(AppendArray *append_array,
                                               void *value, size_t index);
extern StatusCode
arr_AppendArrayPush(AppendArray *append_array, void *value,
                    size_t (*grow_callback)(size_t old_capacity));
extern void arr_AppendArrayPop(AppendArray *append_array);
extern StatusCode arr_AppendArrayShrinkToFit(AppendArray *append_array);
extern void arr_AppendArrayReset(AppendArray *append_array);
/*
 * NOTE: Only use this in internal modules with proper bounds checking. This is
 * only meant for code where you are sure what you are doing and want to
 * optimize for performance. This is a dangerous function if mistakes happen
 * with the ret value usage.
 */
extern void *arr_GetAppendArrayRawData(AppendArray *append_array);

/* ----  FLEX ARRAY  ---- */

extern FlexArray *arr_FlexArrayCreate(size_t data_size,
                                      size_t initial_capacity);
extern void arr_FlexArrayDelete(FlexArray *flex_array);
extern size_t arr_GetFlexArrayCapacity(FlexArray *flex_array);
extern void *arr_GetFlexArrayIndexValue(FlexArray *flex_array, size_t index);
extern StatusCode arr_SetFlexArrayIndexValue(FlexArray *flex_array, void *value,
                                             size_t index);
extern void arr_FlexArrayReset(FlexArray *flex_array);
/*
 * NOTE: Only use this in internal modules with proper bounds checking. This is
 * only meant for code where you are sure what you are doing and want to
 * optimize for performance. This is a dangerous function if mistakes happen
 * with the ret value usage.
 */
extern void *arr_GetFlexArrayRawData(FlexArray *flex_array);
extern StatusCode
arr_GrowFlexArray(FlexArray *flex_array,
                  size_t (*grow_callback)(size_t old_capacity));

/*
 * DESIGN RECOMMENDATION: The modules that use the AppendArray/FlexArray to
 * define their own data/data-structs are allowed to use the raw-data
 * functionality to optimize for performance. But for modules that use those
 * modules' data/data-structs are recommended to use the provided API and skip
 * on the raw-data functionality, to enforce safety.
 */
