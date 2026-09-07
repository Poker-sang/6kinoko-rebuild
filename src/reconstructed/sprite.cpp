#include "kinoko/sprite.h"

#include <cstddef>
#include <d3d9.h>

extern "C" {
extern int32_t g678;
int32_t retdec_set_texture_stage(int32_t stage, int32_t handle);
float function_404130(long double angle);
float function_4040d0(long double angle);
}

static_assert(sizeof(KinokoSpriteVertex) == 28, "Original vertex stride");
static_assert(offsetof(KinokoSprite, vertices) == 8 &&
              offsetof(KinokoSprite, width) == 120 &&
              offsetof(KinokoSprite, angle) == 144 && sizeof(KinokoSprite) == 148,
              "Original CSprite field offsets");

extern "C" void kinoko_sprite_transform(KinokoSprite *sprite, float x, float y) {
    auto &vertices = sprite->vertices;
    vertices[0].x = x - sprite->pivot_x * sprite->scale_x - 0.5f;
    vertices[0].y = y - sprite->pivot_y * sprite->scale_y - 0.5f;
    vertices[1].x = sprite->width * sprite->scale_x + vertices[0].x;
    vertices[1].y = vertices[0].y;
    vertices[2].x = vertices[0].x;
    vertices[2].y = vertices[0].y + sprite->height * sprite->scale_y;
    vertices[3].y = vertices[2].y;
    vertices[3].x = vertices[1].x;

    if (sprite->angle != 0.0f) {
        const float cosine = function_404130(sprite->angle);
        const float sine = function_4040d0(sprite->angle);
        for (auto &vertex : vertices) {
            const float old_x = vertex.x;
            const float old_y = vertex.y;
            vertex.x = (old_x - x) * cosine + x - (old_y - y) * sine;
            vertex.y = (old_x - x) * sine + y + (old_y - y) * cosine;
        }
    }
}

namespace {
int32_t draw(KinokoSprite *sprite, float x, float y, DWORD format) {
    if (!sprite)
        return 0;
    kinoko_sprite_transform(sprite, x, y);
    auto *device = reinterpret_cast<IDirect3DDevice9 *>(static_cast<uintptr_t>(
        static_cast<uint32_t>(g678)));
    if (!device || !*reinterpret_cast<void ***>(device))
        return 0;
    if (retdec_set_texture_stage(0, sprite->texture) < 0)
        return E_FAIL;
    device->SetFVF(format);
    return device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, sprite->vertices,
        sizeof(KinokoSpriteVertex));
}
}

// Original CSprite vtable entries at 4ED2E8/4ED2EC/4ED2F0.
// EDX is unused; x and y remain callee-cleaned stack arguments.
extern "C" int32_t __fastcall kinoko_sprite_draw_404770(
    KinokoSprite *sprite, void *, float x, float y) {
    return draw(sprite, x, y, 324u);
}

extern "C" int32_t __fastcall kinoko_sprite_draw_4049c0(
    KinokoSprite *sprite, void *, float x, float y) {
    return draw(sprite, x, y, 324u);
}

extern "C" int32_t __fastcall kinoko_sprite_draw_404bc0(
    KinokoSprite *sprite, void *, float x, float y) {
    return draw(sprite, x, y, 16706u);
}
