#include "kinoko/map_render.h"
#include <cstddef>

namespace {
struct MapRenderLayer {
    void *vtable;
    int32_t layout;
};
struct Camera {
    unsigned char prefix[56];
    float offset_x, offset_y;
    float reserved[2];
    float left, top, right, bottom;
};
static_assert(offsetof(MapRenderLayer, layout) == 4);
static_assert(offsetof(Camera, left) == 72 && offsetof(Camera, bottom) == 84);
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
        left = static_cast<int32_t>(camera->left);
        top = static_cast<int32_t>(camera->top);
        right = static_cast<int32_t>(camera->right) + 32;
        bottom = static_cast<int32_t>(camera->bottom) + 32;
        x = -camera->offset_x;
        y = -camera->offset_y;
    }
    kinoko_map_update(layer->layout, left, top, right, bottom);
    return kinoko_map_draw(layer->layout, x, y);
}
