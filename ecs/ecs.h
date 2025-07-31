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
#define NO_PROPS (0)

/* ----  PROPS RELATED FUNCTIONS  ---- */

StatusCode ecs_AttachProp(EntityProps *props, EntityProps to_attach);
StatusCode ecs_DetachProp(EntityProps *props, EntityProps to_detach);
StatusCode ecs_ClearProps(EntityProps *props);

/* ----  LAYOUT RELATED FUNCTIONS  ---- */

EntityLayout *ecs_EntityLayoutCreate(EntityProps props);
StatusCode ecs_EntityLayoutDelete(EntityLayout *layout);
Entity *ecs_CreateEntityFromLayout(EntityLayout *layout);
StatusCode ecs_DeleteEntityFromLayout(Entity *entity);

/* ----  INIT/EXIT FUNCTIONS  ---- */

StatusCode ecs_Init(void);
StatusCode ecs_Exit(void);

#ifdef __cplusplus
}
#endif
