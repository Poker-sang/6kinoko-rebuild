#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>

typedef struct KinokoCollisionState KinokoCollisionState;
typedef struct KinokoActor KinokoActor;
typedef struct KinokoActorManager KinokoActorManager;
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed inputs. Results borrow chip/placement storage, owned by the map.
   count selects the output cursor; the native buffer retains its high-water end.
   Internal success convention is 1/0, not the original HRESULT interface. */
int32_t kinoko_map_collision_append(KinokoCollisionState *state, int32_t *count,
    const unsigned char *chip, const void *placement, int32_t index);
int32_t kinoko_map_collision_query(KinokoCollisionState *state, KinokoActLayout *layout,
    int32_t *cached, int32_t left, int32_t top, int32_t right, int32_t bottom,
    int32_t *count);
KinokoActor *kinoko_collision_register_map(KinokoCollisionState *state, KinokoActLayout *layout);
int32_t kinoko_collision_query_actor_map(KinokoCollisionState *state, KinokoActLayout *layout,
    KinokoActor *actor, int32_t layer_index, int32_t *count);
int32_t kinoko_collision_move_actor(KinokoCollisionState *state, KinokoActor *actor, float dx, float dy);
/* Narrow bridges to the remaining C allocator and script-object consumer. */
KinokoActor *kinoko_actor_create_collision_proxy(KinokoActorManager *manager);
void kinoko_actor_set_collision_parent(KinokoActor *actor, KinokoActor *parent);
#ifdef __cplusplus
}
#endif
