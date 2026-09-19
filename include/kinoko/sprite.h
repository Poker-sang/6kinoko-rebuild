#pragma once

#include <stdint.h>

typedef struct KinokoSpriteVertex {
    float x, y, z, rhw;
    uint32_t color;
    float u, v;
} KinokoSpriteVertex;

/* Shared prefix of CSprite, the 232-byte quad and the 248-byte PAT frame. */
typedef struct KinokoColoredQuad {
    void *vtable;
    int32_t texture;
    KinokoSpriteVertex vertices[4];
} KinokoColoredQuad;

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
int32_t __fastcall kinoko_sprite_set_rect(KinokoSprite *sprite, void *unused,
    int32_t texture, int32_t x, int32_t y, int32_t width, int32_t height);
int32_t __fastcall kinoko_sprite_set_rect_pivot(KinokoSprite *sprite, void *unused,
    int32_t texture, int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t pivot_x, int32_t pivot_y);
uint32_t __fastcall kinoko_quad_set_color(KinokoColoredQuad *quad, void *unused, uint32_t color);
uint32_t __fastcall kinoko_quad_set_vertex_colors(KinokoColoredQuad *quad, void *unused, const uint32_t *colors);
uint32_t __fastcall kinoko_quad_modulate_color(KinokoColoredQuad *quad, void *unused, uint32_t color);
void kinoko_actor_frame_color(void *frame, uint32_t actor_color);
int32_t __fastcall kinoko_sprite_draw_404770(KinokoSprite *sprite, void *unused, float x, float y);
int32_t __fastcall kinoko_sprite_draw_4049c0(KinokoSprite *sprite, void *unused, float x, float y);
int32_t __fastcall kinoko_sprite_draw_404bc0(KinokoSprite *sprite, void *unused, float x, float y);

#ifdef __cplusplus
}
#endif
