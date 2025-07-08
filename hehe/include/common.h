#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 255
#define WHITISH 200, 200, 200, 255
#define BLACKISH 50, 50, 50, 255
#define REDDISH 255, 128, 128, 255
#define GREENISH 128, 255, 128, 255
#define BLUISH 128, 128, 255, 255

#define SET_FLAG(var, flag) ((var) |= (flag))
#define CLEAR_FLAG(var, flag) ((var) &= ~(flag))
#define TOGGLE_FLAG(var, flag) ((var) ^= (flag))
/*
 * WARNING: Do not change this to something like
 *
 * #define HAS_FLAG(var, flag) ((var) & (flag))
 *
 * In functions like static void HandleMoving() it will break movement due to
 * optimized approach to calculating movement components.
 */
#define HAS_FLAG(var, flag) (((var) & (flag)) != 0)
#define HAS_ALL_FLAGS(var, required_flags) (((var) & (required_flags)) == (required_flags))

/*
 * A more memory-efficient enum for when the total number of entries is ≤ 255.
 * This macro ensures the enum is stored as a single `uint8_t`.
 */
#if defined(_MSC_VER) && !defined(__clang__)
#define PACKED_ENUM __pragma(pack(push, 1)) enum __pragma(pack(pop))
#else
#define PACKED_ENUM enum __attribute__((__packed__))
#endif // defined(_MSC_VER) && !defined(__clang__)

#define CHAR_BUFFER_SIZE (64)

typedef PACKED_ENUM{
    SUCCESS,
    CAN_NOT_EXECUTE,    // Some condition making the code not execute fully
    WARNING,            // Non-critical issue
    RESOURCE_EXHAUSTED, // Allocation not possible, but existing data is valid
    FAILURE,            // Critical error
    FATAL_ERROR         // Unrecoverable
} StatusCode;

#ifdef DEBUG
#define LOG(fmt, ...)                                                          \
  do {                                                                         \
    printf("Log: ");                                                           \
    printf(fmt, ##__VA_ARGS__);                                                \
    printf("\n");                                                              \
  } while (0)
#else
#define LOG(...)
#endif

typedef char CharBuffer[CHAR_BUFFER_SIZE];

#define CHARBUFF_EQUALS(tok, val) (!strncmp((tok), (val), CHAR_BUFFER_SIZE))
#define CHARBUFF_NOT_EQUALS(tok, val) (!CHARBUFF_EQUALS(tok, val))
#define IF_CHARBUFF_EQUALS(tok, val) if (CHARBUFF_EQUALS(tok, val))
#define IF_CHARBUFF_NOT_EQUALS(tok, val) if (CHARBUFF_NOT_EQUALS(tok, val))
#define STR_TO_BOOL(str)                                                       \
  ((!strcasecmp(str, "true") || !strcmp(str, "1")) ? true : false)

typedef struct {
  int32_t x, y;
} IVec2;

typedef struct {
  float x, y;
} FVec2;

typedef struct {
  int32_t w, h;
} Dimension2;

typedef IVec2 ICoord2;
typedef FVec2 FCoord2;
