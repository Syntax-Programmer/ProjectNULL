#include "../../include/elements/maps.h"
#include "../../include/ini.h"
#include <SDL2/SDL_image.h>
#include <string.h>

// static int32_t ParseTilePropertiesINI(void *user, const char *section,
//                                       const char *name, const char *value);
// static bool InitAssetProperties(AssetProperties *pProperties);

// static bool LoadMapData(SDL_Rect tile_bounding_boxes[TILE_COUNT],
//                         TileId tile_ids[TILE_COUNT], char *map_data_file_path);
// static void UnLoadAssets(SDL_Texture *asset_textures[TILE_ID_COUNT],
//                          bool is_asset_loaded[TILE_ID_COUNT]);
// static bool LoadRequiredMapAssets(SDL_Texture *asset_textures[TILE_ID_COUNT],
//                                   char *asset_names[TILE_ID_COUNT],
//                                   bool is_asset_loaded[TILE_ID_COUNT],
//                                   TileId tile_ids[TILE_COUNT],
//                                   SDL_Renderer *renderer);

// static int32_t ParseTilePropertiesINI(void *user, const char *section,
//                                       const char *name, const char *value) {
//   AssetProperties *props = (AssetProperties *)user;

// #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
//   if (MATCH("NO_TILE", "type")) {
//     props->types[NO_TILE] = NO_TYPE;
//   } else if (MATCH("GRASS", "type")) {
//     props->types[GRASS] = WALKABLE;
//   } else if (MATCH("WATER", "type")) {
//     props->types[WATER] = DECORATION_NON_MOVABLE;
//   } else if (MATCH("ROCK", "type")) {
//     props->types[ROCK] = DECORATION_MOVABLE;
//   } else if (MATCH("NO_TILE", "is_breakable")) {
//     props->is_breakable[NO_TILE] = false;
//   } else if (MATCH("GRASS", "is_breakable")) {
//     props->is_breakable[GRASS] = false;
//   } else if (MATCH("WATER", "is_breakable")) {
//     props->is_breakable[WATER] = false;
//   } else if (MATCH("ROCK", "is_breakable")) {
//     props->is_breakable[ROCK] = false;
//   } else if (MATCH("NO_TILE", "asset_name")) {
//     sscanf(value, "%s", props->asset_name[NO_TILE]);
//   } else if (MATCH("GRASS", "asset_name")) {
//     sscanf(value, "%s", props->asset_name[GRASS]);
//   } else if (MATCH("WATER", "asset_name")) {
//     sscanf(value, "%s", props->asset_name[WATER]);
//   } else if (MATCH("ROCK", "asset_name")) {
//     sscanf(value, "%s", props->asset_name[ROCK]);
//   } else {
//     return 0;
//   }

//   return 1;
// }

// static bool InitAssetProperties(AssetProperties *pProperties) {
//   *pProperties = (AssetProperties){0};
//   char *asset_props_file_path = "assets/asset_props.ini";

//   if (ini_parse(asset_props_file_path, ParseTilePropertiesINI, pProperties) <
//       0) {
// #ifdef DEBUG
//     fprintf(stderr, "Can't load '%s'\n", asset_props_file_path);
// #endif
//     *pProperties = (AssetProperties){0};
//     return false;
//   }
//   pProperties->is_setup = true;
//   // The following line tells that no asset is loaded yet.
//   memset(pProperties->is_asset_loaded, 0, sizeof(pProperties->is_asset_loaded));

//   return true;
// }

// static bool LoadMapData(SDL_Rect tile_bounding_boxes[TILE_COUNT],
//                         TileId tile_ids[TILE_COUNT], char *map_data_file_path) {
//   FILE *file = fopen(map_data_file_path, "rb");
//   if (!file) {
// #ifndef DEBUG
//     fprintf(stderr, "Unable to open file at the path: %s", map_data_file_path);
// #endif
//     return false;
//   }

//   return true;
// }

// static void UnLoadAssets(SDL_Texture *asset_textures[TILE_ID_COUNT],
//                          bool is_asset_loaded[TILE_ID_COUNT]) {
//   for (int32_t i = 0; i < TILE_ID_COUNT; i++) {
//     if (is_asset_loaded[i] && asset_textures[i]) {
//       SDL_DestroyTexture(asset_textures[i]);
//       asset_textures[i] = NULL;
//       is_asset_loaded[i] = false;
//     }
//   }
// }

// static bool LoadRequiredMapAssets(SDL_Texture *asset_textures[TILE_ID_COUNT],
//                                   char *asset_names[TILE_ID_COUNT],
//                                   bool is_asset_loaded[TILE_ID_COUNT],
//                                   TileId tile_ids[TILE_COUNT],
//                                   SDL_Renderer *renderer) {
//   /*
//   NOTE: There is room for potential optimization here, but leaving it at this
//   for now.
//   */
//   UnLoadAssets(asset_textures, is_asset_loaded);

//   TileId curr_id = NO_TILE;
//   /*
//   Currently looping over all the tiles used and then loading the ones we
//   encounter. This is a little inefficient as the tile_ids contain majorly
//   duplicate items but it will do for now.
//   */
//   for (int32_t i = 0; i < TILE_COUNT; i++) {
//     curr_id = tile_ids[i];
//     if (!is_asset_loaded[curr_id]) {
//       SDL_Surface *surface = IMG_Load(asset_names[curr_id]);
//       if (!surface) {
// #ifndef DEBUG
//         fprintf(stderr, "Unable to open asset at the path: %s",
//                 asset_names[curr_id]);
// #endif
//         return false;
//       }
//       asset_textures[curr_id] = SDL_CreateTextureFromSurface(renderer, surface);
//       is_asset_loaded[curr_id] = true;
//       SDL_FreeSurface(surface);
//     }
//   }

//   return true;
// }

// bool terrain_CreateMap(MapData *pMap_data, const char *map_name,
//                        SDL_Renderer *renderer) {
//   if (!pMap_data->asset_props.is_setup &&
//       !InitAssetProperties(&pMap_data->asset_props)) {
//     return false;
//   }

//   char map_data_file_path[512];
//   snprintf(map_data_file_path, sizeof(map_data_file_path), "%s/map_data.dat",
//            map_name);

//   LoadMapData(pMap_data->tile_bounding_boxes, pMap_data->tile_ids,
//               map_data_file_path);
//   LoadRequiredMapAssets(
//       pMap_data->asset_props.asset_textures, pMap_data->asset_props.asset_name,
//       pMap_data->asset_props.is_asset_loaded, pMap_data->tile_ids, renderer);

//   return true;
// }

// void terrain_DeleteMap(MapData *pMap_data) {
//   for (int32_t i = 0; i < TILE_COUNT; i++) {
//     pMap_data->tile_bounding_boxes[i] = (SDL_Rect){0};
//     pMap_data->tile_ids[i] = NO_TILE;
//   }
//   UnLoadAssets(pMap_data->asset_props.asset_textures,
//                pMap_data->asset_props.is_asset_loaded);
//   /*
//   Should never touch other asset_props, as they are meant to stay for the entire
//   runtime.
//   */
// }
