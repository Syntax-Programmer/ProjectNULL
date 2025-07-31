#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"
#include "../utils/status.h"

typedef struct __EntityLayout EntityLayout;
typedef struct __Entity Entity;

typedef u64 EntityProps;
#define BUILTIN_PROPS_COUNT (64)

/* ----  LAYOUT RELATED FUNCTIONS  ---- */

extern EntityLayout *ecs_EntityLayoutCreate(EntityProps props);
extern StatusCode ecs_EntityLayoutDelete(EntityLayout *layout);
extern Entity *ecs_CreateEntityFromLayout(EntityLayout *layout);
extern StatusCode ecs_DeleteEntityFromLayout(Entity *entity);

/* ----  INIT/EXIT FUNCTIONS  ---- */

extern StatusCode ecs_Init(void);
extern StatusCode ecs_Exit(void);

#ifdef __cplusplus
}
#endif
