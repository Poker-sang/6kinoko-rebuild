#pragma once
#include <stdint.h>
#include "kinoko/renderer.h"
#include "kinoko/act_types.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoDeviceListener KinokoDeviceListener;
typedef enum KinokoDeviceEvent { KINOKO_DEVICE_BEFORE_RESET, KINOKO_DEVICE_AFTER_RESET } KinokoDeviceEvent;
void kinoko_initialize_renderer_sets(void);
void kinoko_initialize_device_listeners(void);
int32_t kinoko_add_device_listener(KinokoDeviceListener *object);
void kinoko_remove_device_listener(KinokoDeviceListener *object);
void kinoko_notify_device_listeners(KinokoDeviceEvent event);
int32_t __fastcall kinoko_renderer_before_reset(KinokoRenderer *object, void *unused);
int32_t __fastcall kinoko_renderer_after_reset(KinokoRenderer *object, void *unused);
void kinoko_initialize_texture_cache(void);
int32_t kinoko_set_render_target(int32_t texture_handle);
int32_t __fastcall kinoko_method_create_render_target(KinokoActResource *resource, void *unused,
                                                      int32_t width, int32_t height);
#ifdef __cplusplus
}
#endif
