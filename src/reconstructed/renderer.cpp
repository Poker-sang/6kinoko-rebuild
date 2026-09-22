#include "kinoko/renderer.h"
#include "kinoko/render_target.h"
extern "C" void retdec_trace(const char *);
extern "C" KinokoRenderer *kinoko_renderer_construct(const void *methods) {
    kinoko_renderer.methods=methods;
    kinoko_renderer.state={};
    kinoko_renderer.state.depth_function=D3DCMP_LESSEQUAL;
    kinoko_renderer.state.alpha_function=D3DCMP_ALWAYS;
    kinoko_initialize_renderer_sets();
    kinoko_renderer.device=nullptr;
    kinoko_renderer.backbuffer=nullptr;
    kinoko_renderer.depth_stencil=nullptr;
    return &kinoko_renderer;
}
extern "C" int32_t kinoko_renderer_initialize(void) {
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *render_target = NULL;
    IDirect3DSurface9 *depth_stencil = NULL;
    HRESULT hr;

    if (kinoko_renderer.device != 0) {
        return 0;
    }
    device = kinoko_graphics.device;
    if (device == NULL) {
        return 0;
    }
    kinoko_renderer.device = kinoko_graphics.device;
    retdec_trace("401ae0:pre-list");
    kinoko_add_device_listener((KinokoDeviceListener *)&kinoko_renderer);
    retdec_trace("401ae0:post-list");
    kinoko_renderer.clear_color = 0;
    kinoko_renderer.present_pending = 0;
    retdec_trace("401ae0:pre-clear-1");
    hr = device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    retdec_trace(FAILED(hr) ? "401ae0:clear-1-failed" :
                 "401ae0:clear-1-ok");
    hr = device->SetRenderState(D3DRS_STENCILMASK, 255);
    retdec_trace(FAILED(hr) ? "401ae0:set-render-state-failed" :
                 "401ae0:set-render-state-ok");
    hr = device->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
    retdec_trace(FAILED(hr) ? "401ae0:clear-2-failed" :
                 "401ae0:clear-2-ok");
    hr = device->GetRenderTarget(0, &render_target);
    kinoko_renderer.backbuffer = render_target;
    retdec_trace(FAILED(hr) ? "401ae0:get-render-target-failed" :
                 "401ae0:get-render-target-ok");
    hr = device->GetDepthStencilSurface(&depth_stencil);
    kinoko_renderer.depth_stencil = depth_stencil;
    retdec_trace(FAILED(hr) ? "401ae0:get-depth-stencil-failed" :
                 "401ae0:get-depth-stencil-ok");
    return 1;
}

