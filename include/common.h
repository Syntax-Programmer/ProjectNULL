#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define GAME_TITLE "Game"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 255
#define WHITISH 200, 200, 200, 255
#define BLACKISH 50, 50, 50, 255
#define REDDISH 255, 128, 128, 255
#define GREENISH 128, 255, 128, 255
#define BLUISH 128, 128, 255, 255

typedef struct {
  float x, y;
} Vec2;

#define SET_FLAG(var, flag) ((var) |= (flag))
#define CLEAR_FLAG(var, flag) ((var) &= ~(flag))
#define TOGGLE_FLAG(var, flag) ((var) ^= (flag))
/*
WARNING: Do not change this to something like

#define HAS_FLAG(var, flag) ((var) & (flag))

In functions like static void HandleMoving() it will break movement due to
optimized approach to calculating movement components.
*/
#define HAS_FLAG(var, flag) (((var) & (flag)) != 0)

#define MAX_FPS 60
#define MIN_DT_MS (1000.0 / MAX_FPS)
#define MIN_DT_SEC (MIN_DT_MS / 1000.0)

/*
A more memory-efficient enum for when the total number of entries is ≤ 255.
This macro ensures the enum is stored as a single `uint8_t`.
*/
#if defined(_MSC_VER) && !defined(__clang__)
#define COMMON_PACKED_ENUM __pragma(pack(push, 1)) enum __pragma(pack(pop))
#else
#define COMMON_PACKED_ENUM enum __attribute__((__packed__))
#endif // defined(_MSC_VER) && !defined(__clang__)

typedef struct {
  // Actual arena memory
  uint8_t *mem;
  // The total size of the arena.
  size_t size;
  // Gives the offset for the new data to be stored at.
  size_t offset;
} Arena;

extern Arena common_arena;

extern bool common_InitArena();
extern size_t common_AllocData(size_t data_size);
extern bool common_SetData(uint8_t *data, size_t data_offset, size_t data_size);
extern uint8_t *common_FetchData(size_t data_offset, size_t data_size);
void common_FreeArena();
