#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"
#include "../utils/status.h"

/*
 * All the properties included here without the PROPS_INTERNALS definition.
 */

// Modular System, a typedef of ECS for more better name.

typedef struct __ModSysTmpl ModSysTmpl;

typedef struct __ModSysHandle ModSysHandle;

// This will currently only support 64 components, change it to struct for more.
typedef u64 ModSysProps;
#define MODSYS_PROPS_COUNT (64)
#define NO_PROP (0)

/* ----  TEMPLATE RELATED FUNCTIONS  ---- */

extern StatusCode modsys_StartTmplDefinition(void);
extern StatusCode modsys_CancelTmplDefinition(void);
extern StatusCode modsys_AttachPropsToTmpl(ModSysProps props);
extern ModSysTmpl *modsys_LockTmplDefinition(void);
extern StatusCode modsys_DeleteTmplDefinition(ModSysTmpl *tmpl);

/* ----  INIT/EXIT FUNCTIONS  ---- */

extern StatusCode modsys_Init(void);
extern StatusCode modsys_Exit(void);

#ifdef __cplusplus
}
#endif
