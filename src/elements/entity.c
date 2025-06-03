#include "../../include/elements/entity.h"
#include "../../include/utils/arena.h"
#include "../../include/utils/hashmap.h"

struct EntityProps {
  char *name[DEFAULT_CHAR_BUFF_SIZE];
  uint16_t *speed;
  uint16_t *health;
  bool *is_intractable;
  SDL_Texture **asset;
  uint16_t *width;
  uint16_t *height;
  // void (**on_interact)(char name[DEFAULT_CHAR_BUFF_SIZE]);
};

void entity_GetProps(char name[DEFAULT_CHAR_BUFF_SIZE], uint16_t *pSpeed,
                     uint16_t *pHealth, bool *pIs_intractable,
                     SDL_Texture **pAsset, uint16_t *pWidth,
                     uint16_t *pHeight) {}
