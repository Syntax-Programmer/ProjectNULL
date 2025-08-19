#include "ecs/ecs.h"
#include "engine/engine.h"
#include "utils/common.h"
#include <sys/time.h>

typedef struct {
  u64 x[10000];
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

  // Layout *layout = ecs_LayoutCreate(signature);
  // volatile Entity *entity[1000000];
  // volatile u64 dummy_sum = 0;

  // for (u64 i = 0; i < 1000000; i++) {
  //   entity[i] = ecs_CreateEntityFromLayout(layout);
  //   dummy_sum += (u64)entity[i];
  // }
  // printf("%zu\n", dummy_sum);
  // for (u64 i = 0; i < 1000000; i++) {
  //   ecs_DeleteEntity((Entity *)entity[i]);
  // }

  Entity *entity[1000000];
  u64 sum = 0;

  for (u64 i = 0; i < 1000000; i++) {
    entity[i] = ecs_CreateEntity(signature, DUPLICATE_PROPS_SIGNATURE_KEEP);
  }

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
