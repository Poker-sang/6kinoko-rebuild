#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t kinoko_actor_set_take(int32_t actor, int32_t take);
int32_t __fastcall kinoko_actor_set_take_method(int32_t actor, void *unused, int32_t take);
void kinoko_actor_advance_animation(int32_t actor, int32_t take_before_callback);
void kinoko_actor_sync_animation_state(int32_t actor, int32_t source);

#ifdef __cplusplus
}
#endif
