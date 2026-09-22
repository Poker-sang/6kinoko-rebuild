#include "kinoko/renderer.h"
#include "kinoko/quad_render.h"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/render_target.h"
#include "kinoko/texture_store.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/com_owner.hpp"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <set>
#include <list>
#include <stdexcept>
#include "kinoko/legacy_abi.h"
extern "C" {
extern int32_t g746,g747,g748,g749,g750,g751,g752,g753;
extern CRITICAL_SECTION g676;
HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,
                                D3DFORMAT,D3DPOOL,IDirect3DTexture9**);
}
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
// 401DC0 -> 402670/402360 stores the unsigned handle as a 20-byte set-node
// value. It does not AddRef/retain the texture; the set owns only its nodes.
std::set<uint32_t> render_targets;
std::set<uint32_t> depth_targets;
std::list<KinokoDeviceListener *> device_listeners;
using GraphicsLock=kinoko::graphics::Lock;
int32_t create(uint32_t width,uint32_t height) {
    if(kinoko_graphics.capabilities.TextureCaps&0x20) width=height=(std::max)(width,height);
    kinoko::ComOwner<IDirect3DTexture9> texture;
    {
        GraphicsLock lock;
        if(FAILED(D3DXCreateTexture(kinoko_graphics.device,width,height,1,
            D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,texture.put()))) return 0;
    }
    const int32_t handle=kinoko_texture_register(texture.get(),width,height);
    if(handle) texture.detach();
    // Original inserts the returned handle even when registration returned 0.
    render_targets.insert(static_cast<uint32_t>(handle));
    return handle;
}
}
extern "C" void kinoko_initialize_renderer_sets(void) {
    render_targets.clear();depth_targets.clear();
}
extern "C" void kinoko_initialize_texture_cache(void) {
    // Actual texture ownership is constructed by the standard array/string
    // objects in texture_store.cpp. Only these eight borrowed stage caches
    // remain visible through the decompiled host ABI (4059C0).
    g746=g747=g748=g749=g750=g751=g752=g753=0;
}
extern "C" int32_t kinoko_set_render_target(int32_t handle) {
    // 401E60 selects level zero, releases the temporary surface reference,
    // and restores the renderer's cached backbuffer for handle zero.
    auto* device=kinoko_renderer.device;
    if(!device) return E_FAIL;
    if(!handle) return device->SetRenderTarget(0,kinoko_renderer.backbuffer);
    if(handle<0 || handle>=KINOKO_TEXTURE_CAPACITY) return E_INVALIDARG;
    auto* texture=static_cast<IDirect3DTexture9*>(kinoko_texture_slots[handle].texture);
    if(!texture) return E_INVALIDARG;
    kinoko::ComOwner<IDirect3DSurface9> surface;
    const auto status=texture->GetSurfaceLevel(0,surface.put());
    if(FAILED(status)) return status;
    device->SetRenderTarget(0,surface.get());
    return surface.detach()->Release();
}
extern "C" int32_t __fastcall kinoko_method_create_render_target(KinokoActResource *resource,void*,int32_t width,int32_t height) {
    // 449C10 returns true even if D3DX creation fails; never reinterpret that
    // return as handle != 0. Requested image dimensions remain unsquared.
    using Resource=kinoko::act::TextureResourcePrefix;
    const kinoko::native::RecordView<Resource> view(resource);
    view.set(&Resource::source_x,0.0f);view.set(&Resource::source_y,0.0f);
    view.set(&Resource::width,width);view.set(&Resource::height,height);
    view.set(&Resource::source_width,static_cast<float>(width));
    view.set(&Resource::source_height,static_cast<float>(height));
    view.set(&Resource::flag,uint8_t{0});
    view.set(&Resource::texture,create(static_cast<uint32_t>(width),static_cast<uint32_t>(height)));
    return 1;
}

// 401660/4016E0: insertion order, duplicate suppression and borrowed objects.
extern "C" void kinoko_initialize_device_listeners(void) { device_listeners.clear(); }
extern "C" int32_t kinoko_add_device_listener(KinokoDeviceListener *object) {
    GraphicsLock lock;
    if(std::find(device_listeners.begin(),device_listeners.end(),object)!=device_listeners.end()) return 0;
    if(device_listeners.size()==0x3ffffffeu) throw std::length_error("list<T> too long");
    device_listeners.push_back(object);
    return 1;
}
extern "C" void kinoko_remove_device_listener(KinokoDeviceListener *object) {
    GraphicsLock lock;
    const auto found=std::find(device_listeners.begin(),device_listeners.end(),object);
    if(found!=device_listeners.end()) device_listeners.erase(found);
}
extern "C" void kinoko_notify_device_listeners(KinokoDeviceEvent event) {
    using Callback=void (__thiscall *)(KinokoDeviceListener *);
    struct Methods { Callback before_reset,after_reset; };
    // Queue owns nodes, not listeners; preserve registration order and actual
    // virtual dispatch. The caller already holds the recursive graphics lock.
    for (auto *object:device_listeners) {
        const auto *methods=kinoko::legacy::load<const Methods *>(object);
        const auto callback=event==KINOKO_DEVICE_BEFORE_RESET ? methods->before_reset : methods->after_reset;
        callback(object);
    }
}

extern "C" int32_t __fastcall kinoko_renderer_before_reset(KinokoRenderer *renderer,void*) {
    // 401DA0 releases acquired references, leaving slot bits intact as original.
    renderer->backbuffer->Release();
    return renderer->depth_stencil->Release();
}
extern "C" int32_t __fastcall kinoko_renderer_after_reset(KinokoRenderer *renderer,void*) {
    auto *device=renderer->device;
    for(DWORD stage=0;stage<8;++stage) kinoko_graphics.device->SetTexture(stage,nullptr);
    kinoko_initialize_texture_cache();
    const KinokoRenderState saved=renderer->state;
    renderer->state={};renderer->present_pending=0;
    device->GetRenderTarget(0,&renderer->backbuffer);
    device->GetDepthStencilSurface(&renderer->depth_stencil);
    const auto state=[&](D3DRENDERSTATETYPE type,DWORD value) { device->SetRenderState(type,value); };
    const DWORD alpha=saved.alpha_flags&255u;
    const DWORD test=(saved.alpha_flags>>8)&255u;
    if(alpha) state(D3DRS_ALPHABLENDENABLE,alpha);
    if(test) state(D3DRS_ALPHATESTENABLE,test);
    renderer->state.alpha_flags=alpha|(test<<8);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
    if(saved.alpha_function) { state(D3DRS_ALPHAFUNC,saved.alpha_function);renderer->state.alpha_function=saved.alpha_function; }
    if(saved.alpha_reference) { state(D3DRS_ALPHAREF,saved.alpha_reference);renderer->state.alpha_reference=saved.alpha_reference; }
    state(D3DRS_ZENABLE,saved.depth_flags&255u);
    state(D3DRS_ZWRITEENABLE,(saved.depth_flags>>8)&255u);
    renderer->state.depth_flags=saved.depth_flags&0xffff;
    state(D3DRS_ZFUNC,saved.depth_function);renderer->state.depth_function=saved.depth_function;
    // The original process owns a single renderer; named state helpers use it.
    kinoko_render_set_filter(saved.filter);kinoko_render_set_blend(saved.blend);
    kinoko_render_set_cull(saved.cull);
    state(D3DRS_STENCILMASK,255);
    return device->Clear(0,nullptr,D3DCLEAR_STENCIL,0,1.0f,0);
}
