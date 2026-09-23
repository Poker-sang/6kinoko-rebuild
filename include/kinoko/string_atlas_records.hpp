#pragma once
#include <cstdint>
#include <cstddef>

namespace kinoko::text {
struct AtlasLifecycle {
    unsigned char unknown0[20];
    int32_t last_glyph_id;
    unsigned char unknown24[404];
    int32_t texture;
    int32_t references;
};
static_assert(sizeof(AtlasLifecycle) == 436);
static_assert(offsetof(AtlasLifecycle, texture) == 428);
static_assert(offsetof(AtlasLifecycle, references) == 432);
}
