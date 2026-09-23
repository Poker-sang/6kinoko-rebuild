#pragma once
#include "kinoko/actor_lifecycle.h"
#include <stdint.h>
typedef struct KinokoActor KinokoActor;
typedef struct KinokoActorManager KinokoActorManager;
typedef struct KinokoActorPool KinokoActorPool;
typedef struct KinokoCamera KinokoCamera;
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif
KinokoActorManager *kinoko_actor_manager_construct(KinokoActorManager *manager);
int32_t kinoko_actor_manager_initialize(KinokoActorManager *manager);
KinokoActor *kinoko_actor_manager_create(KinokoActorManager *manager,
    const KinokoOwnedObjectWords *callback, float x, float y, float z,
    const KinokoOwnedObjectWords *argument, const void *initial_data);
int32_t kinoko_actor_initialize(KinokoActor *actor, KinokoActorManager *manager,
    const KinokoOwnedObjectWords *callback, float x, float y, float z,
    const KinokoOwnedObjectWords *argument);
int32_t kinoko_actor_manager_refresh(KinokoActorManager *manager);
int32_t kinoko_actor_manager_update(KinokoActorManager *manager, KinokoCamera *camera);
int32_t kinoko_actor_render_layer_update(void *layer, KinokoCamera *camera);
int32_t kinoko_actor_manager_render_layer(KinokoActorManager *manager, KinokoCamera *camera, int32_t layer);
int32_t kinoko_actor_manager_reindex(KinokoActorManager *manager, KinokoActor *actor);
void *kinoko_actor_manager_clear_actors(KinokoActorManager *manager);
void *kinoko_actor_manager_reset(KinokoActorManager *manager);
int32_t kinoko_actor_activate(KinokoActor *actor, KinokoCamera *camera, float extent);
/* Dependencies at the legacy host boundary; renderer and script tick retain
   their own diagnostics. No ownership passes through these borrowed pointers. */
void kinoko_actor_manager_trace_actor(int32_t phase, KinokoActor *actor, KinokoCamera *camera, int32_t mask);
void kinoko_actor_tick(KinokoActor *actor);
int32_t kinoko_actor_trace_step_begin(KinokoActor *actor, int32_t callback_type);
void kinoko_actor_trace_step_end(KinokoActor *actor, int32_t result, int32_t trace_index);
void kinoko_actor_motion_host(KinokoActor *actor);
int32_t kinoko_actor_render_host(KinokoActor *actor, KinokoCamera *camera);
void kinoko_actor_manager_refresh_collision(void);
const void *kinoko_actor_owner_methods(void);
const void *kinoko_actor_render_layer_methods(void);
void *kinoko_actor_class_object(void);
void *kinoko_actor_user_key(void);
struct SQVM *kinoko_actor_default_vm(void);
#ifdef __cplusplus
}
#endif
