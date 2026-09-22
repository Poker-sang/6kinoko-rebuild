#include "kinoko/quad_render.h"
#include "kinoko/map_render.h"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/map_query.hpp"
#include "kinoko/act_host.h"
#include "kinoko/native_buffer.h"
#include "kinoko/texture_store.h"
#include "kinoko/diagnostics.h"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <vector>
#include <cstring>

namespace {
using namespace kinoko::map;
using namespace kinoko::render;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
struct VisibleChip { retdec_mcd_chip *chip; Placement *placement; };

bool initialize_quad(QuadRecord *storage, int32_t handle, const unsigned char *bytes) {
    if (!storage || !bytes || handle <= 0 || static_cast<uint32_t>(handle) >= KINOKO_TEXTURE_CAPACITY)
        return false;
    const auto &texture = kinoko_texture_slots[handle];
    if (!texture.width || !texture.height) return false;
    const auto chip = ChipView(const_cast<unsigned char *>(bytes)).load();
    if (chip.width <= 0 || chip.height <= 0) return false;
    const QuadView quad(storage);
    quad.clear();
    quad.set(&QuadRecord::texture, handle);
    quad.set(&QuadRecord::texture_width, static_cast<float>(texture.width));
    quad.set(&QuadRecord::texture_height, static_cast<float>(texture.height));
    const float u0 = static_cast<float>(chip.source_left) / texture.width;
    const float v0 = static_cast<float>(chip.source_top) / texture.height;
    const float u1 = static_cast<float>(chip.source_left + chip.width) / texture.width;
    const float v1 = static_cast<float>(chip.source_top + chip.height) / texture.height;
    std::array<KinokoSpriteVertex, 4> vertices{};
    vertices[0].u=u0; vertices[0].v=v0;
    vertices[1].u=u1; vertices[1].v=v0;
    vertices[2].u=u0; vertices[2].v=v1;
    vertices[3].u=u1; vertices[3].v=v1;
    for (auto &vertex : vertices) vertex.color=0xffffffffu;
    quad.set(&QuadRecord::vertices, vertices);
    const float width=static_cast<float>(chip.width), height=static_cast<float>(chip.height);
    const std::array<Position3,4> positions={{{0,0,0},{width,0,0},{0,height,0},{width,height,0}}};
    quad.set(&QuadRecord::base_positions, positions);
    quad.set(&QuadRecord::positions, positions);
    return true;
}

Position3 world_position(KinokoActLayer *layer) {
    Position3 result{};
    // Original virtual GetWorldPosition (+28), not a guessed parent traversal.
    using GetPosition = void (__thiscall *)(KinokoActLayer *, float *, float *, float *);
    struct Methods { const void *prefix[7]; GetPosition get_position; };
    const auto *methods=LayerView(layer).get(&LayerRecord::methods);
    auto get=kinoko::legacy::load<GetPosition>(methods+offsetof(Methods,get_position));
    get(layer,&result.x,&result.y,&result.z);
    return result;
}
}

extern "C" int32_t retdec_map_sprite_init(int32_t sprite, int32_t handle, const unsigned char *bytes) {
    return initialize_quad(pointer<QuadRecord>(sprite),handle,bytes);
}

extern "C" int32_t kinoko_map_update_visible(KinokoActLayout *layout,
    int32_t left, int32_t top, int32_t right, int32_t bottom) {
    if (!layout) return E_FAIL;
    const LayoutView map(layout);
    auto *layer=map.get(&LayoutRecord::owning_layer);
    if (!layer) return E_FAIL;
    if (!LayerView(layer).get(&LayerRecord::visible)) return 0;
    auto *data=kinoko_map_query_chip_data(layout);
    layer=map.get(&LayoutRecord::owning_layer);
    if (!data || !layer || LayerView(layer).get(&LayerRecord::resource) !=
        map.get(&LayoutRecord::cached_chip_resource)) return E_FAIL;

    std::vector<VisibleChip> visible;
    auto cache=map.get(&LayoutRecord::render_scan_cache);
    if (!query_visible(layout,&cache,left,top,right,bottom,
        [&](retdec_mcd_chip *chip, Placement *record, int32_t) {
            visible.push_back({chip,record}); return true;
        })) return E_FAIL;
    map.set(&LayoutRecord::render_scan_cache,cache);
    map.set(&LayoutRecord::render_count,int32_t{0});
    if (visible.empty()) return 0;
    if (visible.size() > INT32_MAX/sizeof(QuadRecord)) return E_FAIL;
    auto buffer=map.view(&LayoutRecord::render_quads);
    if (!kinoko_native_buffer_resize(address(buffer.data()),
        static_cast<uint32_t>(visible.size()*sizeof(QuadRecord)))) return E_FAIL;
    const auto origin=world_position(layer);
    const float scale=map.get(&LayoutRecord::scale);
    auto *output=buffer.get(&QuadBuffer::begin);
    for (size_t i=0;i<visible.size();++i) {
        const QuadView quad(output+i);
        quad.clear(); // invisible/missing textures keep their ordered slot
        const auto record=PlacementView(visible[i].placement).load();
        if (!record.visible) continue;
        const auto chip=ChipView(visible[i].chip->bytes).load();
        auto *texture=retdec_mcd_find_texture(data,chip.texture_id);
        if (!texture || !initialize_quad(output+i,texture->handle,visible[i].chip->bytes)) continue;
        // 434DC7/434DDA store integer subtraction + world origin before translation.
        const float x=static_cast<float>(static_cast<double>(record.left)-left+origin.x);
        const float y=static_cast<float>(static_cast<double>(record.top)-top+origin.y);
        auto positions=quad.get(&QuadRecord::positions);
        for (auto &position:positions) {
            position.x+=x; position.y+=y; position.z+=origin.z;
            position.x*=scale; position.y*=scale; position.z*=scale;
        }
        quad.set(&QuadRecord::positions,positions);
        const auto alpha=std::clamp(static_cast<int32_t>(record.alpha*map.get(&LayoutRecord::alpha)*255.0f),0,255);
        auto vertices=quad.get(&QuadRecord::vertices);
        for (auto &vertex:vertices) vertex.color=(vertex.color&0x00ffffffu)|(static_cast<uint32_t>(alpha)<<24);
        quad.set(&QuadRecord::vertices,vertices);
    }
    map.set(&LayoutRecord::render_count,static_cast<int32_t>(visible.size()));
    retdec_trace_i32("map:update-draw-records",static_cast<int32_t>(visible.size()));
    return 0;
}

extern "C" int32_t kinoko_map_draw_visible(KinokoActLayout *layout,float x,float y) {
    if (!layout) return E_FAIL;
    const LayoutView map(layout);
    auto *layer=map.get(&LayoutRecord::owning_layer);
    if (!layer) return E_FAIL;
    const LayerView owner(layer);
    if (!owner.get(&LayerRecord::visible)) return 0;
    if (!owner.get(&LayerRecord::resource) || owner.get(&LayerRecord::resource)!=
        map.get(&LayoutRecord::cached_chip_resource)) return E_FAIL;
    auto *device=pointer<IDirect3DDevice9>(g678);
    if (!device) return E_FAIL; // inherited unavailable-device boundary
    DWORD address_u=0,address_v=0;
    device->GetSamplerState(0,D3DSAMP_ADDRESSU,&address_u);
    device->GetSamplerState(0,D3DSAMP_ADDRESSV,&address_v);
    device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
    device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
    kinoko_render_set_depth(0,0);
    kinoko_render_set_blend(1);
    kinoko_render_set_alpha(1,0);
    const auto blend=map.get(&LayoutRecord::blend);
    if (blend>=1 && blend<=4) kinoko_render_set_blend(blend);
    auto *quads=map.get(&LayoutRecord::render_quads).begin;
    for (int32_t index=0;index<map.get(&LayoutRecord::render_count);++index) {
        if (QuadView(quads+index).get(&QuadRecord::texture))
            kinoko_quad_submit(reinterpret_cast<KinokoQuad *>(quads+index),x,y);
    }
    retdec_set_texture_stage(0,0);
    device->SetSamplerState(0,D3DSAMP_ADDRESSU,address_u);
    device->SetSamplerState(0,D3DSAMP_ADDRESSV,address_v);
    return 0; // original ignores per-quad HRESULT and continues
}

extern "C" int32_t kinoko_map_update(int32_t layout,int32_t left,int32_t top,int32_t right,int32_t bottom) {
    return kinoko_map_update_visible(pointer<KinokoActLayout>(layout),left,top,right,bottom);
}
extern "C" int32_t kinoko_map_draw(int32_t layout,float x,float y) {
    return kinoko_map_draw_visible(pointer<KinokoActLayout>(layout),x,y);
}

extern "C" KinokoActLayer *__fastcall kinoko_act_layer_world_position(
    KinokoActLayer *layer, void *, float *x, float *y, float *z) {
    if (!x || !y || !z) return layer;
    *x=*y=*z=0.0f;
    while (layer) {
        const LayerView current(layer);
        *x=current.get(&LayerRecord::position_x)+*x;
        *y=current.get(&LayerRecord::position_y)+*y;
        *z=current.get(&LayerRecord::position_z)+*z;
        layer=current.get(&LayerRecord::parent);
    }
    return layer;
}
