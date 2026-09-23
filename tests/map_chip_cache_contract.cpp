// Compile-only handoff: no game or local contract execution by the agent.
#include "kinoko/map_chip_cache.hpp"
#include "kinoko/act_host.h"
#include "kinoko/map_render.h"
#include "kinoko/act_runtime.h"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/texture_store.h"
#include "kinoko/native_buffer.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
using namespace kinoko::map;
using kinoko::legacy::address;
extern "C" {
unsigned char g25,g23,g327,g328;
KinokoGraphics kinoko_graphics{};
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY]{};
retdec_mcd_data *kinoko_map_cached_chip_data(KinokoActLayout *layout) {
    if(!layout) return nullptr;
    const auto *resource=LayoutView(layout).get(&LayoutRecord::cached_chip_resource);
    return resource ? ChipResourceView(const_cast<KinokoActResource*>(resource)).get(&ChipResourceRecord::data) : nullptr;
}
retdec_mcd_data *kinoko_map_query_chip_data(KinokoActLayout *layout) { return kinoko_map_cached_chip_data(layout); }
retdec_mcd_chip *retdec_mcd_find_chip(retdec_mcd_data *data,uint32_t id) {
    if(data) for(uint32_t i=0;i<data->chip_count;++i) if(data->chips[i].chip_id==id) return data->chips+i;
    return nullptr;
}
retdec_mcd_texture *retdec_mcd_find_texture(retdec_mcd_data *data,uint32_t id) {
    if(data) for(uint32_t i=0;i<data->texture_count;++i) if(data->textures[i].texture_id==id) return data->textures+i;
    return nullptr;
}
void retdec_trace_i32(const char*,int32_t) {}
const KinokoActHostSymbols *kinoko_act_host_symbols() {
    static KinokoActHostSymbols symbols{};
    symbols.map_layout_vtable = &g327;
    symbols.map_view_vtable = &g328;
    symbols.color_vtable = &g23;
    return &symbols;
}
int32_t kinoko_render_set_depth(int32_t,int32_t) { return 0; }
int32_t kinoko_render_set_alpha(int32_t,int32_t) { return 0; }
int32_t kinoko_render_set_blend(int32_t) { return 0; }
int32_t kinoko_texture_bind_stage(int32_t,int32_t) { return 0; }
int32_t kinoko_quad_submit(KinokoQuad*,float,float) { return 0; }
}
static void __fastcall world(KinokoActLayer*,void*,float *x,float *y,float *z) { *x=3;*y=4;*z=0; }
static const ChipDefinition *definition(retdec_mcd_chip &chip) { return reinterpret_cast<const ChipDefinition*>(chip.bytes); }
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"MCD line %d\n",__LINE__);return 1; } } while(0)
int main() {
    retdec_mcd_chip chips[4]{};
    const uint32_t ids[]={9,2,0,1};
    for(int i=0;i<4;++i) {
        chips[i].chip_id=ids[i];ChipDefinition value{};
        value.chip_id=ids[i];value.texture_id=10;value.width=16;value.height=24;
        std::memcpy(chips[i].bytes,&value,sizeof(value));
    }
    retdec_mcd_texture textures[]={{10,1},{11,2}};
    retdec_mcd_data data{4,chips,2,textures};
    ChipResourceRecord resource{};resource.data=&data;
    LayoutRecord map{};map.maximum_chip_id=-1;
    map.cached_chip_resource=reinterpret_cast<KinokoActResource*>(&resource);
    auto *layout=reinterpret_cast<KinokoActLayout*>(&map);
    kinoko_texture_slots[1]={nullptr,64,64};kinoko_texture_slots[2]={nullptr,128,128};
    rebuild_chip_index(layout);
    CHECK(map.chip_indices.end-map.chip_indices.begin==10);
    CHECK(map.chip_indices.begin[0]==0 && map.chip_indices.begin[1]==1 && map.chip_indices.begin[9]==3);
    CHECK(map.chip_indices.begin[3]==-1 && map.chip_definitions.begin[1].chip_id==1);
    CHECK(find_chip_sprite(layout,definition(chips[2]))==nullptr); // index zero falls back
    CHECK(find_chip_sprite(layout,definition(chips[3]))!=nullptr);
    CHECK(map.chip_sprite_count==10 && map.chip_sprites.begin[1].valid==1);
    CHECK(map.chip_sprites.begin[1].quad.vertices[3].u==0.25f);
    map.chip_sprites.begin[1].quad.positions[0].x=91; // unchanged definition is a cache hit
    CHECK(find_chip_sprite(layout,definition(chips[3]))->positions[0].x==91);
    ChipView(chips[3].bytes).set(&ChipDefinition::flags,uint32_t{123});
    CHECK(find_chip_sprite(layout,definition(chips[3]))->positions[0].x==0);
    CHECK(set_chip_rectangle(layout,1,16,8,32,16));
    CHECK(set_chip_rectangle(layout,1,32,8,16,16)); // duplicate pointers, latest definition
    CHECK(map.changed_chips.end-map.changed_chips.begin==2);
    CHECK(map.chip_definitions.begin[1].source_left==32);
    CHECK(map.chip_sprites.begin[1].definition.source_left==0); // deferred until update
    struct Methods { void *prefix[7];decltype(&world) position; } methods{{},world};
    LayerRecord layer{};layer.methods=reinterpret_cast<const unsigned char*>(&methods);
    layer.resource=map.cached_chip_resource;map.owning_layer=reinterpret_cast<KinokoActLayer*>(&layer);
    CHECK(kinoko_map_update_visible(layout,0,0,100,100)==0); // invisible owner keeps queue
    CHECK(map.changed_chips.end-map.changed_chips.begin==2);
    layer.visible=1;
    CHECK(kinoko_map_update_visible(layout,0,0,100,100)==0); // flush even with no visible placements
    CHECK(map.changed_chips.end==map.changed_chips.begin);
    CHECK(map.chip_sprites.begin[1].quad.vertices[0].u==0.5f);
    CHECK(!set_chip_rectangle(layout,9,7,8,9,10)); // ID >= definition count: mutates then fails
    CHECK(ChipView(chips[0].bytes).get(&ChipDefinition::source_left)==7);
    CHECK(map.changed_chips.end==map.changed_chips.begin);
    CHECK(!set_chip_rectangle(layout,0,4,5,6,7)); // sorted index zero: same mutation-before-failure
    CHECK(ChipView(chips[2].bytes).get(&ChipDefinition::width)==6);
    CHECK(!set_chip_rectangle(layout,77,1,2,3,4));
    ChipView(chips[3].bytes).set(&ChipDefinition::texture_id,uint32_t{99});
    CHECK(find_chip_sprite(layout,definition(chips[3]))==nullptr && !map.chip_sprites.begin[1].valid);
    textures[1].texture_id=99;
    CHECK(find_chip_sprite(layout,definition(chips[3]))->texture==2); // retry missing texture
    CHECK(set_chip_rectangle(layout,1,16,0,32,16));
    Placement placement{};placement.chip_id=1;placement.visible=1;placement.alpha=0.5f;placement.left=10;placement.top=20;
    kinoko_native_buffer_replace(address(&map.placements),&placement,sizeof(placement));
    map.alpha=1;map.scale=2;map.max_chip_width=map.max_chip_height=32;
    CHECK(kinoko_map_update_visible(layout,0,0,100,100)==0 && map.render_count==1);
    CHECK(map.render_quads.begin[0].positions[0].x==26 && map.render_quads.begin[0].positions[0].y==48);
    CHECK(map.render_quads.begin[0].vertices[0].color==0x7fffffffu);
    CHECK(map.chip_sprites.begin[1].quad.positions[0].x==0 && map.chip_sprites.begin[1].quad.vertices[0].color==0xffffffffu);
    CHECK(set_chip_rectangle(layout,1,32,0,32,16));
    auto *clone=reinterpret_cast<LayoutRecord*>(kinoko_clone_map_layout(address(&map),nullptr));
    CHECK(clone->chip_sprites.begin!=map.chip_sprites.begin && clone->chip_definitions.begin!=map.chip_definitions.begin);
    CHECK(clone->changed_chips.begin!=map.changed_chips.begin && clone->changed_chips.begin[0]==map.changed_chips.begin[0]);
    CHECK(clone->cached_chip_resource==map.cached_chip_resource && clone->suppress_next_binding==1);
    kinoko_clear_map_layout(address(clone));std::free(clone);
    CHECK(map.chip_sprites.begin[1].valid && map.changed_chips.begin[0]==definition(chips[3]));
    flush_changed_chips(layout);
    auto *previous=map.chip_sprites.begin;
    prepare_placements(layout); // rebuild keeps sprite storage and content comparison
    CHECK(map.chip_sprites.begin==previous && map.chip_definitions.begin[1].source_left==32);
    CHECK(map.max_chip_width==32 && map.max_chip_height==16);
    CHECK(map.chip_right==42 && map.chip_bottom==36);
    CHECK(set_chip_rectangle(layout,1,0,0,0,-16)); // signed/zero dimensions are valid original inputs
    flush_changed_chips(layout);
    CHECK(map.chip_sprites.begin[1].quad.positions[3].y==-16);
    kinoko_clear_map_layout(address(&map));
    CHECK(!map.chip_sprites.begin && !map.chip_definitions.begin && !map.changed_chips.begin && !map.chip_indices.begin);
    CHECK(resource.data==&data && textures[0].handle==1); // cache never owns MCD/textures
    return 0;
}
