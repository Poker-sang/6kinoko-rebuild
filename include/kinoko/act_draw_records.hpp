#pragma once
#include "kinoko/act_resource_records.hpp"
#include "kinoko/sprite.h"

namespace kinoko::act {
struct BlitCommand {
    int32_t blend;
    float alpha, x, y;
    int32_t source_x, source_y, width, height, texture;
};
struct BlitSprite { BlitCommand command; KinokoSprite sprite; };
struct TextureResourcePrefix {
    Address vtable;
    std::array<uint8_t, 64> unknown4;
    int32_t texture;
};
static_assert(sizeof(BlitCommand) == 36 && sizeof(BlitSprite) == 184);
static_assert(offsetof(BlitCommand, texture) == 32 && offsetof(BlitSprite, sprite) == 36);
static_assert(offsetof(TextureResourcePrefix, texture) == 68);
}
