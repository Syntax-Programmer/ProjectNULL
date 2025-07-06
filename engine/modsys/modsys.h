#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"

/*
 * All the properties included here without the INTERNALS definition.
 */

// Modular System, a typedef of ECS for more better name.

typedef struct __ModSys ModSys;

typedef u16 ModSysHandle;

typedef enum {
  /*
   * MODSYS_HEALTH,
   * MODSYS_PHYSICS,
   * ...
   */
   X
} ModSysProps;

#ifdef __cplusplus
}
#endif
