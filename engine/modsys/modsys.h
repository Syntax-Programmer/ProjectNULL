#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"

/*
 * All the properties included here without the PROPS_INTERNALS definition.
 */

// Modular System, a typedef of ECS for more better name.

typedef struct __ModSysTmpl ModSysTmpl;

typedef struct __ModSysHandle ModSysHandle;

// This will currently only support 64 components, change it to struct for more.
typedef i64 ModSysProps;
#define MODSYS_PROPS_COUNT (0)

extern StatusCode modsys_Init(void);
extern StatusCode modsys_Exit(void);

#ifdef __cplusplus
}
#endif
