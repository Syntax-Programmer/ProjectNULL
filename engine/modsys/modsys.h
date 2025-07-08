#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"

/*
 * All the properties included here without the INTERNALS definition.
 */

// Modular System, a typedef of ECS for more better name.

typedef struct __ModSysTemplate ModSysTemplate;

typedef struct __ModSysHandle ModSysHandle;

typedef enum {
  NO_PROPS = 0,
  // Must be a bitflag.
} ModSysProps;

#ifdef __cplusplus
}
#endif
