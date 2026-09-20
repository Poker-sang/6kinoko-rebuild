#include "kinoko/render_target.h"
#include "kinoko/texture_store.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/com_owner.hpp"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <set>
extern "C" {
extern int32_t g678,g681;
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
struct GraphicsLock {
    GraphicsLock() { EnterCriticalSection(&g676); }
    ~GraphicsLock() { LeaveCriticalSection(&g676); }
};
int32_t create(uint32_t width,uint32_t height) {
    if(g681&0x20) width=height=(std::max)(width,height);
    kinoko::ComOwner<IDirect3DTexture9> texture;
    {
        GraphicsLock lock;
        if(FAILED(D3DXCreateTexture(pointer<IDirect3DDevice9>(g678),width,height,1,
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
extern "C" int32_t __fastcall kinoko_method_create_render_target(int32_t resource,void*,int32_t width,int32_t height) {
    // 449C10 returns true even if D3DX creation fails; never reinterpret that
    // return as handle != 0. Requested image dimensions remain unsquared.
    field<float>(resource+80)=field<float>(resource+84)=0;
    field<int32_t>(resource+72)=width;field<int32_t>(resource+76)=height;
    field<float>(resource+88)=static_cast<float>(width);
    field<float>(resource+92)=static_cast<float>(height);
    field<uint8_t>(resource+96)=0;
    field<int32_t>(resource+68)=create(static_cast<uint32_t>(width),static_cast<uint32_t>(height));
    return 1;
}
