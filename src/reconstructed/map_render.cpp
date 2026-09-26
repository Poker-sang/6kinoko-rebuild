#include "kinoko/map_render.h"
#include <cstddef>
#include "kinoko/map_layout_records.hpp"
#include "kinoko/actor_records.hpp"

namespace {
using MapRenderLayer = kinoko::map::RenderLayerRecord;
using Camera = kinoko::actor::CameraBoundsRecord;
}

extern "C" int32_t __fastcall kinoko_map_update_all_entry(KinokoActLayout* layout, void *) {
    return kinoko_map_update_visible(layout,
        0, 0, INT32_MAX, INT32_MAX);
}

extern "C" int32_t __fastcall kinoko_map_update_visible_entry(
    KinokoActLayout* layout, void *, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    return kinoko_map_update_visible(layout,
        left, top, right, bottom);
}

extern "C" int32_t __fastcall kinoko_map_draw_entry(
    KinokoActLayout* layout, void *, float x, float y) {
    return kinoko_map_draw_visible(layout, x, y);
}

// Original 46EED0: camera rectangle (+32 right/bottom), then camera offset.
// __fastcall supplies ECX and the original callee-cleaned stack without assembly.
extern "C" int32_t __fastcall kinoko_map_render_layer_entry(
    KinokoRenderLayer* layer_address, void *, KinokoCamera* camera_address) {
    if (!layer_address) return 0;
    const auto layer=kinoko::native::RecordView<MapRenderLayer>(
        layer_address).load();
    if (!layer.layout) return 0;
    int32_t left=0,top=0,right=0,bottom=0;
    float x=0,y=0;
    if (camera_address) {
        const auto camera=kinoko::native::RecordView<Camera>(
            camera_address).load();
        left=static_cast<int32_t>(camera.bounds.left);
        top=static_cast<int32_t>(camera.bounds.top);
        right=static_cast<int32_t>(camera.bounds.right)+32;
        bottom=static_cast<int32_t>(camera.bounds.bottom)+32;
        x=-camera.offset_x;y=-camera.offset_y;
    }
    using Update = int32_t (__thiscall *)(KinokoActLayout *,int32_t,int32_t,int32_t,int32_t);
    using Draw = int32_t (__thiscall *)(KinokoActLayout *,float,float);
    struct Methods { void *prefix[8]; Draw draw; void *update_all; Update update; };
    auto *methods=kinoko::map::LayoutView(layer.layout).get(&kinoko::map::LayoutRecord::methods);
    const auto update=kinoko::legacy::load<Update>(methods+offsetof(Methods,update));
    update(layer.layout,left,top,right,bottom);
    // Reload table after virtual update, preserving the original dispatch order.
    methods=kinoko::map::LayoutView(layer.layout).get(&kinoko::map::LayoutRecord::methods);
    return kinoko::legacy::load<Draw>(methods+offsetof(Methods,draw))(layer.layout,x,y);
}
