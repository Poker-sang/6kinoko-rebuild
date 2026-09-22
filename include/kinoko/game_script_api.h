#pragma once
#include "kinoko/actor_manager.h"
#include "kinoko/map_collision.h"
#ifdef __cplusplus
extern "C" {
#endif
/* The Win32 cdecl ABI passes each trivial 12-byte argument on the stack.
   These entries consume one external reference per by-value script object.
   Manager/map APIs beneath this layer only borrow their input objects. */
int32_t kinoko_script_set_global_update(KinokoOwnedObjectWords closure, KinokoOwnedObjectWords environment);
int32_t kinoko_script_set_init(int32_t id, KinokoOwnedObjectWords closure, KinokoOwnedObjectWords environment);
KinokoOwnedObjectWords *kinoko_script_create_actor(KinokoOwnedObjectWords *result,
    KinokoOwnedObjectWords closure, float x, float y, float z, KinokoOwnedObjectWords argument);
int32_t kinoko_script_create_map_actors(const char *name, KinokoOwnedObjectWords environment);
int32_t kinoko_script_create_event(const char *name, KinokoOwnedObjectWords closure, KinokoOwnedObjectWords environment);
int32_t kinoko_script_load_animation(const char *path);
void *kinoko_script_clear_actors(void);
int32_t kinoko_script_move_actors(float dx, float dy);
void *kinoko_script_refresh_collision(void);
int32_t kinoko_script_clear_collision(void);
KinokoActor *kinoko_script_create_collision(const char *name);
int32_t kinoko_script_load_act(const char *path);
int32_t kinoko_script_load_map(const char *path);
int32_t kinoko_script_release_map(void);
int32_t kinoko_script_clear_render_layers(void);
void *kinoko_script_create_render_layer(const char *name);
/* Native Squirrel closure adapters; VM pointer is real, userdata is the ABI boundary. */
int32_t kinoko_script_global_update_entry(struct SQVM *vm);
int32_t kinoko_script_create_actor_entry(struct SQVM *vm);
#ifdef __cplusplus
}
#endif
