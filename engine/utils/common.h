#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;
typedef float f32;
typedef double f64;

#define CLAMP(val, min, max)                                                   \
  ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SWAP(type, a, b)                                                       \
  do {                                                                         \
    type t = a;                                                                \
    a = b;                                                                     \
    b = t;                                                                     \
  } while (0)
#define MEM_OFFSET(mem, offset) ((u8 *)(mem) + (offset))
#define IS_POWER_OF_TWO(n) (((n) != 0) && (((n) & ((n) - 1)) == 0))

#define SET_FLAG(var, flag) ((var) |= (flag))
#define CLEAR_FLAG(var, flag) ((var) &= ~(flag))
#define TOGGLE_FLAG(var, flag) ((var) ^= (flag))
#define HAS_FLAG(var, flag) (((var) & (flag)) != 0)
#define HAS_ALL_FLAGS(var, required_flags)                                     \
  (((var) & (required_flags)) == (required_flags))

#if defined(_MSC_VER) && !defined(__clang__)
#define PACKED_ENUM __pragma(pack(push, 1)) enum __pragma(pack(pop))
#else
#define PACKED_ENUM enum __attribute__((__packed__))
#endif // defined(_MSC_VER) && !defined(__clang__)

#define REQUIRE(expr) assert((expr) && "REQUIRE failed: " #expr)

typedef u8 StatusCode;

#define SUCCESS (0)
#define FAILURE (1)

/*
 * The __VA_ARGS__ are some additional logic, or cleanup functions that can be
 * added to it.
 */
#define CHECK_NULL_VAR(var, ret_val, ...)                                      \
  do {                                                                         \
    if (!(var)) {                                                              \
      printf("NULL var '%s' during execution of %s\n", #var, __func__);        \
      __VA_ARGS__;                                                             \
      return ret_val;                                                          \
    }                                                                          \
  } while (0)

#define CHECK_NULL_ARG(arg, ret_val, ...)                                      \
  do {                                                                         \
    if (!(arg)) {                                                              \
      printf("NULL argument '%s' passed to %s\n", #arg, __func__);             \
      __VA_ARGS__;                                                             \
      return ret_val;                                                          \
    }                                                                          \
  } while (0)

#define CHECK_ALLOC_FAILURE(ptr, ret_val, ...)                                 \
  do {                                                                         \
    if (!(ptr)) {                                                              \
      printf("Memory allocation failure of '%s' in function %s\n", #ptr,       \
             __func__);                                                        \
      __VA_ARGS__;                                                             \
      return ret_val;                                                          \
    }                                                                          \
  } while (0)

#define CHECK_FUNCTION_FAILURE(func, ret_val, ...)                             \
  do {                                                                         \
    if ((func) != SUCCESS) {                                                   \
      printf("Function '%s' failed in the caller %s\n", #func, __func__);      \
      __VA_ARGS__;                                                             \
      return ret_val;                                                          \
    }                                                                          \
  } while (0)

#ifdef __cplusplus
}
#endif
