#pragma once

#include "../common.h"
#include <SDL2/SDL.h>

// #define TILE_SIZE 50

// typedef COMMON_PACKED_ENUM{
//     // If an entity can walk on the respective tile.
//     TILE_WALKABLE = 1 << 0,
//     // If an entity can swim on the given tile, triggering swimming physics.
//     TILE_SWIMMABLE = 1 << 1,
//     /*
//     This allows for any thing to be breakable or not regardless of
//     type. This allows a more flexible approach to handle break-ability, somewhat
//     in line with games like minecraft.
//     */
//     TILE_BREAKABLE = 1 << 2,
//     /*
//     Tells if a tile can be interacted with in any way, be it
//     moving it, using it etc.
//     */
//     TILE_INTRACTABLE = 1 << 3,
// } TilePropFlags;

// /*
// This entire struct other than the SDL_Texture * will always persist in memory,
// the SDL_Texture will be dynamically loaded/unloaded as we need.
// */
// typedef struct {
//   /*
//   All the metadata of all the assets will be loaded at the start, so no need to
//   keep check of len as it will be a full array of len = size.
//   */
//   size_t size;
//   /*
//   The data will be accessed and manipulated through the handy macros already
//   provided by the common.h file, namely HAS_FLAG(var, flag) and SET_FLAG(var,
//   flag).
//   */
//   TilePropFlags *prop_flags;
//   // Used to load assets when needed.
//   char **asset_path;
//   // This can be null telling that the asset is not loaded.
//   SDL_Texture **asset_textures;
// } TileProperties;

// /*
// Holds the actual map.
// The position and size of the tiles and the TileId of that particular tile.
// They are pointers to support an arbitrary size of map.
// */
// typedef struct {
//     /*
//     Actual tile area, It is a rect for arbitrary tile size, It is a FRect due to
//     consistency reasons.
//     */
//     SDL_FRect *tile_bounding_boxes;
//     /*
//     This tile ID gives the reference index to look up into struct TileProperties,
//     which tells us the properties of the tile and the asset it uses.
//     */
//     uint32_t *tile_id;
//     size_t len;
// } MapData;
