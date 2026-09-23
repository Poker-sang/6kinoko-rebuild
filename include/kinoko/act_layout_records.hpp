#pragma once
#include "kinoko/act_types.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/legacy_string.hpp"
namespace kinoko::act {
using render::Position3;
struct Layout2DRecord {
    const void *methods;
    render::QuadRecord quad;
    Position3 rotation,rotation_pivot,scale,scale_pivot;
    float alpha;
    int32_t blend,red,green,blue;
    KinokoActLayer *layer;
    int32_t texture;
    uint8_t pivots_initialized;
    uint8_t padding313[3];
};
struct Layout3DRecord {
    const void *methods;
    Position3 translation,rotation,scale;
    KinokoActLayer *layer;
    float world[16];
};
struct StringLayoutRecord {
    const void *methods;
    legacy::StringRecord text;uint32_t unknown28;
    legacy::StringRecord pending;uint32_t unknown56;
    legacy::StringRecord face;uint32_t unknown84;
    int32_t font_height,font_weight;
    int32_t red,green,blue,base_red,base_green,base_blue;
    int32_t character_space,line_space;
    uint8_t edge;uint8_t padding129[3];
    int32_t alignment;
    float scale_x,scale_y;
    int32_t wrap_width;
    KinokoActLayer *layer;
    float alpha;
    int32_t blend;
    void *atlas_owner; // native vector owner, original +160
    uint8_t atlas_slots[12];
    void *glyph_owner; // native deque owner, original +176
    uint8_t glyph_slots[20];
    int32_t next_glyph_id,cursor_x,cursor_y,maximum_width,line_height,origin_x,origin_y;
    uint8_t rebuild;uint8_t padding229[3];
    uint8_t unknown232[28];
};
struct StringGlyphRecord {
    int32_t x,y,id,width,height;
    render::QuadRecord quad;
    void *atlas; // borrowed; layout owns atlas storage
};
static_assert(sizeof(Layout2DRecord)==316 && offsetof(Layout2DRecord,layer)==304);
static_assert(offsetof(Layout2DRecord,rotation)==236 && offsetof(Layout2DRecord,texture)==308);
static_assert(sizeof(Layout3DRecord)==108 && offsetof(Layout3DRecord,world)==44);
static_assert(sizeof(StringLayoutRecord)==260 && offsetof(StringLayoutRecord,layer)==148);
static_assert(offsetof(StringLayoutRecord,atlas_owner)==160 && offsetof(StringLayoutRecord,glyph_owner)==176);
static_assert(offsetof(StringLayoutRecord,next_glyph_id)==200 && offsetof(StringLayoutRecord,rebuild)==228);
static_assert(sizeof(StringGlyphRecord)==256 && offsetof(StringGlyphRecord,quad)==20);
}
