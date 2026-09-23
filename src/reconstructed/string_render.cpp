#include "kinoko/map_layout_records.hpp"
#include "kinoko/renderer.h"
#include "kinoko/act_layout_render.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/string_layout.h"
#include "kinoko/string_font.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/string_atlas_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/texture_store.h"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cstring>
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
using LayoutRecord=kinoko::act::StringLayoutRecord;
using GlyphRecord=kinoko::act::StringGlyphRecord;
using AtlasRecord=kinoko::text::AtlasLifecycle;
using kinoko::native::RecordView;
int32_t new_page(int32_t layout) {
    const int32_t page=kinoko_string_append_atlas(layout);
    const RecordView<AtlasRecord> atlas(pointer<void>(page));
    for(int offset:{0,4,8}) field<int32_t>(page+offset)=0;
    atlas.set(&AtlasRecord::references,0);
    field<int32_t>(page+12)=field<int32_t>(page+16)=512;
    kinoko_string_font_configure(page+24,layout);
    atlas.set(&AtlasRecord::texture,kinoko_string_font_texture(page+24));
    return page;
}
// 404EE0's CSpriteEx geometry and texture coordinates. The generic RetDec
// wrapper lost the queried texture dimensions; use the actual texture store.
void rectangle(int32_t s,int32_t handle,int32_t x,int32_t y,int32_t w,int32_t h) {
    field<int32_t>(s+4)=handle;
    if(handle) {
        const float tw=static_cast<float>(kinoko_texture_slots[handle].width);
        const float th=static_cast<float>(kinoko_texture_slots[handle].height);
        field<float>(s+120)=tw;field<float>(s+124)=th;
        const float du=static_cast<float>(w/tw),dv=static_cast<float>(h/th);
        field<float>(s+224)=du;field<float>(s+228)=dv;
        const float u=static_cast<float>(x/tw),v=static_cast<float>(y/th);
        field<float>(s+28)=field<float>(s+84)=u;
        field<float>(s+32)=field<float>(s+60)=v;
        field<float>(s+56)=field<float>(s+112)=du+u;
        field<float>(s+88)=field<float>(s+116)=dv+v;
    } else for(int offset:{120,124,224,228}) field<float>(s+offset)=0;
    for(int offset:{24,52,80,108}) field<uint32_t>(s+offset)=0xffffffffu;
    for(int offset:{128,132,144,152}) field<float>(s+offset)=-0.0f;
    field<float>(s+140)=field<float>(s+164)=static_cast<float>(w);
    field<float>(s+156)=field<float>(s+168)=static_cast<float>(h);
    for(int offset:{136,148,160,172}) field<float>(s+offset)=0;
}

}
extern "C" int32_t kinoko_string_add_character(int32_t layout,const char* character) {
    const RecordView<LayoutRecord> text(pointer<void>(layout));
    auto& cursor=field<int32_t>(layout+204);
    if(*character=='\t') { const int32_t tab=4*field<int32_t>(layout+88);cursor+=tab-cursor%tab;return 1; }
    if(*character=='\n') {
        field<int32_t>(layout+208)+=field<int32_t>(layout+216);cursor=0;
        field<int32_t>(layout+216)=field<int32_t>(layout+88);return 1;
    }
    for(;;) {
        if(kinoko_string_atlas_size(layout)==0) new_page(layout);
        const int32_t page=kinoko_string_atlas_at(layout,kinoko_string_atlas_size(layout)-1);
        const RecordView<AtlasRecord> atlas(pointer<void>(page));
        kinoko_string_font_configure(page+24,layout);
        int32_t width=0,height=0;
        kinoko_string_font_upload(page+24,atlas.get(&AtlasRecord::texture),character,
            field<int32_t>(page),field<int32_t>(page+4),&width,&height);
        if(width>=field<int32_t>(page+12) || height>=field<int32_t>(page+16)) return 0;
        if(field<int32_t>(page)+width>=field<int32_t>(page+12) ||
            (!width && field<int32_t>(page+16)-field<int32_t>(page+8)-field<int32_t>(page+4)>field<int32_t>(layout+88))) {
            field<int32_t>(page+4)+=field<int32_t>(page+8);
            field<int32_t>(page)=field<int32_t>(page+8)=0;
            continue;
        }
        if(field<int32_t>(page+4)+height>=field<int32_t>(page+16) || !height) {new_page(layout);continue;}
        // 440A9B-440BF4 is absent from IDA's decompilation: width/height are
        // output parameters of 405F80, not constants. Follow the assembly.
        atlas.set(&AtlasRecord::references,atlas.get(&AtlasRecord::references)+1);
        const int32_t glyph=kinoko_string_append_glyph(layout),s=glyph+20;
        rectangle(s,atlas.get(&AtlasRecord::texture),field<int32_t>(page),field<int32_t>(page+4),width,height);
        std::copy_n(pointer<unsigned char>(s+128),48,pointer<unsigned char>(s+176));
        field<int32_t>(glyph)=cursor;field<int32_t>(glyph+4)=field<int32_t>(layout+208);
        field<int32_t>(glyph+12)=width;field<int32_t>(glyph+16)=height;
        const RecordView<GlyphRecord> sprite(pointer<void>(glyph));
        const auto id=text.get(&LayoutRecord::next_glyph_id);
        sprite.set(&GlyphRecord::id,id);
        atlas.set(&AtlasRecord::last_glyph_id,id);
        sprite.set(&GlyphRecord::atlas,pointer<void>(page));
        text.set(&LayoutRecord::next_glyph_id,id+1);
        field<int32_t>(page+8)=(std::max)(field<int32_t>(page+8),height);
        field<int32_t>(page)+=width;cursor+=width;
        auto& line_height=field<int32_t>(layout+216);line_height=(std::max)(line_height,height);
        if(field<int32_t>(layout+144)>=0 && cursor>=field<int32_t>(layout+144)) {
            field<int32_t>(layout+208)+=line_height;cursor=0;line_height=field<int32_t>(layout+88);
        }
        field<int32_t>(layout+212)=(std::max)(field<int32_t>(layout+212),cursor);
        return 1;
    }
}
