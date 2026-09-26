#include "kinoko/collision_records.hpp"
#include "kinoko/collision_lifetime.hpp"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_methods.h"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/native_buffer.h"
#include "kinoko/native_control.h"
#include <climits>
#include <cstring>

extern "C" void kinoko_trace_i32(const char *, int32_t);

namespace {
using namespace kinoko::collision;
using namespace kinoko::actor;
using namespace kinoko::map;
using kinoko::legacy::address;
using kinoko::legacy::pointer;

template<class T> int32_t buffer_count(const Buffer<T>& buffer) {
    const uint32_t bytes = static_cast<uint32_t>(address(buffer.end)) -
        static_cast<uint32_t>(address(buffer.begin));
    return kinoko::legacy::load<int32_t>(&bytes) / static_cast<int32_t>(sizeof(T));
}
template<class T> bool reserve(kinoko::native::RecordView<Buffer<T>> buffer, uint32_t count) {
    return count <= INT32_MAX / sizeof(T) &&
        kinoko_native_buffer_ensure((void*)(uintptr_t)(address(buffer.data())), count * sizeof(T));
}


template<class T> uint32_t guarded_buffer_count(const Buffer<T>& buffer) {
    // Preserve the old signed Win32 address guard at this legacy span boundary.
    const int32_t begin = address(buffer.begin), end = address(buffer.end);
    return end >= begin ? (static_cast<uint32_t>(end) - static_cast<uint32_t>(begin)) / sizeof(T) : 0;
}
}

extern "C" void *kinoko_collision_refresh(KinokoCollisionState *state) {
    if (!state) return nullptr;
    const StateView collision(state);
    auto *parents = collision.get(&StateRecord::map_parents).begin;
    if (parents) {
        const uint32_t count = guarded_buffer_count(collision.get(&StateRecord::layouts));
        for (uint32_t index = 0; index < count; ++index) {
            const LockedParent locked(parents + index);
            auto *actor = locked.actor();
            // Expired weak references must not touch the borrowed layout: its
            // storage may already have been retired with the owning document.
            if (!actor) continue;
            const ActorView proxy(actor);
            proxy.set(&ActorRecord::previous_x, proxy.get(&ActorRecord::x));
            proxy.set(&ActorRecord::previous_y, proxy.get(&ActorRecord::y));
            auto *layouts = collision.get(&StateRecord::layouts).begin;
            if (!layouts || !layouts[index]) continue;
            auto *layer = LayoutView(layouts[index]).get(&LayoutRecord::owning_layer);
            if (!layer) continue;
            const LayerView config(layer);
            proxy.set(&ActorRecord::x, config.get(&LayerRecord::position_x));
            proxy.set(&ActorRecord::y, config.get(&LayerRecord::position_y));
        }
    }
    auto *manager = collision.get(&StateRecord::manager);
    if (!manager) {
        collision.set(&StateRecord::actor_count, int32_t{0});
        return nullptr;
    }
    const ManagerView owner(manager);
    const uint32_t count = static_cast<uint32_t>(owner.get(&ManagerPrefix::iteration_count));
    collision.set(&StateRecord::actor_count, int32_t{0});
    if (!count) return manager;
    const auto candidates = collision.view(&StateRecord::actors);
    if (guarded_buffer_count(candidates.load()) < count) {
        if (count > INT32_MAX / sizeof(KinokoActor *) ||
            !kinoko_native_buffer_resize((void*)(uintptr_t)(address(candidates.data())), count * sizeof(KinokoActor *)))
            return manager;
    }
    auto *output = candidates.get(&Buffer<KinokoActor *>::begin);
    auto *items = owner.get(&ManagerPrefix::iteration).begin;
    if (!items) return manager;
    uint32_t written = 0;
    void *last = manager;
    for (uint32_t index = 0; index < count; ++index) {
        auto *actor = items[index];
        if (actor && ActorView(actor).get(&ActorRecord::collision_group)) {
            output[written++] = actor;
            last = actor;
        }
    }
    collision.set(&StateRecord::actor_count, static_cast<int32_t>(written));
    return last;
}

extern "C" int32_t kinoko_collision_reset(KinokoCollisionState *state, KinokoActorManager *manager) {
    if (!state || !manager) return 0;
    const StateView collision(state);
    const auto layouts = collision.view(&StateRecord::layouts);
    const auto layout_begin = layouts.get(&Buffer<KinokoActLayout *>::begin);
    if (layout_begin != layouts.get(&Buffer<KinokoActLayout *>::end))
        layouts.set(&Buffer<KinokoActLayout *>::end, layout_begin);
    const auto parents = collision.view(&StateRecord::map_parents);
    auto *begin = parents.get(&Buffer<ActorReference>::begin);
    auto *end = parents.get(&Buffer<ActorReference>::end);
    for (auto *cursor = begin; cursor != end; ++cursor)
        kinoko_native_release_weak((void*)(uintptr_t)(address(ReferenceView(cursor).get(&ActorReference::control))));
    parents.set(&Buffer<ActorReference>::end, begin);
    collision.set(&StateRecord::manager, manager);
    collision.set(&StateRecord::actor_count, int32_t{0});
    return 1;
}

extern "C" KinokoActor *kinoko_collision_register_map(KinokoCollisionState *state,
    KinokoActLayout *layout) {
    if (!layout) return nullptr;
    const StateView collision(state);
    const int32_t count = buffer_count(collision.get(&StateRecord::layouts));
    const auto layouts = collision.view(&StateRecord::layouts);
    const auto parents = collision.view(&StateRecord::map_parents);
    const auto ends = collision.view(&StateRecord::layer_hit_ends);
    const uint32_t required = static_cast<uint32_t>(count) + 1;
    if (!reserve(layouts, required) || !reserve(parents, required) || !reserve(ends, required))
        return nullptr;
    for (int32_t index = 0; auto *record = placement_at(layout, index); ++index) {
        const PlacementView placement(record);
        placement.set(&Placement::fractional_left, static_cast<float>(placement.get(&Placement::left)));
        placement.set(&Placement::fractional_top, static_cast<float>(placement.get(&Placement::top)));
    }
    auto *actor = kinoko_actor_create_collision_proxy(collision.get(&StateRecord::manager));
    if (!actor) return nullptr;
    const ActorView proxy(actor);
    proxy.set(&ActorRecord::world_bounds, Bounds{-65535.0f, -65535.0f, 65535.0f, 65535.0f});
    proxy.set(&ActorRecord::update_group, uint32_t{0x80000000u});
    proxy.set(&ActorRecord::active, uint8_t{0});
    proxy.set(&ActorRecord::registration_flag20, uint8_t{1});
    kinoko_actor_reset_priority(actor, -1);

    // 4684A0/468580: move existing entries without changing their weak counts.
    auto *layout_records = layouts.get(&Buffer<KinokoActLayout *>::begin);
    auto *parent_records = parents.get(&Buffer<ActorReference>::begin);
    std::memmove(layout_records + 1, layout_records, count * sizeof(*layout_records));
    std::memmove(parent_records + 1, parent_records, count * sizeof(*parent_records));
    layout_records[0] = layout;
    parent_records[0] = ReferenceView(proxy.bytes(&ActorRecord::owner)).load();
    kinoko_native_add_weak((void*)(uintptr_t)(address(parent_records[0].control)));
    layouts.set(&Buffer<KinokoActLayout *>::end, layout_records + required);
    parents.set(&Buffer<ActorReference>::end, parent_records + required);
    auto *hit_ends = ends.get(&Buffer<int32_t>::begin);
    hit_ends[count] = 0;
    ends.set(&Buffer<int32_t>::end, hit_ends + required);
    kinoko_trace_i32("actor:collision-layout-count", count + 1);
    return actor;
}

extern "C" int32_t kinoko_collision_query_actor_map(KinokoCollisionState *state,
    KinokoActLayout *layout, KinokoActor *actor, int32_t layer_index, int32_t *count) {
    const ActorView view(actor);
    const auto bounds = view.get(&ActorRecord::world_bounds);
    // Original per-layer cache starts at 480. Retain the caller's layer-index
    // contract; do not add an invented cap to the legacy address calculation.
    auto *cached = reinterpret_cast<int32_t *>(view.bytes(&ActorRecord::collision_scan_cache) +
        sizeof(int32_t) * layer_index);
    return kinoko_map_collision_query(state, layout, cached,
        static_cast<int32_t>(bounds.left - 24.0f), static_cast<int32_t>(bounds.top - 24.0f),
        static_cast<int32_t>(bounds.right + 24.0f), static_cast<int32_t>(bounds.bottom + 24.0f), count);
}

extern "C" int32_t kinoko_collision_move_actor(KinokoCollisionState *state,
    KinokoActor *actor, float dx, float dy) {
    const StateView collision(state);
    const ActorView moving(actor);
    int32_t count = 0;
    const int32_t layer_count = buffer_count(collision.get(&StateRecord::layouts));
    if (moving.get(&ActorRecord::collision_mask) & 1) {
        for (int32_t index = 0; index < layer_count; ++index) {
            auto *layout = collision.get(&StateRecord::layouts).begin[index];
            auto *layer = LayoutView(layout).get(&LayoutRecord::owning_layer);
            if (!layer || !LayerView(layer).get(&LayerRecord::visible)) continue;
            auto *parent = collision.get(&StateRecord::map_parents).begin + index;
            if (!kinoko_collision_query_actor_map(state, layout, actor, index, &count)) return 0;
            collision.get(&StateRecord::layer_hit_ends).begin[index] = count;
            const auto step = ReferenceView(moving.bytes(&ActorRecord::step)).load();
            if (step.slot == parent->slot && step.control)
                kinoko_actor_set_collision_parent(actor, nullptr);
        }
    }
    for (int32_t index = 0; index < collision.get(&StateRecord::actor_count); ++index) {
        auto *other = collision.get(&StateRecord::actors).begin[index];
        const ActorView candidate(other);
        if (other == actor || !(candidate.get(&ActorRecord::collision_group) &
                               moving.get(&ActorRecord::collision_mask))) continue;
        const auto other_bounds = candidate.get(&ActorRecord::world_bounds);
        const auto bounds = moving.get(&ActorRecord::world_bounds);
        if (other_bounds.right + 24 >= bounds.left && other_bounds.left - 24 <= bounds.right) {
            // The Actor embeds the same three-word borrowed collision record.
            const auto record = kinoko::native::RecordView<KinokoCollisionRecord>(
                candidate.bytes(&ActorRecord::collision_chip)).load();
            if (!kinoko_map_collision_append(state, &count, record.chip, record.layout, record.index)) return 0;
        }
    }
    const int32_t support = kinoko_actor_collision_move(actor,
        collision.get(&StateRecord::hits).begin, count, dx, dy);
    if (support >= 0 && (moving.get(&ActorRecord::collision_mask) & 1)) {
        for (int32_t index = 0; index < layer_count; ++index) {
            if (support < collision.get(&StateRecord::layer_hit_ends).begin[index]) {
                const LockedParent parent(collision.get(&StateRecord::map_parents).begin + index);
                if (parent.has_slot()) {
                    kinoko_actor_set_collision_parent(actor, parent.actor());
                    break;
                }
            }
        }
    }
    return 1;
}
