#include "../../include/core/state_mgr.h"

static void HandlePlayerMoving(SDL_FRect *pPlayer_bounding_box,
                               float player_base_speed, InputFlags input_flags,
                               double delta_time);

static void HandlePlayerMoving(SDL_FRect *pPlayer_bounding_box,
                               float player_base_speed, InputFlags input_flags,
                               double delta_time) {
  // If only one of them is true, their respective sign will make accurate ans.
  // If both are true then they cancel out and no moving in that direction.
  int32_t x_comp = HAS_FLAG(input_flags, RIGHT) - HAS_FLAG(input_flags, LEFT),
          y_comp = HAS_FLAG(input_flags, DOWN) - HAS_FLAG(input_flags, UP);

  // Normalization with squared term still achieves the same purpose.
  float magnitude_squared = (x_comp * x_comp) + (y_comp * y_comp);
  float delta_x = (x_comp * player_base_speed * delta_time),
        delta_y = (y_comp * player_base_speed * delta_time);

  if (magnitude_squared) {
    /*
    Squaring the delta_dist also to account for the squared magnitude.
    Everything is squared to avoid the sqrt(), which is a performance debt.
    */
    pPlayer_bounding_box->x +=
        ((delta_x >= 0) ? (delta_x * delta_x) : (delta_x * delta_x * -1)) /
        magnitude_squared;
    pPlayer_bounding_box->y +=
        ((delta_y >= 0) ? (delta_y * delta_y) : (delta_y * delta_y * -1)) /
        magnitude_squared;
    ;
  }
}

void state_HandleState(Entities *entities, InputFlags input_flags,
                       double delta_time) {
  HandlePlayerMoving(&entities->bounding_boxes[PLAYER_INDEX],
                     entities->speeds[PLAYER_INDEX], input_flags, delta_time);
  entity_HandleCollision(entities);
}

InputFlags GetInput() {
  InputFlags input_flags = 0;
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      input_flags = QUIT;
    }
  }

  /*
  We have to use states for things like movement keys. To provided continuous
  movement by holding the key.
  */
  const uint8_t *state = SDL_GetKeyboardState(NULL);

  if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP]) {
    SET_FLAG(input_flags, UP);
  }
  if (state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN]) {
    SET_FLAG(input_flags, DOWN);
  }
  if (state[SDL_SCANCODE_A] || state[SDL_SCANCODE_LEFT]) {
    SET_FLAG(input_flags, LEFT);
  }
  if (state[SDL_SCANCODE_D] || state[SDL_SCANCODE_RIGHT]) {
    SET_FLAG(input_flags, RIGHT);
  }

  return input_flags;
}
