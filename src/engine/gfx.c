#include "../../include/engine/gfx.h"
#include <SDL2/SDL_image.h>

#define GAME_TITLE "ProjectNULL"
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

// static void RenderEntities(SDL_Renderer *renderer, Entities *entities);

SDL_Renderer *common_renderer = NULL;

StatusCode gfx_InitSDL(SDL_Window **pWindow, SDL_Renderer **pRenderer) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER)) {
    LOG("Unable to initialize SDL, %s", SDL_GetError());
    return FATAL_ERROR;
  }
  if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
    LOG("Unable to initialize SDL, %s", SDL_GetError());
    gfx_ExitSDL(pWindow, pRenderer);
    return FATAL_ERROR;
  }

  *pWindow = SDL_CreateWindow(GAME_TITLE, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                              WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (!(*pWindow)) {
    LOG("Unable to initialize SDL Window, %s", SDL_GetError());
    gfx_ExitSDL(pWindow, pRenderer);
    return FATAL_ERROR;
  }

  *pRenderer = SDL_CreateRenderer(*pWindow, 0, SDL_RENDERER_ACCELERATED);
  if (!(*pRenderer)) {
    LOG("Unable to initialize SDL Renderer, %s", SDL_GetError());
    gfx_ExitSDL(pWindow, pRenderer);
    return FATAL_ERROR;
  }

  common_renderer = *pRenderer;

  return SUCCESS;
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
  IMG_Quit();
  SDL_Quit();
}

// static void RenderEntities(SDL_Renderer *renderer, Entities *entities) {
//   for (size_t i = 0, j = 0; i < entities->occupied_slots.len; i++) {
//     j = entities->occupied_slots.arr[i];
//     SDL_SetRenderDrawColor(renderer, entities->colors[j].r,
//                            entities->colors[j].g, entities->colors[j].b,
//                            entities->colors[j].a);
//     SDL_RenderFillRectF(renderer, &entities->bounding_boxes[j]);
//   }
// }

void gfx_Render(SDL_Renderer *renderer) {
  SDL_RenderClear(renderer);
  // RenderEntities(renderer, entities);
  SDL_SetRenderDrawColor(renderer, BLACKISH);
  SDL_RenderPresent(renderer);
}
