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
// Factory prefix and tail of C2DMapLayout, with gaps kept opaque until
// their semantics are recovered from the original vtable users.
struct MapLayoutRecord {
    const void *methods, *view_methods;
    std::array<uint8_t, 232> unknown8;
    int32_t x_limit, y_limit;
    std::array<uint8_t, 72> unknown248;
    float alpha, secondary_alpha;
    int32_t blend;
    std::array<uint8_t, 120> unknown332;
    int32_t resource_id;
    std::array<uint8_t, 8> unknown456;
};
using MapLayoutView = kinoko::native::RecordView<MapLayoutRecord>;
static_assert(sizeof(MapLayoutRecord) == 464);
static_assert(offsetof(MapLayoutRecord, x_limit) == 240);
static_assert(offsetof(MapLayoutRecord, alpha) == 320);
static_assert(offsetof(MapLayoutRecord, resource_id) == 452);
using MapCellView = kinoko::native::RecordView<MapCellRecord>;
static_assert(sizeof(MapCellRecord) == 32);
static_assert(offsetof(MapCellRecord, index) == 20);
static_assert(offsetof(MapCellRecord, enabled) == 24);
static_assert(offsetof(MapCellRecord, opacity) == 28);
}
