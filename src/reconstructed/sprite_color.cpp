#include "kinoko/sprite.h"

#include <cstddef>

namespace {
struct FrameAppearance {
    int32_t blend;
    uint32_t color;
    float scale_x, scale_y, roll_x, roll_y, roll_z;
};

struct PatFrame {
    KinokoColoredQuad quad;
    float texture_width, texture_height;
    float base_positions[4][3];
    float positions[4][3];
    float source_width, source_height;
    int16_t sprite_x, sprite_y, pivot_x, pivot_y, duration;
    uint16_t padding;
    FrameAppearance *appearance;
};

static_assert(offsetof(KinokoColoredQuad, vertices) == 8);
static_assert(offsetof(KinokoSpriteVertex, color) == 16);
static_assert(sizeof(KinokoColoredQuad) == 120);
static_assert(offsetof(PatFrame, appearance) == 244 && sizeof(PatFrame) == 248);
static_assert(offsetof(FrameAppearance, color) == 4 && sizeof(FrameAppearance) == 28);
}

// Original 42B280/42B2A0: explicit ECX receiver and callee-cleaned argument.
extern "C" uint32_t __fastcall kinoko_quad_set_color(
    KinokoColoredQuad *quad, void *, uint32_t color) {
    for (auto &vertex : quad->vertices)
        vertex.color = color;
    return color;
}

extern "C" uint32_t __fastcall kinoko_quad_set_vertex_colors(
    KinokoColoredQuad *quad, void *, const uint32_t *colors) {
    for (size_t i = 0; i < 4; ++i)
        quad->vertices[i].color = colors[i];
    return colors[3];
}

// Original 42B2D0 uses vertex zero, integer division by 255 (not a shift),
// and broadcasts the result to all four vertices, including alpha.
extern "C" uint32_t __fastcall kinoko_quad_modulate_color(
    KinokoColoredQuad *quad, void *, uint32_t color) {
    const uint32_t base = quad->vertices[0].color;
    uint32_t result = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        const uint32_t channel = ((base >> shift) & 255u) *
            ((color >> shift) & 255u) / 255u;
        result |= channel << shift;
    }
    return kinoko_quad_set_color(quad, nullptr, result);
}

// Actor::Render 45F283..45F2EB resets the PAT base color on every draw,
// then modulates it by Actor ARGB. Never compound last frame's result.
extern "C" void kinoko_actor_frame_color(void *address, uint32_t actor_color) {
    auto &frame = *static_cast<PatFrame *>(address);
    if (frame.appearance) {
        kinoko_quad_set_color(&frame.quad, nullptr, frame.appearance->color);
        kinoko_quad_modulate_color(&frame.quad, nullptr, actor_color);
    } else {
        kinoko_quad_set_color(&frame.quad, nullptr, actor_color);
    }
}
