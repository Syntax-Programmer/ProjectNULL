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

extern StatusCode ecs_AttachProp(EntityProps *props, EntityProps to_attach);
extern StatusCode ecs_DetachProp(EntityProps *props, EntityProps to_detach);
extern StatusCode ecs_ClearProps(EntityProps *props);

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
