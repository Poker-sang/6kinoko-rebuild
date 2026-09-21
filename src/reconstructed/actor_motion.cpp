#include "kinoko/collision_lifetime.hpp"
#include "kinoko/actor_records.hpp"
#include <cmath>

namespace {
using namespace kinoko::actor;
using namespace kinoko::collision;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
void save_previous(const ActorView &actor) {
    actor.set(&ActorRecord::previous_x, actor.get(&ActorRecord::x));
    actor.set(&ActorRecord::previous_y, actor.get(&ActorRecord::y));
    actor.set(&ActorRecord::previous_bounds, actor.get(&ActorRecord::world_bounds));
}
bool has_collision_bounds(const ActorView &actor) {
    auto *animation = pointer(actor.get(&ActorRecord::animation));
    return actor.get(&ActorRecord::collision_mask) && animation &&
        kinoko::native::RecordView<AnimationRecord>(animation).get(&AnimationRecord::has_bounds);
}
float take_step(float &remaining) {
    if (remaining > 8.0f) { remaining -= 8.0f; return 8.0f; }
    if (remaining < -8.0f) { remaining += 8.0f; return -8.0f; }
    const float step = remaining;
    remaining = 0.0f;
    return step;
}
}

extern "C" void kinoko_actor_refresh_collision_bounds(KinokoActor *receiver) {
    const ActorView actor(receiver);
    const float width_scale = actor.get(&ActorRecord::scale) * actor.get(&ActorRecord::scale_x);
    const float height_scale = actor.get(&ActorRecord::scale) * actor.get(&ActorRecord::scale_y);
    const auto local = actor.get(&ActorRecord::local_bounds);
    const float x = actor.get(&ActorRecord::x), y = actor.get(&ActorRecord::y);
    Bounds bounds;
    if (0.0f >= actor.get(&ActorRecord::direction)) {
        bounds.left = local.left * width_scale + x;
        bounds.right = local.right * width_scale + x;
    } else {
        bounds.left = x - local.right * width_scale;
        bounds.right = x - local.left * width_scale;
    }
    bounds.top = local.top * height_scale + y;
    bounds.bottom = local.bottom * height_scale + y;
    actor.set(&ActorRecord::world_bounds, bounds);
    actor.set(&ActorRecord::bounds_anchor_x, bounds.left);
    actor.set(&ActorRecord::bounds_anchor_y, bounds.top);
    actor.view(&ActorRecord::initial).set(&InitialData::width, static_cast<int16_t>(bounds.right - bounds.left));
    actor.view(&ActorRecord::initial).set(&InitialData::height, static_cast<int16_t>(bounds.bottom - bounds.top));
}

extern "C" int32_t kinoko_actor_move(KinokoCollisionState *state, KinokoActor *receiver, float dx, float dy) {
    const ActorView actor(receiver);
    while (dx != 0.0f || dy != 0.0f) {
        save_previous(actor);
        if (has_collision_bounds(actor)) {
            const float step_x = take_step(dx), step_y = take_step(dy);
            kinoko_collision_move_actor(state, receiver, step_x, step_y);
        } else {
            actor.set(&ActorRecord::x, actor.get(&ActorRecord::x) + dx);
            actor.set(&ActorRecord::y, actor.get(&ActorRecord::y) + dy);
            actor.set(&ActorRecord::hits, std::array<int32_t, 4>{});
            dx = dy = 0.0f;
        }
        kinoko_actor_refresh_collision_bounds(receiver);
    }
    return 0;
}

extern "C" int32_t kinoko_actor_update_motion(KinokoCollisionState *state, KinokoActor *receiver) {
    if (!receiver) return 0;
    const ActorView actor(receiver);
    kinoko_actor_trace_motion(receiver, 0);
    save_previous(actor);
    {
        // Retain the parent through movement, diagnostics and possible detach,
        // just as 45EC60 keeps the temporary locked pair to the end of Update.
        const LockedParent locked(reinterpret_cast<ActorReference *>(actor.bytes(&ActorRecord::step)));
        auto *parent = locked.actor();
        float parent_dx = 0.0f, parent_dy = 0.0f;
        if (parent) {
            const ActorView support(parent);
            if (support.get(&ActorRecord::update_group) & kinoko_actor_motion_update_mask()) {
                parent_dx = support.get(&ActorRecord::x) - support.get(&ActorRecord::previous_x);
                parent_dy = support.get(&ActorRecord::y) - support.get(&ActorRecord::previous_y);
            }
        }
        actor.set(&ActorRecord::parent_velocity_x, parent_dx);
        actor.set(&ActorRecord::parent_velocity_y, parent_dy);
        if (has_collision_bounds(actor)) {
            const float vx = actor.get(&ActorRecord::velocity_x);
            const float dx = vx + parent_dx;
            float dy = actor.get(&ActorRecord::velocity_y) + parent_dy;
            const uint32_t chip_flags = static_cast<uint32_t>(actor.view(&ActorRecord::initial).get(&InitialData::chip_flags));
            if (!((chip_flags | actor.get(&ActorRecord::flags)) & 0x800000u) &&
                actor.get(&ActorRecord::hits)[3] != 0 && vx != 0) {
                const float slope_dy = std::fabs(actor.get(&ActorRecord::pitch) * vx);
                dy += std::fmin(std::fabs(vx), slope_dy);
            }
            kinoko_collision_move_actor(state, receiver, dx, dy);
        } else {
            actor.set(&ActorRecord::x, actor.get(&ActorRecord::x) + (actor.get(&ActorRecord::velocity_x) + parent_dx));
            actor.set(&ActorRecord::y, actor.get(&ActorRecord::y) + (actor.get(&ActorRecord::velocity_y) + parent_dy));
            actor.set(&ActorRecord::hits, std::array<int32_t, 4>{});
        }
        kinoko_actor_refresh_collision_bounds(receiver);
        kinoko_actor_trace_motion(receiver, 1);
        if (parent) {
            const auto first = ActorView(parent).get(&ActorRecord::world_bounds);
            const auto second = actor.get(&ActorRecord::world_bounds);
            const bool touching = second.right >= first.left && second.bottom >= first.top &&
                second.left <= first.right && second.top <= first.bottom;
            if (!touching) kinoko_actor_set_collision_parent(receiver, nullptr);
        }
    }
    kinoko_actor_trace_motion(receiver, 2);
    return 0;
}
