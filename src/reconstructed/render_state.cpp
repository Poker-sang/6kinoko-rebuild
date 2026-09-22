#include "kinoko/quad_render.h"
#include "kinoko/legacy_memory.hpp"
#include <d3d9.h>
extern "C" { extern int32_t g702,g703,g707,g709; }
namespace {
struct BlendTransition { int32_t key; DWORD operation,source,destination; };
// 402770 updates only changed D3D states. Zero means no write, not D3DBLEND_ZERO.
constexpr BlendTransition transitions[]={
    {0,D3DBLENDOP_ADD,D3DBLEND_SRCALPHA,D3DBLEND_INVSRCALPHA},
    {1,D3DBLENDOP_ADD,D3DBLEND_SRCALPHA,D3DBLEND_ONE},
    {2,D3DBLENDOP_REVSUBTRACT,D3DBLEND_SRCALPHA,D3DBLEND_ONE},
    {34,D3DBLENDOP_REVSUBTRACT,D3DBLEND_SRCALPHA,D3DBLEND_ONE},
    {3,D3DBLENDOP_ADD,D3DBLEND_ZERO,D3DBLEND_SRCCOLOR},
    {27,D3DBLENDOP_ADD,D3DBLEND_ZERO,D3DBLEND_SRCCOLOR},
    {9,0,0,D3DBLEND_ONE},{10,D3DBLENDOP_REVSUBTRACT,0,D3DBLEND_ONE},
    {11,0,D3DBLEND_ZERO,D3DBLEND_SRCCOLOR},{19,0,D3DBLEND_ZERO,D3DBLEND_SRCCOLOR},
    {16,0,0,D3DBLEND_INVSRCALPHA},{18,D3DBLENDOP_REVSUBTRACT,0,0},
    {24,D3DBLENDOP_ADD,0,D3DBLEND_INVSRCALPHA},{25,D3DBLENDOP_ADD,0,0},
    {32,D3DBLENDOP_ADD,D3DBLEND_SRCALPHA,D3DBLEND_INVSRCALPHA},
    {33,0,D3DBLEND_SRCALPHA,D3DBLEND_ONE}
};
}
extern "C" int32_t kinoko_render_set_blend(int32_t mode) {
    if (g703==mode) return mode;
    auto *device=kinoko::legacy::pointer<IDirect3DDevice9>(g702);
    HRESULT result=S_OK;
    const auto key=mode-1+8*g703;
    if(device) for(const auto &entry:transitions) {
        if(entry.key!=key) continue;
        if(entry.operation) result=device->SetRenderState(D3DRS_BLENDOP,entry.operation);
        if(entry.source) result=device->SetRenderState(D3DRS_SRCBLEND,entry.source);
        if(entry.destination) result=device->SetRenderState(D3DRS_DESTBLEND,entry.destination);
        break;
    }
    g703=mode;
    return result;
}
extern "C" int32_t kinoko_render_set_alpha(int32_t blend_enabled,int32_t test_enabled) {
    auto *device=kinoko::legacy::pointer<IDirect3DDevice9>(g702);
    auto *flags=reinterpret_cast<unsigned char *>(&g709); // remaining host state boundary
    const auto blend=static_cast<uint8_t>(blend_enabled),test=static_cast<uint8_t>(test_enabled);
    HRESULT result=S_OK;
    if(flags[0]!=blend) {
        if(device) result=device->SetRenderState(D3DRS_ALPHABLENDENABLE,blend);
        flags[0]=blend;
    }
    if(flags[1]!=test) {
        if(device) result=device->SetRenderState(D3DRS_ALPHATESTENABLE,test);
        flags[1]=test;
    }
    if(device) result=device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
    return result;
}

extern "C" int32_t kinoko_render_set_depth(int32_t test_enabled,int32_t write_enabled) {
    auto *device=kinoko::legacy::pointer<IDirect3DDevice9>(g702);
    const auto test=static_cast<uint8_t>(test_enabled),write=static_cast<uint8_t>(write_enabled);
    HRESULT result=S_OK;
    if(device) {
        device->SetRenderState(D3DRS_ZENABLE,test);
        result=device->SetRenderState(D3DRS_ZWRITEENABLE,write);
    }
    auto *flags=reinterpret_cast<unsigned char *>(&g707);
    flags[1]=write;flags[0]=test;
    return result;
}
