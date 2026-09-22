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
/* Refresh borrows all actors. The result preserves the legacy address token:
   last selected actor, or manager if none; nullptr for absent state/manager. */
void *kinoko_collision_refresh(KinokoCollisionState *state);
/* Release registered weak parents and reset logical map/actor counts. Retain
   storage and scratch ends; a null manager is rejected without mutation. */
int32_t kinoko_collision_reset(KinokoCollisionState *state, KinokoActorManager *manager);
uint32_t kinoko_collision_chip_flags(KinokoCollisionState *state, KinokoActor *actor);
int32_t kinoko_collision_has_chip(KinokoCollisionState *state, KinokoActor *actor,
    float left, float top, float right, float bottom);
int32_t kinoko_collision_event_at_point(KinokoMapManager *manager,
    int32_t x, int32_t y, uint32_t layer_index, int32_t *count);
void kinoko_actor_refresh_collision_bounds(KinokoActor *actor);
int32_t kinoko_actor_move(KinokoCollisionState *state, KinokoActor *actor, float dx, float dy);
int32_t kinoko_actor_update_motion(KinokoCollisionState *state, KinokoActor *actor);
int32_t kinoko_collision_dispatch_pair(KinokoActor *first, KinokoActor *second);
int32_t kinoko_collision_dispatch_all(KinokoActorManager *manager);
int32_t kinoko_collision_dispatch_actor(KinokoActorManager *manager, KinokoActor *actor);
/* Host diagnostic/update-mask bridges preserve the existing calls and order. */
void kinoko_actor_trace_motion(KinokoActor *actor, int32_t phase);
void kinoko_actor_trace_collision(KinokoActor *actor, int32_t after);
void kinoko_actor_clear_failed_collision_callback(KinokoActor *actor);
uint32_t kinoko_actor_motion_update_mask(void);
/* Narrow bridges to the remaining C allocator and script-object consumer. */
KinokoActor *kinoko_actor_create_collision_proxy(KinokoActorManager *manager);
void kinoko_actor_set_collision_parent(KinokoActor *actor, KinokoActor *parent);
#ifdef __cplusplus
}
#endif
