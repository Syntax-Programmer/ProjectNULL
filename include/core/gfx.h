#pragma once

#include "../common.h"
#include "../elements/entities.h"
#include "../elements/maps.h"

extern bool gfx_InitSDL(SDL_Window **pWindow, SDL_Renderer **pRenderer);
extern void gfx_ExitSDL(SDL_Window **pWindow, SDL_Renderer **pRenderer);

extern void gfx_Render(SDL_Renderer *renderer,
    Entities *entities);
