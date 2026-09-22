#pragma once
#include "kinoko/sprite.h"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstdint>
namespace kinoko::render {
struct Position3 { float x, y, z; };
// Borrowed texture handle. The enclosing layout/animation owns this storage.
struct QuadRecord {
    uint32_t vtable;
    int32_t texture;
    std::array<KinokoSpriteVertex, 4> vertices;
    float texture_width, texture_height;
    std::array<Position3, 4> base_positions, positions;
    float source_u_extent, source_v_extent;
};
using QuadView = native::RecordView<QuadRecord>;
static_assert(sizeof(QuadRecord) == 232);
static_assert(offsetof(QuadRecord, positions) == 176);
}
