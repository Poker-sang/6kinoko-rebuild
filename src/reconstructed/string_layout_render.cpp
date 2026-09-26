#include "kinoko/act_layout_render.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/string_layout.h"
#include "kinoko/renderer.h"
#include "kinoko/quad_render.h"
#include "kinoko/legacy_memory.hpp"
#include <cstring>
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
namespace {
using TextRecord=kinoko::act::StringLayoutRecord;
using GlyphRecord=kinoko::act::StringGlyphRecord;
using TextView=kinoko::native::RecordView<TextRecord>;
using GlyphView=kinoko::native::RecordView<GlyphRecord>;
}
extern "C" int32_t __fastcall kinoko_method_set_string_layer(KinokoStringLayout* object,void*,KinokoActLayer* layer) {
    if(!layer) return E_FAIL;
    TextView(object).set(&TextRecord::layer,layer);return 0;
}
extern "C" int32_t __fastcall kinoko_method_update_string_layout(KinokoStringLayout* layout,void*) {
    const TextView text(layout);
    auto *layer=text.get(&TextRecord::layer);
    if(!layer) return E_FAIL;
    if(text.get(&TextRecord::rebuild)) { kinoko_string_rebuild_queue(layout);text.set(&TextRecord::rebuild,uint8_t{0}); }
    StringView pending(text.bytes(&TextRecord::pending)),displayed(text.bytes(&TextRecord::text));
    // Original consumes pending multibyte characters before visibility testing.
    while(pending.length()) {
        const auto bytes=static_cast<uint32_t>(CharNextA(pending.data())-pending.data());
        char character[8]{};memcpy_s(character,sizeof(character),pending.data(),bytes);
        kinoko_string_add_character((KinokoStringLayout*)(uintptr_t)(layout), character);
        displayed.append(character,static_cast<uint32_t>(std::strlen(character)));
        pending.assign(pending,bytes,UINT32_MAX);
    }
    const kinoko::map::LayerView owner(layer);
    if(!owner.get(&kinoko::map::LayerRecord::visible)) return 0;
    const auto alpha=static_cast<uint32_t>(static_cast<int64_t>(text.get(&TextRecord::alpha)*255.0));
    const uint32_t color=(alpha<<24)|(uint32_t(static_cast<uint8_t>(text.get(&TextRecord::base_red)))<<16)|
        (uint32_t(static_cast<uint8_t>(text.get(&TextRecord::base_green)))<<8)|static_cast<uint8_t>(text.get(&TextRecord::base_blue));
    const uint32_t count=kinoko_string_queue_size((KinokoStringLayout*)(uintptr_t)(layout));
    for(uint32_t i=0;i<count;++i) {
        const GlyphView glyph(pointer(kinoko_string_queue_at((KinokoStringLayout*)(uintptr_t)(layout), i)));
        auto quad=glyph.get(&GlyphRecord::quad);
        for(auto &vertex:quad.vertices) vertex.color=color;
        glyph.set(&GlyphRecord::quad,quad);
    }
    float x=0,y=0,z=0;
    using Position=void (__thiscall *)(KinokoActLayer*,float*,float*,float*);
    auto method=kinoko::legacy::load<Position>(owner.get(&kinoko::map::LayerRecord::methods)+7*sizeof(void*));
    method(layer,&x,&y,&z);
    text.set(&TextRecord::origin_x,static_cast<int32_t>(x));text.set(&TextRecord::origin_y,static_cast<int32_t>(y));
    for(uint32_t i=0;i<count;++i) {
        const GlyphView glyph(pointer(kinoko_string_queue_at((KinokoStringLayout*)(uintptr_t)(layout), i)));
        float gx=static_cast<float>(glyph.get(&GlyphRecord::x));
        const auto alignment=text.get(&TextRecord::alignment);
        if(alignment==1) gx-=text.get(&TextRecord::maximum_width)/2;
        if(alignment==2) gx-=text.get(&TextRecord::maximum_width);
        const auto sx=text.get(&TextRecord::scale_x),sy=text.get(&TextRecord::scale_y);
        const float dx=static_cast<float>(double(sx)*gx+text.get(&TextRecord::origin_x));
        const auto height=glyph.get(&GlyphRecord::height);
        const float dy=static_cast<float>((double(height)-double(height)*sy)*0.5+
            text.get(&TextRecord::origin_y)+double(sy)*static_cast<float>(glyph.get(&GlyphRecord::y)));
        auto quad=glyph.get(&GlyphRecord::quad);quad.positions=quad.base_positions;
        for(auto &p:quad.positions) { p.x*=sx;p.y*=sy;p.z*=1.0f;p.x+=dx;p.y+=dy;p.z+=0.0f; }
        glyph.set(&GlyphRecord::quad,quad);
    }
    return 0;
}
extern "C" int32_t __fastcall kinoko_method_draw_string_layout(KinokoStringLayout* layout,void*,float x,float y) {
    const TextView text(layout);
    auto *layer=text.get(&TextRecord::layer);
    if(!layer) return E_FAIL;
    if(!kinoko::map::LayerView(layer).get(&kinoko::map::LayerRecord::visible)) return 0;
    auto *device=kinoko_graphics.device;
    DWORD saved[4]{};
    const D3DRENDERSTATETYPE states[]={D3DRS_SRCBLEND,D3DRS_DESTBLEND,D3DRS_BLENDOP,D3DRS_ALPHABLENDENABLE};
    for(int i=0;i<4;++i) device->GetRenderState(states[i],&saved[i]);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);kinoko::act::set_layout_blend(text.get(&TextRecord::blend));
    kinoko_render_set_filter(2);
    const uint32_t count=kinoko_string_queue_size((KinokoStringLayout*)(uintptr_t)(layout));
    for(uint32_t i=0;i<count;++i) {
        const GlyphView glyph(pointer(kinoko_string_queue_at((KinokoStringLayout*)(uintptr_t)(layout), i)));
        kinoko_quad_submit(reinterpret_cast<KinokoQuad *>(glyph.bytes(&GlyphRecord::quad)),x,y);
    }
    for(int i=0;i<4;++i) device->SetRenderState(states[i],saved[i]);
    return 0;
}
