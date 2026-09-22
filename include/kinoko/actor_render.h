#pragma once
#include "kinoko/actor_manager.h"
typedef struct KinokoAnimationFrame KinokoAnimationFrame;
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_actor_render(KinokoActor *actor, KinokoCamera *camera);
/* Device submission and diagnostics belong to the host; no ownership passes. */
int32_t kinoko_actor_render_trace_begin(KinokoActor *actor, KinokoCamera *camera);
void kinoko_actor_render_trace_draw(KinokoActor *actor);
void kinoko_actor_render_trace_end(int32_t trace_index, int32_t result);
void kinoko_actor_render_set_blend(int32_t mode);
int32_t kinoko_actor_render_submit(KinokoAnimationFrame *frame);
#ifdef __cplusplus
}
#endif
