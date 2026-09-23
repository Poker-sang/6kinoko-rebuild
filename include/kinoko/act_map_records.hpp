#pragma once
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace kinoko::act {
// 32-byte map entry read from the archive. Its first 20 bytes belong to the
// serialized tile; the loader assigns runtime index, enabled and opacity.
struct MapCellRecord {
    std::array<uint8_t, 20> serialized;
    uint32_t index;
    uint8_t enabled;
    std::array<uint8_t, 3> padding;
    float opacity;
};
using MapCellView = kinoko::native::RecordView<MapCellRecord>;
static_assert(sizeof(MapCellRecord) == 32);
static_assert(offsetof(MapCellRecord, index) == 20);
static_assert(offsetof(MapCellRecord, enabled) == 24);
static_assert(offsetof(MapCellRecord, opacity) == 28);
}
