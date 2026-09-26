#pragma once
#include "kinoko/camera.h"
#include "kinoko/actor_lifecycle.h"

#include <stdint.h>

#ifndef KINOKO_SCRIPT_CALLBACK_RECORD
#define KINOKO_SCRIPT_CALLBACK_RECORD
typedef struct KinokoScriptCallback {
    struct SQVM *vm;
    KinokoOwnedObjectWords environment, closure;
} KinokoScriptCallback;
#endif

#ifdef __cplusplus
extern "C" {
#endif

KinokoScriptCallback *kinoko_script_callback_construct(KinokoScriptCallback *callback, const char *name);
void kinoko_script_callback_clear(KinokoScriptCallback *callback);
int32_t kinoko_script_callback_invoke(KinokoScriptCallback *callback);
int32_t kinoko_script_callback_invoke_owned(KinokoScriptCallback *callback, KinokoOwnedObjectWords *temporary, int32_t type, int32_t data);

/* The three stack words are a SqPlus SquirrelObject: vtable, type, value.
   Ownership of that by-value argument passes to the callee. */
int32_t __fastcall kinoko_actor_set_update_callback(KinokoActor *actor, void *unused,
    const void* vtable, int32_t type, int32_t value);
int32_t __fastcall kinoko_actor_set_collision_callback(KinokoActor *actor, void *unused,
    const void* vtable, int32_t type, int32_t value);
int32_t __fastcall kinoko_camera_set_update_callback(KinokoCamera *camera, void *unused,
    const void* vtable, int32_t type, int32_t value);
int32_t __fastcall kinoko_camera_update(KinokoCamera *camera, void *unused);
int32_t kinoko_actor_step_callback(KinokoActor *actor);
int32_t kinoko_actor_clear_script(KinokoActor *actor);
int32_t kinoko_destroy_script_callback(KinokoScriptCallback *callback);

#ifdef __cplusplus
}
#endif
