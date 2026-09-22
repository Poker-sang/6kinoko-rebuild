#include "kinoko/graphics_device.h"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/render_target.h"
#include "kinoko/diagnostics.h"
#include <initializer_list>
extern "C" { extern char g713; extern int32_t g714; }

// 4013D0. A failed Reset skips after-reset notifications and retains the
// failure state; the message-loop cooperative-level poll decides the next try.
extern "C" int32_t kinoko_graphics_reset(void) {
    auto &state=kinoko_graphics;
    if (!state.factory || !state.device || state.cooperative_status==D3DERR_DEVICELOST) return 0;
    state.present.BackBufferFormat=state.present.Windowed ? state.display.Format : D3DFMT_X8R8G8B8;
    kinoko::graphics::Lock lock;
    kinoko_notify_device_listeners(KINOKO_DEVICE_BEFORE_RESET);
    if (state.swap_chain) { state.swap_chain->Release();state.swap_chain=nullptr; }
    if (FAILED(state.device->Reset(&state.present))) return 0;
    state.device->GetSwapChain(0,&state.swap_chain);
    kinoko_notify_device_listeners(KINOKO_DEVICE_AFTER_RESET);
    return 1;
}

extern "C" HRESULT kinoko_graphics_poll(void) {
    auto &state=kinoko_graphics;
    if (state.device) state.cooperative_status=state.device->TestCooperativeLevel();
    if (state.cooperative_status==D3DERR_DEVICENOTRESET) kinoko_graphics_reset();
    return state.cooperative_status; // do not force D3D_OK after Reset
}

extern "C" int32_t kinoko_graphics_toggle_window(void) {
    auto &parameters=kinoko_graphics.present;
    parameters.Windowed=!parameters.Windowed;
    const auto result=kinoko_graphics_reset();
    if (!result) { parameters.Windowed=!parameters.Windowed;return result; }
    if (parameters.Windowed) {
        const int width=GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXDLGFRAME)+
            GetSystemMetrics(SM_CXBORDER)+static_cast<int>(parameters.BackBufferWidth);
        const int height=GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYDLGFRAME)+
            GetSystemMetrics(SM_CYBORDER)+GetSystemMetrics(SM_CYCAPTION)+static_cast<int>(parameters.BackBufferHeight);
        return SetWindowPos(parameters.hDeviceWindow,HWND_NOTOPMOST,
            (GetSystemMetrics(SM_CXSCREEN)-width)/2,(GetSystemMetrics(SM_CYSCREEN)-height)/2,
            width,height,SWP_FRAMECHANGED);
    }
    // The original makes these first metric reads even though their values
    // are discarded, then computes the border displacement a second time.
    for(int metric:{SM_CXEDGE,SM_CXDLGFRAME,SM_CXBORDER,SM_CYEDGE,SM_CYDLGFRAME,SM_CYBORDER,SM_CYCAPTION})
        GetSystemMetrics(metric);
    const int border_x=GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXDLGFRAME)+GetSystemMetrics(SM_CXBORDER);
    const int border_y=GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYDLGFRAME)+GetSystemMetrics(SM_CYBORDER);
    return SetWindowPos(parameters.hDeviceWindow,nullptr,-border_x/2,
        -(border_y/2+GetSystemMetrics(SM_CYCAPTION)),0,0,SWP_FRAMECHANGED|SWP_NOSIZE);
}

extern "C" int32_t kinoko_graphics_begin_scene(void) {
    EnterCriticalSection(&g676);
    auto *device=kinoko_graphics.device;
    if (!device) { LeaveCriticalSection(&g676);return 0; }
    const auto status=device->BeginScene();
    static volatile LONG traces;
    if (InterlockedIncrement(&traces)<=5) retdec_trace_hresult("401760:beginscene-hr",status);
    // 40177D compares exactly against zero, not merely SUCCEEDED(status).
    if (status!=D3D_OK) { LeaveCriticalSection(&g676);return 0; }
    return 1;
}
extern "C" int32_t kinoko_graphics_end_scene(void) {
    if (auto *device=kinoko_graphics.device)
        retdec_trace_hresult("401790:endscene-hr",device->EndScene());
    LeaveCriticalSection(&g676);
    return 0;
}
extern "C" int32_t kinoko_graphics_present(void) {
    if (!g713 || !TryEnterCriticalSection(&g676)) return 0;
    auto *swap_chain=kinoko_graphics.swap_chain;
    // Retain the existing null-swap-chain startup boundary. The normal path
    // clears pending only on D3D_OK, retaining it on WASSTILLDRAWING/failure.
    HRESULT status=D3D_OK;
    if (swap_chain) status=swap_chain->Present(nullptr,nullptr,nullptr,nullptr,D3DPRESENT_DONOTWAIT);
    static volatile LONG traces;
    if (InterlockedIncrement(&traces)<=5) retdec_trace_hresult("4017b0:present-hr",status);
    if (status==D3D_OK) g713=0;
    LeaveCriticalSection(&g676);
    return status==D3D_OK;
}
extern "C" int32_t kinoko_graphics_clear(void) {
    auto *device=kinoko_graphics.device;
    if (!device) return D3DERR_INVALIDCALL;
    const auto status=device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,g714,1.0f,0);
    static volatile LONG traces;
    if (InterlockedIncrement(&traces)<=5) retdec_trace_hresult("401820:clear-hr",status);
    return status;
}
