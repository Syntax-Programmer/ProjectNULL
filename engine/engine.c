#include "engine.h"
#include "../ecs/ecs.h"

StatusCode engine_Init(void) { return ecs_Init(); }
StatusCode engine_Exit(void) { return ecs_Exit(); }
