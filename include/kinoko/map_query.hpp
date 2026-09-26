#pragma once
#include "kinoko/map_layout_records.hpp"
#include "kinoko/map_render.h"
#include "kinoko/act_runtime.h"
namespace kinoko::map {
// Original 435220: forward scan from cached index, then backward scan.
// Consumers own their output; chip and placement references remain borrowed.
template<class Emit>
int query_visible(KinokoActLayout *layout, int32_t *cached,
    int32_t left, int32_t top, int32_t right, int32_t bottom, Emit emit) {
    auto *data = kinoko_map_query_chip_data(layout);
    const LayoutView map(layout);
    auto *layer = map.get(&LayoutRecord::owning_layer);
    const int32_t total = placement_count(layout);
    const int32_t start = *cached;
    bool forward = true, backward = false;
    const int32_t max_width = map.get(&LayoutRecord::max_chip_width);
    const int32_t max_height = map.get(&LayoutRecord::max_chip_height);
    if (!data || !layer) return 0;
    if (total <= 0) return 1;
    const LayerView owner(layer);
    const float offset_x = owner.get(&LayerRecord::position_x);
    const float offset_y = owner.get(&LayerRecord::position_y);
    const int32_t offset_ix = static_cast<int32_t>(offset_x);
    const int32_t offset_iy = static_cast<int32_t>(offset_y);
    const int64_t minimum_x = static_cast<int64_t>(left) - max_width;
    const int64_t minimum_y = static_cast<int64_t>(top) - max_height;
    if (start > 0 && start < total) {
        const int32_t x = PlacementView(placement_at(layout, start)).get(&Placement::left) + offset_ix;
        forward = right >= x;
        backward = minimum_x <= x;
    }
    *cached = 0;
    for (int pass = 0; pass != 2; ++pass) {
        const int step = pass == 0 ? 1 : -1;
        int32_t index = pass == 0 ? start : start - forward;
        bool first = true;
        if ((pass == 0 && !forward) || (pass == 1 && !backward)) continue;
        for (; index >= 0 && index < total; index += step) {
            auto *record = placement_at(layout, index);
            const PlacementView placement(record);
            const int32_t record_x = placement.get(&Placement::left);
            const int32_t record_y = placement.get(&Placement::top);
            const int32_t x = record_x + offset_ix, y = record_y + offset_iy;
            if ((step > 0 && x > right) || (step < 0 && x < minimum_x)) break;
            if (x < minimum_x || x > right || y < minimum_y || y > bottom) continue;
            auto *chip = kinoko_mcd_find_chip(data, placement.get(&Placement::chip_id));
            if (!chip) continue;
            placement.set(&Placement::fractional_left, static_cast<float>(record_x) + offset_x);
            placement.set(&Placement::fractional_top, static_cast<float>(record_y) + offset_y);
            if (!emit(chip, record, index)) return 0;
            if (pass != 0 || first) *cached = index;
            first = false;
        }
    }
    return 1;
}
}
