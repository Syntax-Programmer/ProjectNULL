#include "../include/game.h"
#include "../include/core/gfx.h"
#include "../include/core/state_mgr.h"

static void GetDeltaTime(uint32_t *pStart_time, uint32_t *pFrame_c,
                         double *pDelta_time);

static bool InitGame(SDL_Window **pWindow, SDL_Renderer **pRenderer,
                     Entities *pEntities);
static void GameLoop(SDL_Renderer *renderer, Entities *pEntities);
static void ExitGame(SDL_Window **pWindow, SDL_Renderer **pRenderer);

static void GetDeltaTime(uint32_t *pStart_time, uint32_t *pFrame_c,
                         double *pDelta_time) {
  uint32_t diff;

  if ((diff = (SDL_GetTicks() - *pStart_time)) >= 500) {
    *pDelta_time = diff / (*pFrame_c * 1000.0);
    if (*pDelta_time < MIN_DT_SEC) {
      /*
      The minimum delta time for the code, essentially clamping the fps.
      As all the stuff in the code is clamped to max fps, it makes sense to
      clamp delta time here also, otherwise the experienced FPS will be 60 but
      the delta time will be calculated for the actual fps, causing a mismatch.
      */
      *pDelta_time = MIN_DT_SEC;
    }
    *pFrame_c = 0;
    *pStart_time = SDL_GetTicks();
  }
}

static bool InitGame(SDL_Window **pWindow, SDL_Renderer **pRenderer,
                     Entities *pEntities) {
  entity_InitEntities(pEntities);

  return common_InitArena() && gfx_InitSDL(pWindow, pRenderer);
}

static void GameLoop(SDL_Renderer *renderer, Entities *pEntities) {
  InputFlags input_flags = 0;
  /*
  Using a trick called frame counting, where instead of getting delta time each
  frame, we just take the avg over many frames.
  */
  double delta_time = MIN_DT_SEC;
  uint32_t frame_c = 0, fps_calc_start_time = SDL_GetTicks(),
           fps_limiter_time = SDL_GetTicks();

  entity_SpawnEntity(pEntities, NPC, (SDL_FRect){500, 500, 30, 30},
                     (SDL_Color){0, 0, 123, 255}, 200, 21);
  entity_SpawnEntity(pEntities, NPC, (SDL_FRect){250, 500, 30, 30},
                     (SDL_Color){0, 0, 123, 255}, 200, 21);
  entity_SpawnEntity(pEntities, NPC, (SDL_FRect){700, 500, 30, 30},
                     (SDL_Color){0, 0, 123, 255}, 200, 21);
  entity_SpawnEntity(pEntities, NPC, (SDL_FRect){100, 500, 30, 30},
                     (SDL_Color){0, 0, 123, 255}, 200, 21);
  entity_SpawnEntity(pEntities, NPC, (SDL_FRect){3, 100, 50, 50},
                     (SDL_Color){0, 0, 123, 255}, 200, 21);

  while (true) {
    GetDeltaTime(&fps_calc_start_time, &frame_c, &delta_time);
    frame_c++;

    input_flags = GetInput();
    if (HAS_FLAG(input_flags, QUIT)) {
      return;
    }

    /*
    FPS Clamping on all the stuff other than input polling.
    Input polling is not clamped to provide good responsiveness and crisp input.
    */
    if (SDL_GetTicks() - fps_limiter_time >= MIN_DT_MS) {
      fps_limiter_time = SDL_GetTicks();
      state_HandleState(pEntities, input_flags, delta_time);
      gfx_Render(renderer, pEntities);
    }
  }
}

static void ExitGame(SDL_Window **pWindow, SDL_Renderer **pRenderer) {
  gfx_ExitSDL(pWindow, pRenderer);
}

void Game(void) {
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  Entities entities;

  if (InitGame(&window, &renderer, &entities)) {
    GameLoop(renderer, &entities);
  }
  ExitGame(&window, &renderer);
}
