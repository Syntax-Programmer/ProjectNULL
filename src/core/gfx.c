#include "../../include/core/gfx.h"
#include <SDL2/SDL_image.h>

static void RenderEntities(SDL_Renderer *renderer, Entities *pEntities);

bool gfx_InitSDL(SDL_Window **pWindow, SDL_Renderer **pRenderer) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER)) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize SDL, %s", SDL_GetError());
#endif
    return false;
  }
  if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize SDL, %s", SDL_GetError());
#endif
    return false;
  }

  *pWindow = SDL_CreateWindow(GAME_TITLE, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                              WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (!(*pWindow)) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize SDL Window, %s", SDL_GetError());
#endif
    return false;
  }

  *pRenderer = SDL_CreateRenderer(*pWindow, 0, SDL_RENDERER_ACCELERATED);
  if (!(*pRenderer)) {
#ifdef DEBUG
    fprintf(stderr, "Unable to initialize SDL Renderer, %s", SDL_GetError());
#endif
    return false;
  }

  return true;
}

void gfx_ExitSDL(SDL_Window **pWindow, SDL_Renderer **pRenderer) {
  if (pWindow && *pWindow) {
    SDL_DestroyWindow(*pWindow);
    *pWindow = NULL;
  }
  if (pRenderer && *pRenderer) {
    SDL_DestroyRenderer(*pRenderer);
    *pRenderer = NULL;
  }
  SDL_Quit();
}

static void RenderEntities(SDL_Renderer *renderer, Entities *pEntities) {
  for (int32_t i = 0, j = 0; i < pEntities->entity_occupied_slots.len; i++) {
    j = pEntities->entity_occupied_slots.arr[i];
    SDL_SetRenderDrawColor(
        renderer, pEntities->entity_colors[j].r, pEntities->entity_colors[j].g,
        pEntities->entity_colors[j].b, pEntities->entity_colors[j].a);
    SDL_RenderFillRectF(renderer, &pEntities->entity_bounding_boxes[j]);
  }
}

void gfx_Render(SDL_Renderer *renderer, Entities *pEntities) {
  SDL_RenderClear(renderer);
  RenderEntities(renderer, pEntities);
  SDL_SetRenderDrawColor(renderer, BLACKISH);
  SDL_RenderPresent(renderer);
}
