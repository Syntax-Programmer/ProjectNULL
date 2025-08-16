#include "ecs/ecs.h"
#include "engine/engine.h"
#include <sys/time.h>

typedef struct {
  u64 x[50];
} AA;

i32 main() {
  struct timeval start, end;
  long mtime, seconds, useconds;

  gettimeofday(&start, NULL);

  engine_Init();

  PropId id1 = ecs_PropIdCreate(sizeof(AA));
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

  gettimeofday(&end, NULL);

  seconds = end.tv_sec - start.tv_sec;
  if (end.tv_usec < start.tv_usec) {
    seconds--;
    useconds = 1000000 + end.tv_usec - start.tv_usec;
  } else {
    useconds = end.tv_usec - start.tv_usec;
  }

  mtime = ((seconds) * 1000 + useconds / 1000.0) + 0.5;

  printf("Execution time: %ld milliseconds\n", mtime);

  return 0;
}
