#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The three stack words are a SqPlus SquirrelObject: vtable, type, value.
   Ownership of that by-value argument passes to the callee. */
int32_t __fastcall kinoko_actor_set_update_callback(int32_t actor, void *unused,
    int32_t vtable, int32_t type, int32_t value);
int32_t __fastcall kinoko_actor_set_collision_callback(int32_t actor, void *unused,
    int32_t vtable, int32_t type, int32_t value);
int32_t __fastcall kinoko_camera_set_update_callback(int32_t camera, void *unused,
    int32_t vtable, int32_t type, int32_t value);
int32_t __fastcall kinoko_camera_update(int32_t camera, void *unused);
int32_t kinoko_actor_step_callback(int32_t actor);

#ifdef __cplusplus
}
#endif
