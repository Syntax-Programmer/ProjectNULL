#pragma once

#include "../common.h"

// Layout defined from file parsing.
typedef struct EntityLayout EntityLayout;

// Memory region where actual entity spawns happen.
typedef struct EntityPool EntityPool;

/*
 * A hot swappable module that contains a entity_layout and a entity_pool. You
 * can swap pools/layouts during runtime making it a pretty powerful tool.
 */
typedef struct EntityModule EntityModule;

/* ----  ENTITY_LAYOUT  ---- */

extern EntityLayout *ent_CreateEntityLayout(const char *layout_path);
extern void ent_DeleteEntityLayout(EntityLayout *entity_layout);

/* ----  ENTITY POOL  ---- */

extern EntityPool *ent_CreateEntityPool(size_t pool_capacity);
extern void ent_DeleteEntityPool(EntityPool *entity_pool);

/* ----  ENTITY_MODULE  ---- */

extern EntityModule *ent_CreateEntityModule();
extern EntityModule *ent_CreateFullEntityModule(size_t pool_capacity,
                                                const char *layout_path);
// NOTE: This will also delete any attached modules.
extern void ent_DeleteEntityModule(EntityModule *entity_module);
extern void ent_AttachEntityPoolToModule(EntityModule *entity_module,
                                         EntityPool *entity_pool);
extern void ent_AttachEntityLayoutToModule(EntityModule *entity_module,
                                           EntityLayout *entity_layout);
extern EntityPool *ent_DetathEntityPoolFromModule(EntityModule *entity_module);
extern EntityLayout *
ent_DetachEntityLayoutFromModule(EntityModule *entity_module);

/* ----  LOGIC  ---- */

extern StatusCode ent_SpawnEntityRandom(EntityModule *entity_module,
                                        float spawn_x, float spawn_y);
extern StatusCode ent_DespawnEntity(EntityModule *entity_module,
                                    uint32_t index);
extern void ent_HandleCollision(EntityModule *entity_module);
