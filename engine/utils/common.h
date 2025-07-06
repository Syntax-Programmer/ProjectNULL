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
#define INVALID_FUNCTION_ARGUMENTS (1)
#define MEMORY_ALLOCATION_FAILURE (2)
#define ATTEMPTED_INVALID_ACCESS (3)
#define CANNOT_EXECUTE (4)

extern void SetStatusCode(StatusCode codes);
extern const char *GetStatusCode();

#ifdef __cplusplus
}
#endif
