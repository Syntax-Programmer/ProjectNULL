#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

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

/*
 * A more memory-efficient enum for when the total number of entries is ≤ 255.
 * This macro ensures the enum is stored as a single `uint8_t`.
 */
#if defined(_MSC_VER) && !defined(__clang__)
#define COMMON_PACKED_ENUM __pragma(pack(push, 1)) enum __pragma(pack(pop))
#else
#define COMMON_PACKED_ENUM enum __attribute__((__packed__))
#endif // defined(_MSC_VER) && !defined(__clang__)

#define DEFAULT_STR_BUFFER_SIZE (64)

typedef COMMON_PACKED_ENUM{SUCCESS, FAILURE} StatusCode;

#ifdef DEBUG
#define LOG(...) printf("Log: "); printf(__VA_ARGS__); printf("\n");
#else
#define LOG(...)
#endif

#define MATCH_TOKEN(tok, value) if (!strcmp(tok, value))
#define STR_TO_BOOL(str)                                                       \
  ((!strcasecmp(str, "true") || !strcmp(str, "1")) ? true : false)
