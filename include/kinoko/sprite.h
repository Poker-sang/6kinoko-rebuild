#pragma once

#include <stdint.h>

typedef struct KinokoSpriteVertex {
    float x, y, z, rhw;
    uint32_t color;
    float u, v;
} KinokoSpriteVertex;

typedef struct KinokoSprite {
    void *vtable;
    int32_t texture;
    KinokoSpriteVertex vertices[4];
    float width, height;
    float pivot_x, pivot_y;
    float scale_x, scale_y;
    float angle;
} KinokoSprite;

#ifdef __cplusplus
extern "C" {
#endif

void kinoko_sprite_transform(KinokoSprite *sprite, float x, float y);
int32_t __fastcall kinoko_sprite_draw_404770(KinokoSprite *sprite, void *unused, float x, float y);
int32_t __fastcall kinoko_sprite_draw_4049c0(KinokoSprite *sprite, void *unused, float x, float y);
int32_t __fastcall kinoko_sprite_draw_404bc0(KinokoSprite *sprite, void *unused, float x, float y);

#ifdef __cplusplus
}
#endif
