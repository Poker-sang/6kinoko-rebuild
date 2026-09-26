#include "kinoko/map_collision.h"
#include "kinoko/map_query.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/map_render.h"
#include "kinoko/actor_collision.h"
#include "kinoko/act_runtime.h"
#include "kinoko/native_buffer.h"
#include "kinoko/collision_records.hpp"
#include <climits>

namespace {
using namespace kinoko::map;
using namespace kinoko::collision;
static_assert(sizeof(HitBuffer) == 12 && sizeof(KinokoCollisionRecord) == 12);
}

extern "C" int32_t kinoko_map_collision_append(KinokoCollisionState *state,
    int32_t *count, const unsigned char *chip, const void *placement, int32_t index) {
    const uint32_t required = static_cast<uint32_t>(*count) + 1;
    if (required > INT32_MAX / sizeof(KinokoCollisionRecord)) return 0;
    auto hits = StateView(state).view(&StateRecord::hits);
    // The sole integer-pointer boundary is the existing native_buffer ABI.
    if (!kinoko_native_buffer_ensure((void*)(uintptr_t)(kinoko::legacy::address(hits.data())), required * sizeof(KinokoCollisionRecord))) return 0;
    auto *records = hits.get(&HitBuffer::begin);
    records[*count] = {chip, placement, index};
    ++*count;
    // Preserve the legacy signed Win32 address comparison and high-water end.
    if (kinoko::legacy::address(hits.get(&HitBuffer::end)) <
        kinoko::legacy::address(records + *count))
        hits.set(&HitBuffer::end, records + *count);
    return 1;
}

// Original 435220, with iterator setup/steps 4361C0/436290/4362F0.
// Keep R137's internal caller contract, including empty-map success and chip
// lookup fallback. This migration does not expand the public HRESULT interface.
extern "C" int32_t kinoko_map_collision_query(KinokoCollisionState *state,
    KinokoActLayout *layout, int32_t *cached, int32_t left, int32_t top,
    int32_t right, int32_t bottom, int32_t *count) {
    return kinoko::map::query_visible(layout, cached, left, top, right, bottom,
        [&](kinoko_mcd_chip *chip, Placement *record, int32_t index) {
            return kinoko_map_collision_append(state, count, chip->bytes, record, index);
        });
}
