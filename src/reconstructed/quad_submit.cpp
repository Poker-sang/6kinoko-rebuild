#include "kinoko/texture_store.h"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/diagnostics.h"
#include <d3d9.h>
extern "C" {
void kinoko_trace_i32(const char *,int32_t);
}

// 405800: IColor + borrowed texture + four transformed 28-byte vertices.
extern "C" int32_t kinoko_quad_submit(KinokoQuad *storage,float x,float y) {
    if (!storage || !kinoko_graphics.device) return E_FAIL; // inherited unavailable-device boundary
    const kinoko::render::QuadView quad(storage);
    using kinoko::render::QuadRecord;
    auto vertices=quad.get(&QuadRecord::vertices);
    const auto positions=quad.get(&QuadRecord::positions);
    static volatile LONG traces;
    const bool trace=InterlockedIncrement(&traces)<=8;
    if (trace) {
        kinoko_trace_i32("draw:vertex-buffer",kinoko::legacy::address(storage));
        kinoko_trace_i32("draw:handle",quad.get(&QuadRecord::texture));
    }
    for(size_t i=0;i<vertices.size();++i) {
        vertices[i].x=positions[i].x+x-0.5f;
        vertices[i].y=positions[i].y+y-0.5f;
        vertices[i].z=positions[i].z+0.5f;
        vertices[i].rhw=1.0f;
    }
    quad.set(&QuadRecord::vertices,vertices);
    const auto texture_result=kinoko_texture_bind_stage(0,quad.get(&QuadRecord::texture));
    if (trace) kinoko_trace_hresult("draw:set-texture-hr",texture_result);
    auto *device=kinoko_graphics.device;
    if (!device || !kinoko::legacy::load<const void *>(device)) return E_FAIL;
    const auto format_result=device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    if (trace) kinoko_trace_hresult("draw:set-fvf-hr",format_result);
    // Both setup HRESULTs are ignored in the original; the draw supplies return.
    const auto result=device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,
        quad.bytes(&QuadRecord::vertices),sizeof(KinokoSpriteVertex));
    if (trace) kinoko_trace_hresult("draw:primitive-hr",result);
    return result;
}
