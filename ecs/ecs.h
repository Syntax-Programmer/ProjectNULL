#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/common.h"
#include "../utils/status.h"

typedef struct __Layout Layout;
typedef struct __Entity Entity;
/*
 * A vector of u64s, which represents all the PropIds attached to it, by
 * storing them in a bitset.
 *
 * Here the first u64 will hold the first 64 PropIds, while the next u64 will
 * hold PropIds from 65-128, and so on.
 *
 * This allows for very quick prop comparisons compared to storing an array of
 * PropIds.
 */
typedef struct __PropsSignature PropsSignature;
/*
 * A normal counter representing the len of the registered components, that acts
 * as two things at the same time.
 * 1. An index into the metadata table(which is a structure of vectors itself.).
 * 2. The bit position that represents at which bit location this prop fits
 * in the propsSignature.
 */
typedef u64 PropId;

#define INVALID_PROP_ID ((u64)(-1))

/* ----  PROP RELATED FUNCTIONS  ---- */

typedef enum {
  PROP_SIGNATURE_ATTACH,
  PROP_SIGNATURE_DETACH
} PropsSignatureHandleMode;

PropId ecs_PropIdCreate(u64 prop_struct_size);
PropsSignature *ecs_PropSignatureCreate(void);
StatusCode ecs_PropsSignatureDelete(PropsSignature *signature);
StatusCode ecs_HandlePropIdToPropSignatures(PropsSignature *signature,
                                            PropId id,
                                            PropsSignatureHandleMode mode);
StatusCode ecs_PropSignatureClear(PropsSignature *signature);

/* ----  LAYOUT RELATED FUNCTIONS  ---- */

typedef enum {
  DUPLICATE_PROPS_SIGNATURE_FREE,
  DUPLICATE_PROPS_SIGNATURE_KEEP
} DuplicatePropsSignatureHandleMode;

Layout *ecs_LayoutCreate(PropsSignature *signature,
                         DuplicatePropsSignatureHandleMode mode);
StatusCode ecs_LayoutDelete(Layout *layout);

/* ----  ENTITY RELATED FUNCTIONS  ---- */

Entity *ecs_CreateEntityFromLayout(Layout *layout);
Entity *ecs_CreateEntity(PropsSignature *signature,
                         DuplicatePropsSignatureHandleMode mode);
StatusCode ecs_DeleteEntity(Entity *entity);
void *ecs_GetPropDataFromEntity(Entity *entity, PropId id);

/* ----  INIT/EXIT FUNCTIONS  ---- */

StatusCode ecs_Init(void);
StatusCode ecs_Exit(void);

#ifdef __cplusplus
}
#endif
