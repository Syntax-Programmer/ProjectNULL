#pragma once

#include "../common.h"
#include <SDL2/SDL.h>

// typedef enum {
//   // Player Or an entities can't see this and can't interact with it.
//   NO_TYPE,
//   // Entities will be on top of it walking'.
//   WALKABLE,
//   // Non Walkable tiles that will block entities in their path.
//   BLOCKERS,
//   // Walkable, but swim animation.
//   SWIMMABLE,
//   // Decorative items, that a player Or an entity can move.
//   DECORATION_MOVABLE,
//   // Decorative items, that a player Or an entity can not move.
//   DECORATION_NON_MOVABLE,
// } TileType;

// /*
// Property of all the tile ids. Guarantees to have the elements arranged in
// order in which TileId is laid out allowing for direct access through TileId.
// */
// typedef struct {
//   // Tells if this struct is already setup so that we don't redo it ever.
//   bool is_setup;
//   TileType types[TILE_ID_COUNT];
//   /*
//   This allows for any thing to be breakable or not regardless of type.
//   This allows a more flexible approach to handle break-ability, somewhat in
//   line with games like minecraft.
//   */
//   bool is_breakable[TILE_ID_COUNT];
//   /*
//   This will help us have the space accounting for all the assets but not load
//   them till they are needed. This will also preserve the element order
//   guarantee.
//   If an index i in this array is false, this implies that the SDL_Texture or
//   SDL_Color in the corresponding index is also NULL.
//   */
//   bool is_asset_loaded[TILE_ID_COUNT];
//   char *asset_name[TILE_ID_COUNT];
//   SDL_Texture *asset_textures[TILE_ID_COUNT];
// } AssetProperties;

// /*
// This will represent a singular default level/map.
// NOTE: Important : To load another one, either modify the preexisting one.
// */
// typedef struct {
//   SDL_Rect tile_bounding_boxes[TILE_COUNT];
//   /*
//   The tile id of the index corresponding to the tile_bounding_box.
//   This acts as a sort of foreign key into the tile_properties struct, which
//   will allow us to access that particular tile id's assets and stuff

//   Ex: To access the assets of the tile id at index 5, we do:
//   tile_properties.assets[tile_ids[5]];
//   */
//   TileId tile_ids[TILE_COUNT];

//   AssetProperties asset_props;
// } MapData;

// extern bool terrain_CreateMap(MapData *pMap_data, const char *map_name,
//                               SDL_Renderer *renderer);
// extern void terrain_DeleteMap(MapData *pMap_data);

typedef COMMON_PACKED_ENUM {
    TILE_WALKABLE = 1 << 0,
    TILE_BREAKABLE = 1 << 1,
    TILE_INTRACTABLE = 1 << 2,
} TilePropFlags;

typedef struct {
  /*
  Has the is_walkable, is_intractable and is_breakable flags in a single
  uint8_t rather than having three separate bool arrays for each of them. Has
  room for more optimization though HAHA.

  is_walkable: This will translate to swimming if TileId is WATER.

  is_intractable: Tells if a tile can be interacted with in any way, be it
  moving it, using it etc.

  is_breakable: This allows for any thing to be breakable or not regardless of
  type. This allows a more flexible approach to handle break-ability, somewhat
  in line with games like minecraft.

  The data will be accessed and manipulated through the handy macros already
  provided by the common.h file, namely HAS_FLAG(var, flag) and SET_FLAG(var,
  flag).
  */
  TilePropFlags *prop_flags;

  // Will be used. Maybe, Maybe Not.
  char **asset_path;
  SDL_Texture **asset_textures;
} TileProperties;

/*
Holds the actual map.
The position and size of the tiles and the TileId of that particular tile.
They are pointers to support an arbitrary size of map.
*/
typedef struct {
  /*
  Actual tile area, It is a rect for arbitrary tile size, It is a FRect due to
  consistency reasons.
  */
  SDL_FRect *tile_bounding_boxes;
  /*
  This tile ID gives the reference index to look up into struct TileProperties,
  which tells us the properties of the tile and the asset it uses.
  */
  uint32_t *tile_id;
  size_t tile_count;
} TileData;

typedef struct {
  TileData tile_data;
  /*
  There will be a cool optimization done here where, when we are changing the
  maps, while loading new assets, if the one we need already exists, we skip its
  creation and let the existing one persist instead of erasing it.
  */
  TileProperties tile_props;
} MapData;
