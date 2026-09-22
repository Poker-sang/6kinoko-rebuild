#pragma once

#include <stdint.h>
#include "kinoko/actor_lifecycle.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t kinoko_actor_set_take(KinokoActor *actor, int32_t take);
int32_t __fastcall kinoko_actor_set_take_method(KinokoActor *actor, void *unused, int32_t take);
void kinoko_actor_advance_animation(KinokoActor *actor, int32_t take_before_callback);
void kinoko_actor_sync_animation_state(KinokoActor *actor, KinokoActor *source);

#ifdef __cplusplus
}
#endif
