#include "../include/game.h"
#include "../include/engine/gfx.h"
#include "../include/engine/state.h"
#include "../include/utils/arena.h"
#include <time.h>

#define MAX_FPS 144
#define MIN_DT_MS (1000.0 / MAX_FPS)
#define MIN_DT_SEC (MIN_DT_MS / 1000.0)

#define MAX_SPAWN_POOL (48)
#define ENTITY_LAYOUT_FILE ("gamedata/properties/entity_properties.yaml")

static void GetDeltaTime(uint32_t *pStart_time, uint32_t *pFrame_c,
                         double *pDelta_time);

static StatusCode InitGame(SDL_Window **pWindow, SDL_Renderer **pRenderer);
static void GameLoop(SDL_Renderer *renderer);
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

static StatusCode InitGame(SDL_Window **pWindow, SDL_Renderer **pRenderer) {
  srand(time(NULL));
  if (arena_Init() == FATAL_ERROR ||
      gfx_InitSDL(pWindow, pRenderer) == FATAL_ERROR) {
    return FATAL_ERROR;
  }

  // *pEntity_module =
  //     ent_CreateFullEntityModule(MAX_SPAWN_POOL, ENTITY_LAYOUT_FILE);
  // if (!(*pEntity_module)) {
  //   /*
  //    * In future if any other module is created for various reasons, it
  //    * failing can't be classified as a fatal error, but failing to create a
  //    * single main one is sign of fatal error.
  //    */
  //   return FATAL_ERROR;
  // }

  return SUCCESS;
}

static void GameLoop(SDL_Renderer *renderer) {
  InputFlags input_flags = 0;
  /*
  Using a trick called frame counting, where instead of getting delta time each
  frame, we just take the avg over many frames.
  */
  double delta_time = MIN_DT_SEC;
  uint32_t frame_c = 0, fps_calc_start_time = SDL_GetTicks(),
           fps_limiter_time = SDL_GetTicks();

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
      state_HandleState(input_flags, delta_time);
      gfx_Render(renderer);
    }
  }
}

static void ExitGame(SDL_Window **pWindow, SDL_Renderer **pRenderer) {
  arena_Delete();
  gfx_ExitSDL(pWindow, pRenderer);
}

void Game(void) {
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;

  if (InitGame(&window, &renderer) != FATAL_ERROR) {
    GameLoop(renderer);
  }
  ExitGame(&window, &renderer);
}
