#pragma once
#include "kinoko/graphics_device.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoRenderState {
    int32_t blend, filter, cull, unknown24;
    uint32_t depth_flags;
    int32_t depth_function;
    uint32_t alpha_flags;
    int32_t alpha_function, alpha_reference, unknown48;
} KinokoRenderState;
typedef struct KinokoRenderer {
    const void *methods;
    IDirect3DDevice9 *device; /* borrowed from KinokoGraphics */
    uint32_t unknown8;
    KinokoRenderState state;
    uint16_t unknown52;
    uint8_t present_pending, unknown55;
    uint32_t clear_color;
    uint8_t unknown60[16];
    IDirect3DSurface9 *backbuffer;
    uint8_t unknown80[16];
    IDirect3DSurface9 *depth_stencil;
} KinokoRenderer;
/* GetRenderTarget/GetDepthStencilSurface acquire references released before
   Reset/shutdown. Original leaves released slots intact until reacquisition. */
extern KinokoRenderer kinoko_renderer;
KinokoRenderer *kinoko_renderer_construct(const void *methods);
int32_t kinoko_renderer_initialize(void);
int32_t kinoko_render_set_filter(int32_t mode);
int32_t kinoko_render_set_cull(int32_t mode);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoRenderState)==40 && sizeof(KinokoRenderer)==100);
static_assert(offsetof(KinokoRenderer,state)==12);
static_assert(offsetof(KinokoRenderer,present_pending)==54);
static_assert(offsetof(KinokoRenderer,backbuffer)==76);
static_assert(offsetof(KinokoRenderer,depth_stencil)==96);
#endif
