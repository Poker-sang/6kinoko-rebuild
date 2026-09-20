#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_initialize_renderer_sets(void);
void kinoko_initialize_device_listeners(void);
int32_t kinoko_add_device_listener(int32_t object);
void kinoko_remove_device_listener(int32_t object);
void kinoko_notify_device_listeners(int32_t slot);
int32_t __fastcall kinoko_renderer_before_reset(int32_t object, void *unused);
int32_t __fastcall kinoko_renderer_after_reset(int32_t object, void *unused);
void kinoko_initialize_texture_cache(void);
int32_t kinoko_set_render_target(int32_t texture_handle);
int32_t __fastcall kinoko_method_create_render_target(int32_t resource, void *unused,
                                                      int32_t width, int32_t height);
#ifdef __cplusplus
}
#endif
