#include "ecs/ecs.h"
#include "engine/engine.h"

i32 main() {
  engine_Init();

  PropId id1 = ecs_PropIdCreate(sizeof(u64));
  PropId id2 = ecs_PropIdCreate(sizeof(void *));

  PropsSignature *signature = ecs_PropSignatureCreate();
  ecs_HandlePropIdToPropSignatures(signature, id1, PROP_SIGNATURE_ATTACH);
  ecs_HandlePropIdToPropSignatures(signature, id2, PROP_SIGNATURE_ATTACH);
  ecs_HandlePropIdToPropSignatures(signature, id1, PROP_SIGNATURE_DETACH);
  ecs_HandlePropIdToPropSignatures(signature, id1, PROP_SIGNATURE_ATTACH);

  ecs_HandlePropIdToPropSignatures(signature, id1, PROP_SIGNATURE_DETACH);
  ecs_HandlePropIdToPropSignatures(signature, id2, PROP_SIGNATURE_DETACH);

  ecs_HandlePropIdToPropSignatures(signature, id1, PROP_SIGNATURE_ATTACH);

  Layout *layout = ecs_LayoutCreate(signature);
  Entity *entity[100];
  for (u64 i = 0; i < 100; i++) {
    entity[i] = ecs_CreateEntityFromLayout(layout);
  }
  for (u64 i = 0; i < 100; i++) {
    ecs_DeleteEntityFromLayout(entity[i]);
  }

  ecs_LayoutDelete(layout);

  engine_Exit();

  return 0;
}
