#include "kinoko/map_render.h"
#include <cstddef>
#include "kinoko/map_layout_records.hpp"
#include "kinoko/actor_records.hpp"

namespace {
using MapRenderLayer = kinoko::map::RenderLayerRecord;
using Camera = kinoko::actor::CameraBoundsRecord;
}

extern "C" int32_t __fastcall kinoko_map_entry_434b40(int32_t layout, void *) {
    return kinoko_map_update(layout, 0, 0, INT32_MAX, INT32_MAX);
}

extern "C" int32_t __fastcall kinoko_map_entry_434b60(
    int32_t layout, void *, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    return kinoko_map_update(layout, left, top, right, bottom);
}

extern "C" int32_t __fastcall kinoko_map_entry_434f40(
    int32_t layout, void *, float x, float y) {
    return kinoko_map_draw(layout, x, y);
}

// Original 46EED0: camera rectangle (+32 right/bottom), then camera offset.
// __fastcall supplies ECX and the original callee-cleaned stack without assembly.
extern "C" int32_t __fastcall kinoko_map_entry_46eed0(
    int32_t layer_address, void *, int32_t camera_address) {
    auto *layer = reinterpret_cast<MapRenderLayer *>(layer_address);
    auto *camera = reinterpret_cast<Camera *>(camera_address);
    if (!layer || !layer->layout) return 0;
    int32_t left = 0, top = 0, right = 0, bottom = 0;
    float x = 0, y = 0;
    if (camera) {
        left = static_cast<int32_t>(camera->bounds.left);
        top = static_cast<int32_t>(camera->bounds.top);
        right = static_cast<int32_t>(camera->bounds.right) + 32;
        bottom = static_cast<int32_t>(camera->bounds.bottom) + 32;
        x = -camera->offset_x;
        y = -camera->offset_y;
    }
    kinoko_map_update_visible(layer->layout, left, top, right, bottom);
    return kinoko_map_draw_visible(layer->layout, x, y);
}
