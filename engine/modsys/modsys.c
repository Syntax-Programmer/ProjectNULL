#include "modsys.h"
#include "../types/array.h"

#define INTERNALS
/*
 * All the properties included here.
 */
#undef INTERNALS

struct __ModSys {
  /*
   * BuffArr *prop1;
   * BuffArr *prop2;
   * ...
   */
  BuffArr *prop_masks;
  Vector *empty_slots;
  Vector *occupied_slots;
};

#define DEFAULT_POOL_CAP (16)

static ModSys *__curr_bound_system = NULL;

// Users should always create a different ModSys for different concerns.
ModSys *modsys_CreateCustomSystem(u64 cap, ModSysProps props) {
  ModSys *sys = malloc(sizeof(ModSys));
  if (!sys) {
    SetStatusCode(MEMORY_ALLOCATION_FAILURE);
    return NULL;
  }

  /*
   * if (HAS_FLAG(props, prop1)) {
      sys.prop1 = arr_create();
      }
      if (HAS_FLAG(props, prop2)) }
      sys.prop2 = arr_create();
      }
      ...
   */

  return sys;
}

ModSys *modsys_CreateDefaultSystem() {
  return modsys_CreateCustomSystem(DEFAULT_POOL_CAP, ALL_PROPS);
}

StatusCode modsys_DeleteSystem();

/*
 * The following functions will act on the currently binded system. This is to
 * reduce argument clutter.
 */
void modsys_BindSystem(ModSys *system) { __curr_bound_system = system; }

ModSysHandle modsys_CreateEntry();
StatusCode modsys_DeleteEntry(ModSysHandle handle);

StatusCode modsys_AttachProperty(ModSysHandle handle, ModSysProps prop,
                                 void *prop_data);
StatusCode modsys_DetachProperties(ModSysHandle handle, ModSysProps props);
StatusCode modsys_ClearProperties(ModSysHandle handle);

void *modsys_GetPropDataPtr(ModSysHandle handle, ModSysProps prop);
StatusCode modsys_GetPropData(ModSysHandle handle, ModSysProps prop,
                              void *dest);
StatusCode modsys_SetPropData(ModSysHandle handle, ModSysProps prop,
                              void *data);

StatusCode modsys_PropForEach(ModSysProps prop,
                              void (*foreach_callback)(void *prop));
