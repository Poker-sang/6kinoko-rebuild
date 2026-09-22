#include "kinoko/map_manager_records.hpp"
#include "kinoko/collision_records.hpp"
#include "kinoko/actor_records.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/map_containers.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/native_buffer.h"

namespace {
using namespace kinoko::collision;
using namespace kinoko::actor;
using namespace kinoko::map;
using kinoko::legacy::address;
int32_t *scan_cache(const ActorView &actor, int32_t index) {
    return reinterpret_cast<int32_t *>(actor.bytes(&ActorRecord::collision_scan_cache) + index * sizeof(int32_t));
}
class PointHits final {
    std::array<int32_t, 12> storage_{};
public:
    KinokoCollisionState *state() { return reinterpret_cast<KinokoCollisionState *>(storage_.data()); }
    auto hits() { return StateView(state()).view(&StateRecord::hits); }
    ~PointHits() { kinoko_native_buffer_destroy(address(hits().data())); }
};
}

extern "C" uint32_t kinoko_collision_chip_flags(KinokoCollisionState *state, KinokoActor *actor) {
    const StateView collision(state);
    const ActorView receiver(actor);
    const auto layouts = collision.get(&StateRecord::layouts);
    int32_t count = 0, index = 0;
    for (auto *entry = layouts.begin; entry != layouts.end; ++entry, ++index) {
        auto *layer = LayoutView(*entry).get(&LayoutRecord::owning_layer);
        if (LayerView(layer).get(&LayerRecord::visible)) {
            const auto bounds = receiver.get(&ActorRecord::world_bounds);
            if (!kinoko_map_collision_query(state, *entry, scan_cache(receiver, index),
                static_cast<int32_t>(bounds.left), static_cast<int32_t>(bounds.top),
                static_cast<int32_t>(bounds.right), static_cast<int32_t>(bounds.bottom), &count)) return 0;
        }
    }
    uint32_t flags = 0;
    auto *records = collision.get(&StateRecord::hits).begin;
    for (int32_t i = 0; i < count; ++i)
        if (records[i].chip)
            flags |= ChipView(const_cast<unsigned char *>(records[i].chip)).get(&ChipDefinition::flags);
    return flags;
}

extern "C" int32_t kinoko_collision_has_chip(KinokoCollisionState *state, KinokoActor *actor,
    float left, float top, float right, float bottom) {
    const StateView collision(state);
    const ActorView receiver(actor);
    const auto layouts = collision.get(&StateRecord::layouts);
    auto *actors = collision.get(&StateRecord::actors).begin;
    const int32_t actor_count = collision.get(&StateRecord::actor_count);
    int32_t count = 0, index = 0;
    for (auto *entry = layouts.begin; entry != layouts.end; ++entry, ++index) {
        auto *layer = LayoutView(*entry).get(&LayoutRecord::owning_layer);
        if (LayerView(layer).get(&LayerRecord::visible)) {
            if (!kinoko_map_collision_query(state, *entry, scan_cache(receiver, index),
                static_cast<int32_t>(left), static_cast<int32_t>(top),
                static_cast<int32_t>(right), static_cast<int32_t>(bottom), &count)) return 0;
            if (count > 0) return 1;
        }
    }
    // Includes touching edges, excludes only the receiver; no mask filtering.
    for (int32_t i = 0; i < actor_count; ++i) {
        const auto bounds = ActorView(actors[i]).get(&ActorRecord::world_bounds);
        if (bounds.right >= left && bounds.left <= right && bounds.bottom >= top &&
            bounds.top <= bottom && actors[i] != actor) return 1;
    }
    return 0;
}

extern "C" int32_t kinoko_collision_event_at_point(KinokoMapManager *manager,
    int32_t x, int32_t y, uint32_t layer_index, int32_t *output_count) {
    const uint32_t layer_count = kinoko_map_event_count(address(manager));
    const kinoko::map::ManagerView result(manager);
    result.set(&ManagerRecord::last_id, int32_t{-1});
    if (layer_index >= layer_count) return 0;
    auto *layout = kinoko::legacy::pointer<KinokoActLayout>(kinoko_map_event_at(address(manager), layer_index));
    if (!layout) return 0;
    const LayoutView map(layout);
    const int32_t width = map.get(&LayoutRecord::max_chip_width);
    const int32_t height = map.get(&LayoutRecord::max_chip_height);
    PointHits scratch;
    int32_t cached = 0, count = 0;
    if (!kinoko_map_collision_query(scratch.state(), layout, &cached,
        x - width, y - height, x + width, y + height, &count)) return 0;
    if (output_count) *output_count = count;
    auto *records = scratch.hits().get(&HitBuffer::begin);
    for (int32_t i = 0; i < count; ++i) {
        const PlacementView placement(const_cast<void *>(records[i].layout));
        const ChipView chip(const_cast<unsigned char *>(records[i].chip));
        const int32_t left = placement.get(&Placement::left), top = placement.get(&Placement::top);
        const int32_t right = left + chip.get(&ChipDefinition::width);
        const int32_t bottom = top + chip.get(&ChipDefinition::height);
        // Point query uses local integer placement coordinates and half-open edges.
        if (left <= x && right > x && top <= y && bottom > y) {
            result.set(&ManagerRecord::last_id, static_cast<int32_t>(placement.get(&Placement::chip_id)));
            result.set(&ManagerRecord::last_bounds, Bounds{static_cast<float>(left), static_cast<float>(top),
                static_cast<float>(right), static_cast<float>(bottom)});
            break;
        }
    }
    return 0;
}
