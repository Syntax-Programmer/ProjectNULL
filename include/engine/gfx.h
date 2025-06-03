#pragma once

#include "../common.h"
#include "../elements/entity.h"
#include "../elements/world.h"

extern StatusCode gfx_InitSDL(SDL_Window **pWindow, SDL_Renderer **pRenderer);
extern void gfx_ExitSDL(SDL_Window **pWindow, SDL_Renderer **pRenderer);

extern void gfx_Render(SDL_Renderer *renderer);
