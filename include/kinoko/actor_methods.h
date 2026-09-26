#pragma once

#include <stdint.h>
typedef struct KinokoActor KinokoActor;

#ifdef __cplusplus
extern "C" {
#endif

/* ECX receives this; the unused EDX parameter makes these compiler-generated
   fastcall entries compatible with the original thiscall script descriptors. */
int64_t __fastcall kinoko_actor_set_chip_flags(KinokoActor *actor, void *unused, int32_t flags);
int32_t __fastcall kinoko_actor_set_chip_bound_type(KinokoActor *actor, void *unused, uint16_t shape);
int32_t __fastcall kinoko_actor_get_chip_id(KinokoActor *actor, void *unused, int32_t layer);
int32_t __fastcall kinoko_actor_reset_priority_method(KinokoActor *actor, void *unused, int32_t priority);
int32_t __fastcall kinoko_actor_interrupt_collision(KinokoActor *actor, void *unused);
int32_t __fastcall kinoko_actor_get_chip_flags(KinokoActor *actor, void *unused);
int32_t __fastcall kinoko_actor_has_chip(KinokoActor *actor, void *unused,
    float left, float top, float right, float bottom);

int32_t kinoko_actor_reset_priority(KinokoActor *actor, int32_t priority);
int32_t __fastcall kinoko_actor_release(KinokoActor *actor, void *unused);
KinokoActor *kinoko_actor_set_init_data(KinokoActor *actor, const void *source);
int32_t __fastcall kinoko_actor_sync_animation(KinokoActor *actor, void *unused,
    const void* vtable, int32_t type, int32_t value);

#ifdef __cplusplus
}
#endif
