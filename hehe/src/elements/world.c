#include "../../include/elements/world.h"
#include "../../include/utils/hashmap.h"
#include "../../include/utils/arena.h"

// #include "../../include/elements/world.h"
// #include <SDL2/SDL_image.h>

// #define IF_ASSET_COUNT_FOUND(s, n)
//   if (!strcmp(s, "ASSET_COUNT") && !strcmp(n, "count"))

// // Performance optimization to only check for t, f instead of "true", "false"
// #define STR_TO_BOOL(str) ((str[0] == 't') ? true : false)

// static int32_t ParseTilePropertisINI(void *user, const char *section,
//                                      const char *name, const char *value);
// static bool InitAssetProperties(TileProperties *pTile_props);

// static int32_t ParseTilePropertiesINI(void *user, const char *section,
//                                       const char *name, const char *value) {
//   TileProperties *props = (TileProperties *)user;

//   IF_ASSET_COUNT_FOUND(section, name) { props->size = atoi(value); }
//   else {
//     int32_t index = atoi(section);
//     if (!strcmp(name, "TILE_WALKABLE") && STR_TO_BOOL(value)) {
//       SET_FLAG(props->prop_flags[index], TILE_WALKABLE);
//     } else if (!strcmp(name, "TILE_SWIMMABLE") && STR_TO_BOOL(value)) {
//       SET_FLAG(props->prop_flags[index], TILE_SWIMMABLE);
//     } else if (!strcmp(name, "TILE_BREAKABLE") && STR_TO_BOOL(value)) {
//       SET_FLAG(props->prop_flags[index], TILE_BREAKABLE);
//     } else if (!strcmp(name, "TILE_INTRACTABLE") && STR_TO_BOOL(value)) {
//       SET_FLAG(props->prop_flags[index], TILE_INTRACTABLE);
//     } else if (!strcmp(name, "ASSET_PATH")) {
//       strcpy(props->asset_path[index], value);
//       printf("%s\n", props->asset_path[index]);
//     } else {
//       return 0;
//     }
//   }

//   return 1;
// }

// static bool InitAssetProperties(TileProperties *pTile_props) {
//   *pTile_props = (TileProperties){0};
//   char *asset_props_file_path = "properties/tile_props.ini";

// //   if (ini_parse(asset_props_file_path, ParseTilePropertiesINI, pTile_props) <
// //       0) {
// // #ifdef DEBUG
// //     fprintf(stderr, "Can't load '%s'\n", asset_props_file_path);
// // #endif
// //     *pTile_props = (TileProperties){0};
// //     return false;
// //   }

//   return true;
// }
