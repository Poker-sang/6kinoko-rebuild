#include "kinoko/critical_section.h"
#include "kinoko/render_target.h"
#include "kinoko/graphics_device.h"
#include "kinoko/diagnostics.h"
#include <cstring>
#include <cstddef>
static_assert(sizeof(D3DCAPS9)==304 && offsetof(D3DCAPS9,TextureCaps)==60);
static_assert(sizeof(D3DPRESENT_PARAMETERS)==56);
extern "C" void kinoko_trace_i32(const char *,int32_t);

// 4011B0: preserve the original HAL/HW -> HAL/SW -> REF/SW fallback order.
extern "C" int32_t kinoko_graphics_create(HWND window,int32_t width,int32_t height) {
    if (!window) return 0; // inherited invalid-window boundary
    auto &state=kinoko_graphics;
    state.original_window_style=GetWindowLongA(window,GWL_STYLE);
    kinoko_trace("4011b0:pre-d3d-create");
    state.factory=Direct3DCreate9(D3D_SDK_VERSION);
    if (!state.factory) {
        MessageBoxA(window,"Direct3DCreate9 failed","DirectX-Error",MB_OK);return 0;
    }
    state.display={};
    if (FAILED(state.factory->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&state.display))) {
        // Retain the inherited startup-failure cleanup boundary.
        state.factory->Release();state.factory=nullptr;
        MessageBoxA(window,"GetAdapterDisplayMode failed","DirectX-Error",MB_OK);return 0;
    }
    if (width<=0 || height<=0) {
        WINDOWINFO info{};info.cbSize=sizeof(info);
        GetWindowInfo(window,&info);
        width=info.rcClient.right-info.rcClient.left;
        height=info.rcClient.bottom-info.rcClient.top;
    }
    state.present={};
    auto &parameters=state.present;
    parameters.BackBufferWidth=width;
    parameters.BackBufferHeight=height;
    parameters.BackBufferFormat=state.display.Format;
    parameters.BackBufferCount=1;
    parameters.MultiSampleType=D3DMULTISAMPLE_NONE;
    parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;
    parameters.hDeviceWindow=window;
    parameters.Windowed=TRUE;
    parameters.EnableAutoDepthStencil=TRUE;
    parameters.AutoDepthStencilFormat=D3DFMT_D24S8;
    parameters.Flags=D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    parameters.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
    kinoko_trace_i32("4011b0:client-width",width);
    kinoko_trace_i32("4011b0:client-height",height);
    struct Attempt { D3DDEVTYPE type; DWORD behavior; };
    constexpr Attempt attempts[]={
        {D3DDEVTYPE_HAL,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED},
        {D3DDEVTYPE_HAL,D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED},
        {D3DDEVTYPE_REF,D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED}
    };
    HRESULT status=E_FAIL;
    for (const auto attempt:attempts) {
        status=state.factory->CreateDevice(D3DADAPTER_DEFAULT,attempt.type,window,
            attempt.behavior,&parameters,&state.device);
        kinoko_trace_hresult("4011b0:create-device-hr",status);
        if (SUCCEEDED(status)) break;
    }
    if (FAILED(status) || !state.device) {
        state.factory->Release();state.factory=nullptr;state.device=nullptr;
        MessageBoxA(window,"CreateDevice failed","DirectX-Error",MB_OK);return 0;
    }
    state.capabilities={};
    state.device->GetDeviceCaps(&state.capabilities);
    state.device->GetSwapChain(0,&state.swap_chain);
    state.cooperative_status=D3D_OK;
    kinoko_trace("4011b0:done");
    return 1;
}

// 401600: swap chain, device, then factory; consumers borrow those interfaces.
extern "C" int32_t kinoko_graphics_release(void) {
    auto &state=kinoko_graphics;
    if (state.swap_chain) { state.swap_chain->Release();state.swap_chain=nullptr; }
    if (state.device) { state.device->Release();state.device=nullptr; }
    ULONG result=0;
    if (state.factory) { result=state.factory->Release();state.factory=nullptr; }
    return static_cast<int32_t>(result);
}

// 401040, invoked once by the reconstructed CRT initialization sequence.
extern "C" void kinoko_graphics_initialize_runtime() {
    kinoko_critical_section_construct(&kinoko_graphics_lock);
    kinoko_initialize_device_listeners();
    kinoko_graphics.factory = nullptr;
    kinoko_graphics.device = nullptr;
    kinoko_graphics.swap_chain = nullptr;
    kinoko_graphics.unknown_state = 0;
}
