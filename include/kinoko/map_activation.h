#pragma once
#include <stdint.h>
#include "kinoko/act_types.h"

typedef struct KinokoActorManager KinokoActorManager;
typedef struct KinokoActor KinokoActor;
typedef struct KinokoSquirrelObject KinokoSquirrelObject;
struct SQVM;

#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed script wrappers. These functions never consume the caller's refs.
   Actor callbacks looked up within create_actors own one temporary external
   reference, released after each creation attempt. Layout records are borrowed
   only until a script call; the next iteration re-reads their buffer. */
int32_t kinoko_map_create_actors(KinokoActorManager *manager, KinokoActLayout *layout,
    const KinokoSquirrelObject *environment);
int32_t kinoko_map_create_events(KinokoMapManager *manager, struct SQVM *vm,
    const char *name, const KinokoSquirrelObject *callback,
    const KinokoSquirrelObject *environment);
/* Narrow bridge to the not-yet-migrated ActorManager allocator/initializer. */
KinokoActor *kinoko_actor_create_map_instance(KinokoActorManager *manager,
    const KinokoSquirrelObject *callback, float x, float y, int32_t chip_id,
    const unsigned char *initialization_data);
#ifdef __cplusplus
}
#endif
