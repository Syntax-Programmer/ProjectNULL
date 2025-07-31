#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../ecs/ecs.h"

StatusCode engine_Init(void);
StatusCode engine_Exit(void);

#ifdef __cplusplus
}
#endif
