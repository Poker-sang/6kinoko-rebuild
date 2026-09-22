#pragma once
#include "kinoko/actor_manager.h"
#include "kinoko/render_queue.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow manager iteration storage and camera. No Actor or layer ownership moves. */
void kinoko_actor_move_with_camera(KinokoActorManager *manager, KinokoCamera *camera, float dx, float dy);
KinokoRenderLayer *kinoko_actor_render_layer_object(KinokoActorManager *manager, uint32_t index);
void kinoko_actor_trace_render_layers(KinokoActorManager *manager);
void *kinoko_scene_create_render_layer(const char *name);
#ifdef __cplusplus
}
#endif
