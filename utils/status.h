#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef enum {
  SUCCESS,
  WARNING,
  // Generic failure of code.
  FAILURE,
  /*
   * Return this when the failure is specifically due to null pointer. For
   * example a NULL argument to a function, or a null state pointer etc.
   *
   * do NOT use it for if a function returned NULL instead of valid ptr due to
   * its internal failure.
   */
  NULL_EXCEPTION,
  /*
   * This includes out of memory, or any other edge case that caused something
   * to not be created, where creation was the desired output.
   */
  CREATION_FAILURE,
  /*
   * Use this when either knowingly or unknowingly something fails to
   * clear bounds checks. Be it array index, hashmap key or whatever.
   */
  OUT_OF_BOUNDS_ACCESS,
  USE_AFTER_FREE
} StatusCode;

// A thread safe and generally better logger.
void status_Log(StatusCode code, const char *file_name,
                       const char *func_name, i32 line_num, const char *fmt,
                       ...);
#define STATUS_LOG(code, log, ...)                                             \
  status_Log(code, __FILE__, __func__, __LINE__, log, ##__VA_ARGS__)

#define IF_NULL(x) if (!(x))
#define IF_FUNC_FAILED(func_call) if ((func_call) != SUCCESS)

#define NULL_EXCEPTION_ROUTINE(x, ret_val, log, ...)                           \
  do {                                                                         \
    IF_NULL((x)) {                                                             \
      STATUS_LOG(NULL_EXCEPTION, log, ##__VA_ARGS__);                          \
      return ret_val;                                                          \
    }                                                                          \
  } while (0)

#define NULL_FUNC_ARG_ROUTINE(x, ret_val)                                      \
  NULL_EXCEPTION_ROUTINE(x, ret_val, "NULL argument '%s' provided.", #x)

/*
 * I am not adding clean up here as it can be arbitrary or not needed, that's
 * why its a sub routine.
 */
#define MEM_ALLOC_FAILURE_SUB_ROUTINE(x, ret_val)                              \
  do {                                                                         \
    STATUS_LOG(CREATION_FAILURE, "Failed to allocate memory for '%s'.", #x);   \
    return ret_val;                                                            \
  } while (0)
#define MEM_ALLOC_FAILURE_NO_CLEANUP_ROUTINE(x, ret_val)                       \
  do {                                                                         \
    IF_NULL((x)) {                                                             \
      STATUS_LOG(CREATION_FAILURE, "Failed to allocate memory for '%s'.", #x); \
      return ret_val;                                                          \
    }                                                                          \
  } while (0)

#ifdef __cplusplus
}
#endif
