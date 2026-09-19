#include "kinoko/sprite.h"
#include "kinoko/texture_store.h"

// 404610 dispatches slot 4, supplying zero pivots; keep virtual dispatch for
// derived sprites instead of replacing it with a direct concrete call.
extern "C" int32_t __fastcall kinoko_sprite_set_rect(KinokoSprite* sprite, void*,
    int32_t texture, int32_t x, int32_t y, int32_t width, int32_t height) {
    using SetRect = int32_t (__thiscall*)(KinokoSprite*, int32_t, int32_t, int32_t,
        int32_t, int32_t, int32_t, int32_t);
    const auto table = static_cast<void**>(sprite->vtable);
    return reinterpret_cast<SetRect>(table[4])(sprite, texture, x, y, width, height, 0, 0);
}

// 404640: the last two arguments are pivots, not texture dimensions. 405EA0
// replaces temporary stack copies with the dimensions of the texture; handle
// zero has the original 256x256 default. Coordinates x/y of vertices survive.
extern "C" int32_t __fastcall kinoko_sprite_set_rect_pivot(KinokoSprite* sprite, void*,
    int32_t texture, int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t pivot_x, int32_t pivot_y) {
    sprite->width = static_cast<float>(width); sprite->height = static_cast<float>(height);
    sprite->texture = texture;
    sprite->pivot_x = static_cast<float>(pivot_x); sprite->pivot_y = static_cast<float>(pivot_y);
    const uint32_t texture_width = texture ? kinoko_texture_slots[texture].width : 256;
    const uint32_t texture_height = texture ? kinoko_texture_slots[texture].height : 256;
    const float u0 = static_cast<float>(static_cast<double>(x) / texture_width);
    const float v0 = static_cast<float>(static_cast<double>(y) / texture_height);
    const float u1 = static_cast<float>(static_cast<double>(static_cast<int32_t>(
        static_cast<uint32_t>(x) + static_cast<uint32_t>(width))) / texture_width);
    const float v1 = static_cast<float>(static_cast<double>(static_cast<int32_t>(
        static_cast<uint32_t>(y) + static_cast<uint32_t>(height))) / texture_height);
    for (int i = 0; i < 4; ++i) {
        auto& vertex = sprite->vertices[i];
        vertex.u = (i & 1) ? u1 : u0; vertex.v = (i & 2) ? v1 : v0;
        vertex.color = 0xffffffffu; vertex.z = 0.5f; vertex.rhw = 1;
    }
    sprite->scale_x = sprite->scale_y = 1;
    sprite->angle = 0;
    return -1; // Original EAX result; the callers consume the written geometry.
}
