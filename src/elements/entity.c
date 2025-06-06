#include "../../include/elements/entity.h"
#include "../../include/utils/arena.h"

struct EntityProps {
  //StrHashmap *names;
  uint16_t *speeds;
  uint16_t *healths;
  bool *is_intractable;
  SDL_Texture **assets;
  uint16_t *widths;
  uint16_t *heights;
  // void (**on_interact)(char name[DEFAULT_CHAR_BUFF_SIZE]);
};

void entity_GetProps(FixedSizeString name, uint16_t *pSpeed,
                     uint16_t *pHealth, bool *pIs_intractable,
                     SDL_Texture **pAsset, uint16_t *pWidth,
                     uint16_t *pHeight) {}
