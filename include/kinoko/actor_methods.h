#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ECX receives this; the unused EDX parameter makes these compiler-generated
   fastcall entries compatible with the original thiscall script descriptors. */
int64_t __fastcall kinoko_actor_set_chip_flags(int32_t actor, void *unused, int32_t flags);
int32_t __fastcall kinoko_actor_set_chip_bound_type(int32_t actor, void *unused, uint16_t shape);
int32_t __fastcall kinoko_actor_get_chip_id(int32_t actor, void *unused, int32_t layer);
int32_t __fastcall kinoko_actor_reset_priority_method(int32_t actor, void *unused, int32_t priority);
int32_t __fastcall kinoko_actor_interrupt_collision(int32_t actor, void *unused);
int32_t __fastcall kinoko_actor_get_chip_flags(int32_t actor, void *unused);
int32_t __fastcall kinoko_actor_has_chip(int32_t actor, void *unused,
    float left, float top, float right, float bottom);

int32_t kinoko_actor_reset_priority(int32_t actor, int32_t priority);
int32_t __fastcall kinoko_actor_release(int32_t actor, void *unused);
int32_t kinoko_actor_set_init_data(int32_t actor, int32_t source);
int32_t __fastcall kinoko_actor_sync_animation(int32_t actor, void *unused,
    int32_t vtable, int32_t type, int32_t value);

#ifdef __cplusplus
}
#endif
