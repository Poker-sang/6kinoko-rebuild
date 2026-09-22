#include "kinoko/sprite.h"
#include "kinoko/actor_records.hpp"

#include <cstddef>

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
    using namespace kinoko::actor;
    const kinoko::native::RecordView<FrameRecord> frame(address);
    auto vertices=frame.get(&FrameRecord::vertices);
    uint32_t color=actor_color;
    if (const auto *appearance=frame.get(&FrameRecord::owned_payload)) {
        const auto base=kinoko::native::RecordView<FrameAppearance>(const_cast<FrameAppearance *>(appearance)).get(&FrameAppearance::color);
        color=0;
        for (unsigned shift=0;shift<32;shift+=8)
            color|=(((base>>shift)&255u)*((actor_color>>shift)&255u)/255u)<<shift;
    }
    for (auto &vertex:vertices) vertex.color=color;
    frame.set(&FrameRecord::vertices,vertices);
}
