#include "kinoko/actor_animation.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/animation_storage.h"
#include "kinoko/legacy_memory.hpp"


namespace {
using namespace kinoko::actor;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::native::RecordView;

int32_t frame_count(const AnimationRecord& animation) {
    return static_cast<int32_t>(static_cast<uint32_t>(address(animation.frames_end)) -
        static_cast<uint32_t>(address(animation.frames_begin))) /
        static_cast<int32_t>(sizeof(FrameRecord));
}
int32_t increment(int32_t value) {
    return static_cast<int32_t>(static_cast<uint32_t>(value) + 1u);
}
AnimationRecord animation_at(KinokoAnimation *value) {
    return RecordView<AnimationRecord>(value).load();
}
KinokoAnimationFrame *select_frame(const ActorView& actor, const AnimationRecord& animation,
                     int32_t index) {
    // The original SyncAnimation only upper-clamps and can publish a negative
    // index. Keep that Win32 address arithmetic at this explicit ABI boundary.
    auto *frame=pointer<KinokoAnimationFrame>(static_cast<int32_t>(
        static_cast<uint32_t>(address(animation.frames_begin)) +
        static_cast<uint32_t>(index)*static_cast<uint32_t>(sizeof(FrameRecord))));
    actor.set(&ActorRecord::current_frame, frame);
    actor.set(&ActorRecord::sprite_frame, frame);
    return frame;
}
void update_bounds(const ActorView& actor, const AnimationRecord& animation) {
    const auto x = actor.get(&ActorRecord::x);
    const auto y = actor.get(&ActorRecord::y);
    const auto scale = actor.get(&ActorRecord::scale);
    const auto scale_x = actor.get(&ActorRecord::scale_x);
    const auto scale_y = actor.get(&ActorRecord::scale_y);
    Bounds local{};
    if (animation.has_bounds) {
        local = {
            static_cast<float>(static_cast<double>(animation.left) - 0.5),
            static_cast<float>(animation.top),
            static_cast<float>(static_cast<double>(animation.right) + 0.5),
            static_cast<float>(static_cast<double>(animation.bottom) + 1.0)};
        actor.set(&ActorRecord::bounds_anchor_x,
            static_cast<float>(static_cast<double>(animation.left) + x));
        actor.set(&ActorRecord::bounds_anchor_y,
            static_cast<float>(static_cast<double>(animation.top) + y));
    } else {
        actor.set(&ActorRecord::bounds_anchor_x, 0.0f);
        actor.set(&ActorRecord::bounds_anchor_y, 0.0f);
        const auto initial = actor.view(&ActorRecord::initial);
        initial.set(&InitialData::width, int16_t{0});
        initial.set(&InitialData::height, int16_t{0});
    }
    actor.set(&ActorRecord::local_bounds, local);
    // Keep original double intermediates, operation order and half-pixel rules.
    Bounds world{};
    if (actor.get(&ActorRecord::direction) <= 0) {
        world.left = static_cast<float>(static_cast<double>(local.left) * scale * scale_x + x);
        world.right = static_cast<float>(static_cast<double>(local.right) * scale * scale_x + x);
    } else {
        world.left = static_cast<float>(x - static_cast<double>(local.right) * scale * scale_x);
        world.right = static_cast<float>(x - static_cast<double>(local.left) * scale * scale_x);
    }
    world.top = static_cast<float>(static_cast<double>(local.top) * scale * scale_y + y);
    world.bottom = static_cast<float>(static_cast<double>(local.bottom) * scale * scale_y + y);
    actor.set(&ActorRecord::world_bounds, world);
    const auto initial = actor.view(&ActorRecord::initial);
    initial.set(&InitialData::width, static_cast<int16_t>(static_cast<int32_t>(world.right - world.left)));
    initial.set(&InitialData::height, static_cast<int16_t>(static_cast<int32_t>(world.bottom - world.top)));
}
}

// 462280: SetTake updates state before lookup, including failed lookups.
extern "C" int32_t kinoko_actor_set_take(int32_t value, int32_t take) {
    const ActorView actor(pointer(value));
    const ManagerView manager(pointer(static_cast<int32_t>(actor.get(&ActorRecord::manager))));
    actor.set(&ActorRecord::take, take);
    actor.set(&ActorRecord::frame_index, int32_t{0});
    actor.set(&ActorRecord::frame_time, int32_t{0});
    const auto lookup = manager.view(&ManagerPrefix::animation_lookup);
    auto *selected=kinoko_animation_find(reinterpret_cast<KinokoActorManager *>(manager.data()),take);
    if (!selected) return static_cast<int32_t>(lookup.get(&TreeIndex::head));
    actor.set(&ActorRecord::animation, selected);
    const auto animation = animation_at(selected);
    actor.set(&ActorRecord::take_duration, animation.duration_total);
    update_bounds(actor, animation);
    return address(select_frame(actor, animation, 0));
}
extern "C" int32_t __fastcall kinoko_actor_set_take_method(int32_t actor, void*, int32_t take) {
    return kinoko_actor_set_take(actor, take);
}

// A changed take defers ticking until the next update, as in 45E120.
extern "C" void kinoko_actor_advance_animation(int32_t value, int32_t take_before_callback) {
    const ActorView actor(pointer(value));
    const auto current = actor.get(&ActorRecord::current_frame);
    if (!current || actor.get(&ActorRecord::take) != take_before_callback) return;
    const auto time = increment(actor.get(&ActorRecord::frame_time));
    actor.set(&ActorRecord::frame_time, time);
    const RecordView<FrameRecord> frame(current);
    const auto animation_address = actor.get(&ActorRecord::animation);
    if (time < frame.get(&FrameRecord::duration) || !animation_address) return;
    const auto animation = animation_at(animation_address);
    const auto count = frame_count(animation);
    if (count <= 0) return;
    const auto next = increment(actor.get(&ActorRecord::frame_index));
    actor.set(&ActorRecord::frame_index, next);
    if (next == count) {
        if (animation.loops) {
            actor.set(&ActorRecord::frame_index, int32_t{0});
        } else {
            actor.set(&ActorRecord::frame_index, next - 1);
            actor.set(&ActorRecord::frame_time, int32_t{0});
            return;
        }
    }
    select_frame(actor, animation, actor.get(&ActorRecord::frame_index));
    actor.set(&ActorRecord::frame_time, int32_t{0});
}

// 45FE80 has only an upper-bound clamp: do not invent a lower-bound policy.
extern "C" void kinoko_actor_sync_animation_state(int32_t value, int32_t source_value) {
    const ActorView actor(pointer(value)), source(pointer(source_value));
    auto frame = source.get(&ActorRecord::frame_index);
    actor.set(&ActorRecord::frame_time, source.get(&ActorRecord::frame_time));
    actor.set(&ActorRecord::frame_index, frame);
    const auto animation = animation_at(actor.get(&ActorRecord::animation));
    const auto count = frame_count(animation);
    if (frame >= count) {
        frame = count - 1;
        if (frame < 0) frame = 0;
        actor.set(&ActorRecord::frame_index, frame);
    }
    select_frame(actor, animation, frame);
}
