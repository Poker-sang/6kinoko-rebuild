#pragma once
#include <stdint.h>
#include <windows.h>
#include <d3d9.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Factory, device and swap_chain each own one COM reference. Renderer and
   draw consumers borrow device. Actual SDK structs replace split RetDec words. */
typedef struct KinokoGraphics {
    IDirect3D9 *factory;
    IDirect3DDevice9 *device;
    IDirect3DSwapChain9 *swap_chain;
    D3DCAPS9 capabilities;
    D3DPRESENT_PARAMETERS present;
    D3DDISPLAYMODE display;
    LONG original_window_style;
    HRESULT cooperative_status;
    int32_t unknown_state;
} KinokoGraphics;
extern KinokoGraphics kinoko_graphics;
void kinoko_graphics_initialize_runtime(void);
int32_t kinoko_graphics_create(HWND window,int32_t width,int32_t height);
int32_t kinoko_graphics_release(void);
int32_t kinoko_graphics_reset(void);
int32_t kinoko_graphics_toggle_window(void);
HRESULT kinoko_graphics_poll(void);
int32_t kinoko_graphics_begin_scene(void);
int32_t kinoko_graphics_end_scene(void);
int32_t kinoko_graphics_present(void);
int32_t kinoko_graphics_clear(void);
#ifdef __cplusplus
}
#endif
