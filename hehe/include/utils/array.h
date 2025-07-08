#pragma once

#include "../common.h"

// This is a sort of pythonic list. If you know what I mean.
typedef struct AppendArr AppendArr;

// This is C style resizable arrays.
typedef struct FlexArr FlexArr;

/* ----  APPEND ARRAY  ---- */

extern AppendArr *arr_AppendArrCreate(size_t data_size);
extern AppendArr *arr_SizedAppendArrCreate(size_t data_size,
                                               size_t capacity);
extern void arr_AppendArrDelete(AppendArr *append_array);
extern size_t arr_GetAppendArrLen(AppendArr *append_array);
extern size_t arr_GetAppendArrCapacity(AppendArr *append_array);
extern void *arr_GetAppendArrIndexValue(AppendArr *append_array,
                                          size_t index);
extern StatusCode arr_SetAppendArrIndexValue(AppendArr *append_array,
                                               void *value, size_t index);
extern StatusCode
arr_AppendArrPush(AppendArr *append_array, void *value,
                    size_t (*grow_callback)(size_t old_capacity));
extern void arr_AppendArrPop(AppendArr *append_array);
extern StatusCode arr_AppendArrShrinkToFit(AppendArr *append_array);
extern void arr_AppendArrReset(AppendArr *append_array);
/*
 * NOTE: Only use this in internal modules with proper bounds checking. This is
 * only meant for code where you are sure what you are doing and want to
 * optimize for performance. This is a dangerous function if mistakes happen
 * with the ret value usage.
 */
extern void *arr_GetAppendArrRawData(AppendArr *append_array);

/* ----  FLEX ARRAY  ---- */

extern FlexArr *arr_FlexArrCreate(size_t data_size,
                                      size_t initial_capacity);
extern void arr_FlexArrDelete(FlexArr *flex_array);
extern size_t arr_GetFlexArrCapacity(FlexArr *flex_array);
extern void *arr_GetFlexArrIndexValue(FlexArr *flex_array, size_t index);
extern StatusCode arr_SetFlexArrIndexValue(FlexArr *flex_array, void *value,
                                             size_t index);
extern void arr_FlexArrReset(FlexArr *flex_array);
/*
 * NOTE: Only use this in internal modules with proper bounds checking. This is
 * only meant for code where you are sure what you are doing and want to
 * optimize for performance. This is a dangerous function if mistakes happen
 * with the ret value usage.
 */
extern void *arr_GetFlexArrRawData(FlexArr *flex_array);
extern StatusCode
arr_GrowFlexArr(FlexArr *flex_array,
                  size_t (*grow_callback)(size_t old_capacity));

/*
 * DESIGN RECOMMENDATION: The modules that use the AppendArr/FlexArr to
 * define their own data/data-structs are allowed to use the raw-data
 * functionality to optimize for performance. But for modules that use those
 * modules' data/data-structs are recommended to use the provided API and skip
 * on the raw-data functionality, to enforce safety.
 */
