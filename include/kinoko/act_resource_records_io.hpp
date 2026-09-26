#pragma once
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

#include "kinoko/boost_control.hpp"

namespace kinoko::act {
// Both resource variants occupy 100 bytes in the original Win32 ACT factory.
// String fields own their native storage; MCD data is transferred on success.
struct ChipResourceRecord {
    const void *methods;
    int32_t id;
    legacy::StringRecord name;
    uint32_t unknown32;
    legacy::StringRecord source_name;
    uint32_t unknown60;
    kinoko_mcd_data *data;
    kinoko::native::upstream::CountedControl *shared_data; // one strong reference
    legacy::StringRecord loaded_path;
    uint32_t unknown96;
};
struct TextureResourceRecord {
    const void *methods;
    int32_t id;
    legacy::StringRecord name;
    uint32_t unknown32;
    uint8_t borrows_texture;
    std::array<uint8_t, 3> padding37;
    legacy::StringRecord texture_name;
    uint32_t unknown64;
    int32_t texture;
    int32_t width, height;
    float source_x, source_y, source_width, source_height;
    uint8_t auto_size;
    std::array<uint8_t, 3> padding97;
};
using ChipResourceFields = kinoko::native::RecordView<ChipResourceRecord>;
using TextureResourceFields = kinoko::native::RecordView<TextureResourceRecord>;
static_assert(sizeof(ChipResourceRecord) == 100 && sizeof(TextureResourceRecord) == 100);
static_assert(offsetof(ChipResourceRecord, source_name) == 36);
static_assert(offsetof(ChipResourceRecord, data) == 64);
static_assert(offsetof(ChipResourceRecord, shared_data) == 68);
static_assert(offsetof(ChipResourceRecord, loaded_path) == 72);
static_assert(offsetof(TextureResourceRecord, texture_name) == 40);
static_assert(offsetof(TextureResourceRecord, texture) == 68);
static_assert(offsetof(TextureResourceRecord, source_x) == 80);
static_assert(offsetof(TextureResourceRecord, auto_size) == 96);
}
