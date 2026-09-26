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
void* new_page(KinokoStringLayout* layout) {
    auto* page=kinoko_string_append_atlas((KinokoStringLayout*)(uintptr_t)(layout));
    const RecordView<AtlasRecord> atlas(page);
    atlas.set(&AtlasRecord::cursor_x,0);
    atlas.set(&AtlasRecord::cursor_y,0);
    atlas.set(&AtlasRecord::row_height,0);
    atlas.set(&AtlasRecord::references,0);
    atlas.set(&AtlasRecord::width,512);
    atlas.set(&AtlasRecord::height,512);
    kinoko_string_font_configure(atlas.bytes(&AtlasRecord::renderer), layout);
    atlas.set(&AtlasRecord::texture,kinoko_string_font_texture(atlas.bytes(&AtlasRecord::renderer)));
    return page;
}
// 404EE0's CSpriteEx geometry and texture coordinates. The generic RetDec
// wrapper lost the queried texture dimensions; use the actual texture store.
void rectangle(kinoko::render::QuadView quad, int32_t handle, int32_t x, int32_t y, int32_t w, int32_t h) {
    using Quad = kinoko::render::QuadRecord;
    quad.set(&Quad::texture, handle);
    if (handle) {
        const float tw = static_cast<float>(kinoko_texture_slots[handle].width);
        const float th = static_cast<float>(kinoko_texture_slots[handle].height);
        quad.set(&Quad::texture_width, tw); quad.set(&Quad::texture_height, th);
        const float du = w / tw, dv = h / th, u = x / tw, v = y / th;
        quad.set(&Quad::source_u_extent, du); quad.set(&Quad::source_v_extent, dv);
        auto vertices = quad.get(&Quad::vertices);
        vertices[0].u = vertices[2].u = u;
        vertices[0].v = vertices[1].v = v;
        vertices[1].u = vertices[3].u = du + u;
        vertices[2].v = vertices[3].v = dv + v;
        quad.set(&Quad::vertices, vertices);
    } else {
        quad.set(&Quad::texture_width, 0.0f); quad.set(&Quad::texture_height, 0.0f);
        quad.set(&Quad::source_u_extent, 0.0f); quad.set(&Quad::source_v_extent, 0.0f);
    }
    auto vertices = quad.get(&Quad::vertices);
    for (auto& vertex : vertices) vertex.color = 0xffffffffu;
    quad.set(&Quad::vertices, vertices);
    quad.set(&Quad::base_positions, std::array<kinoko::render::Position3, 4>{
        kinoko::render::Position3{-0.0f, -0.0f, 0.0f}, {static_cast<float>(w), -0.0f, 0.0f},
        {-0.0f, static_cast<float>(h), 0.0f}, {static_cast<float>(w), static_cast<float>(h), 0.0f}});

}

}
extern "C" int32_t kinoko_string_add_character(KinokoStringLayout* layout,const char* character) {
    const RecordView<LayoutRecord> text(layout);
    int32_t cursor=text.get(&LayoutRecord::cursor_x);
    const auto font_height=text.get(&LayoutRecord::font_height);
    if(*character=='\t') {
        const int32_t tab=4*font_height;
        text.set(&LayoutRecord::cursor_x,cursor+tab-cursor%tab);
        return 1;
    }
    if(*character=='\n') {
        text.set(&LayoutRecord::cursor_y,
            text.get(&LayoutRecord::cursor_y)+text.get(&LayoutRecord::line_height));
        text.set(&LayoutRecord::cursor_x,0);
        text.set(&LayoutRecord::line_height,font_height);
        return 1;
    }
    for(;;) {
        if(kinoko_string_atlas_size((KinokoStringLayout*)(uintptr_t)(layout))==0) new_page(layout);
        auto* page=kinoko_string_atlas_at((KinokoStringLayout*)(uintptr_t)(layout), kinoko_string_atlas_size((KinokoStringLayout*)(uintptr_t)(layout))-1);
        const RecordView<AtlasRecord> atlas(page);
        kinoko_string_font_configure(atlas.bytes(&AtlasRecord::renderer), layout);
        int32_t width=0,height=0;
        kinoko_string_font_upload(atlas.bytes(&AtlasRecord::renderer), atlas.get(&AtlasRecord::texture), character, atlas.get(&AtlasRecord::cursor_x), atlas.get(&AtlasRecord::cursor_y), &width, &height);
        if(width>=atlas.get(&AtlasRecord::width) || height>=atlas.get(&AtlasRecord::height)) return 0;
        if(atlas.get(&AtlasRecord::cursor_x)+width>=atlas.get(&AtlasRecord::width) ||
            (!width && atlas.get(&AtlasRecord::height)-atlas.get(&AtlasRecord::row_height)-
                atlas.get(&AtlasRecord::cursor_y)>font_height)) {
            atlas.set(&AtlasRecord::cursor_y,
                atlas.get(&AtlasRecord::cursor_y)+atlas.get(&AtlasRecord::row_height));
            atlas.set(&AtlasRecord::cursor_x,0);
            atlas.set(&AtlasRecord::row_height,0);
            continue;
        }
        if(atlas.get(&AtlasRecord::cursor_y)+height>=atlas.get(&AtlasRecord::height) || !height) {
            new_page(layout);continue;
        }
        // 440A9B-440BF4 is absent from IDA's decompilation: width/height are
        // output parameters of 405F80, not constants. Follow the assembly.
        atlas.set(&AtlasRecord::references,atlas.get(&AtlasRecord::references)+1);
        auto* glyph=kinoko_string_append_glyph(layout);
        const RecordView<GlyphRecord> sprite(glyph);
        auto quad=sprite.view(&GlyphRecord::quad);
        rectangle(quad,atlas.get(&AtlasRecord::texture),atlas.get(&AtlasRecord::cursor_x),
            atlas.get(&AtlasRecord::cursor_y),width,height);
        quad.set(&kinoko::render::QuadRecord::positions, quad.get(&kinoko::render::QuadRecord::base_positions));
        sprite.set(&GlyphRecord::x,cursor);
        sprite.set(&GlyphRecord::y,text.get(&LayoutRecord::cursor_y));
        sprite.set(&GlyphRecord::width,width);
        sprite.set(&GlyphRecord::height,height);
        const auto id=text.get(&LayoutRecord::next_glyph_id);
        sprite.set(&GlyphRecord::id,id);
        atlas.set(&AtlasRecord::last_glyph_id,id);
        sprite.set(&GlyphRecord::atlas,page);
        text.set(&LayoutRecord::next_glyph_id,id+1);
        atlas.set(&AtlasRecord::row_height,(std::max)(atlas.get(&AtlasRecord::row_height),height));
        atlas.set(&AtlasRecord::cursor_x,atlas.get(&AtlasRecord::cursor_x)+width);
        cursor+=width;
        auto line_height=(std::max)(text.get(&LayoutRecord::line_height),height);
        if(text.get(&LayoutRecord::wrap_width)>=0 && cursor>=text.get(&LayoutRecord::wrap_width)) {
            text.set(&LayoutRecord::cursor_y,text.get(&LayoutRecord::cursor_y)+line_height);
            cursor=0;line_height=font_height;
        }
        text.set(&LayoutRecord::cursor_x,cursor);
        text.set(&LayoutRecord::line_height,line_height);
        text.set(&LayoutRecord::maximum_width,(std::max)(text.get(&LayoutRecord::maximum_width),cursor));
        return 1;
    }
}
