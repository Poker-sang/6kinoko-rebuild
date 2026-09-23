#pragma once
#include <cstdint>
#include <cstddef>

namespace kinoko::text {
struct AtlasLifecycle {
    int32_t cursor_x, cursor_y, row_height, width, height;
    int32_t last_glyph_id;
    unsigned char unknown24[404];
    int32_t texture;
    int32_t references;
};
static_assert(sizeof(AtlasLifecycle) == 436);
static_assert(offsetof(AtlasLifecycle, last_glyph_id) == 20);
static_assert(offsetof(AtlasLifecycle, width) == 12);
static_assert(offsetof(AtlasLifecycle, texture) == 428);
static_assert(offsetof(AtlasLifecycle, references) == 432);
}
