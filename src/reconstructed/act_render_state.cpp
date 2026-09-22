#include "kinoko/act_layout_render.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/renderer.h"
void kinoko::act::set_layout_blend(int32_t mode) {
    auto* device=kinoko_graphics.device;
    if(mode<1 || mode>4) {
        device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SRCBLEND,mode==5?D3DBLEND_DESTCOLOR:D3DBLEND_ONE);
        device->SetRenderState(D3DRS_DESTBLEND,mode==5?D3DBLEND_ONE:D3DBLEND_ZERO);
        return;
    }
    if(kinoko_renderer.state.blend==mode) return;
    auto* cached=kinoko_renderer.device;
    auto op=[&](DWORD value){cached->SetRenderState(D3DRS_BLENDOP,value);};
    auto src=[&](DWORD value){cached->SetRenderState(D3DRS_SRCBLEND,value);};
    auto dst=[&](DWORD value){cached->SetRenderState(D3DRS_DESTBLEND,value);};
    // Original 42AC20 shares the transition key with 402770. Case 32 has
    // no blend-op write here, so do not substitute that near-duplicate helper.
    switch(8*kinoko_renderer.state.blend+mode-1) {
    case 0:op(1);src(5);dst(6);break;
    case 1:op(1);src(5);dst(2);break;
    case 2:case 34:op(3);src(5);dst(2);break;
    case 3:case 27:op(1);src(1);dst(3);break;
    case 9:dst(2);break;
    case 10:op(3);dst(2);break;
    case 11:case 19:src(1);dst(3);break;
    case 16:dst(6);break;
    case 18:op(3);break;
    case 24:op(1);dst(6);break;
    case 25:op(1);break;
    case 32:src(5);dst(6);break;
    case 33:src(5);dst(2);break;
    default:break;
    }
    kinoko_renderer.state.blend=mode;
}
