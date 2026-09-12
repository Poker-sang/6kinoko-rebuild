#include "kinoko/actor_animation.h"

#include <cstddef>
#include <cstdint>

extern "C" int32_t function_4706c0_this(int32_t tree, int32_t *entry, int32_t *key);

namespace {
struct Bounds {
    float left, top, right, bottom;
};

struct Animation {
    uint8_t unknown_0[8];
    uint32_t frames_begin, frames_end;
    uint8_t unknown_16[8];
    uint8_t loops, has_bounds;
    uint8_t unknown_26[2];
    int32_t left, top, right, bottom, flags;
};

// Partial Actor view: retain every original offset without owning the memory.
struct ActorAnimationState {
    uint8_t unknown_0[148];
    int32_t manager;
    uint32_t sprite_frame;
    uint8_t unknown_156[12];
    float scale, scale_x, scale_y;
    uint8_t unknown_180[20];
    uint32_t animation, current_frame;
    int32_t take, frame_index, frame_time, animation_flags;
    uint8_t unknown_224[16];
    float x, y;
    uint8_t unknown_248[24];
    float direction;
    uint8_t unknown_276[76];
    float bounds_anchor_x, bounds_anchor_y;
    uint8_t unknown_360[28];
    int16_t width, height;
    uint8_t unknown_392[32];
    Bounds local_bounds, world_bounds;
};

static_assert(sizeof(void *) == 4);
static_assert(sizeof(Animation) == 48);
static_assert(offsetof(Animation, loops) == 24);
static_assert(offsetof(Animation, left) == 28);
static_assert(offsetof(Animation, flags) == 44);
static_assert(offsetof(ActorAnimationState, manager) == 148);
static_assert(offsetof(ActorAnimationState, sprite_frame) == 152);
static_assert(offsetof(ActorAnimationState, scale) == 168);
static_assert(offsetof(ActorAnimationState, animation) == 200);
static_assert(offsetof(ActorAnimationState, frame_time) == 216);
static_assert(offsetof(ActorAnimationState, x) == 240);
static_assert(offsetof(ActorAnimationState, direction) == 272);
static_assert(offsetof(ActorAnimationState, bounds_anchor_x) == 352);
static_assert(offsetof(ActorAnimationState, width) == 388);
static_assert(offsetof(ActorAnimationState, local_bounds) == 424);
static_assert(offsetof(ActorAnimationState, world_bounds) == 440);
static_assert(sizeof(ActorAnimationState) == 456);

template <typename T>
T &memory(uint32_t address) {
    return *reinterpret_cast<T *>(static_cast<uintptr_t>(address));
}

constexpr uint32_t frame_stride = 248;
constexpr uint32_t frame_duration_offset = 240;

int32_t frame_count(const Animation &animation) {
    return static_cast<int32_t>(animation.frames_end - animation.frames_begin) /
        static_cast<int32_t>(frame_stride);
}

int32_t increment(int32_t value) {
    return static_cast<int32_t>(static_cast<uint32_t>(value) + 1u);
}

uint32_t select_frame(ActorAnimationState &actor, const Animation &animation,
                      int32_t index) {
    const uint32_t frame = animation.frames_begin + static_cast<uint32_t>(index) * frame_stride;
    actor.current_frame = frame;
    actor.sprite_frame = frame;
    return frame;
}

void update_bounds(ActorAnimationState &actor, const Animation &animation) {
    if (animation.has_bounds) {
        actor.local_bounds = {
            static_cast<float>(static_cast<double>(animation.left) - 0.5),
            static_cast<float>(animation.top),
            static_cast<float>(static_cast<double>(animation.right) + 0.5),
            static_cast<float>(static_cast<double>(animation.bottom) + 1.0)};
        actor.bounds_anchor_x = static_cast<float>(static_cast<double>(animation.left) + actor.x);
        actor.bounds_anchor_y = static_cast<float>(static_cast<double>(animation.top) + actor.y);
    } else {
        actor.local_bounds = {};
        actor.bounds_anchor_x = 0;
        actor.bounds_anchor_y = 0;
        actor.width = 0;
        actor.height = 0;
    }

    // Keep intermediate double precision and original operation order before
    // storing each float, as in the existing reconstruction of the x87 path.
    const auto &local = actor.local_bounds;
    auto &world = actor.world_bounds;
    if (actor.direction <= 0) {
        world.left = static_cast<float>(static_cast<double>(local.left) * actor.scale * actor.scale_x + actor.x);
        world.right = static_cast<float>(static_cast<double>(local.right) * actor.scale * actor.scale_x + actor.x);
    } else {
        world.left = static_cast<float>(actor.x - static_cast<double>(local.right) * actor.scale * actor.scale_x);
        world.right = static_cast<float>(actor.x - static_cast<double>(local.left) * actor.scale * actor.scale_x);
    }
    world.top = static_cast<float>(static_cast<double>(local.top) * actor.scale * actor.scale_y + actor.y);
    world.bottom = static_cast<float>(static_cast<double>(local.bottom) * actor.scale * actor.scale_y + actor.y);
    actor.width = static_cast<int16_t>(static_cast<int32_t>(world.right - world.left));
    actor.height = static_cast<int16_t>(static_cast<int32_t>(world.bottom - world.top));
}
}

// 462280: SetTake updates state before the tree lookup, including failed lookups.
extern "C" int32_t kinoko_actor_set_take(int32_t address, int32_t take) {
    auto &actor = memory<ActorAnimationState>(static_cast<uint32_t>(address));
    const uint32_t manager = static_cast<uint32_t>(actor.manager);
    actor.take = take;
    actor.frame_index = 0;
    actor.frame_time = 0;
    int32_t entry = 0;
    function_4706c0_this(static_cast<int32_t>(manager + 36), &entry, &take);
    if (entry == memory<int32_t>(manager + 40))
        return entry;
    actor.animation = memory<uint32_t>(static_cast<uint32_t>(entry) + 16);
    const auto &animation = memory<Animation>(actor.animation);
    actor.animation_flags = animation.flags;
    update_bounds(actor, animation);
    return static_cast<int32_t>(select_frame(actor, animation, 0));
}

extern "C" int32_t __fastcall kinoko_actor_set_take_method(int32_t actor, void *, int32_t take) {
    return kinoko_actor_set_take(actor, take);
}

// 45E120 after the script callback: a changed take defers ticking until next update.
extern "C" void kinoko_actor_advance_animation(int32_t address, int32_t take_before_callback) {
    auto &actor = memory<ActorAnimationState>(static_cast<uint32_t>(address));
    if (!actor.current_frame || actor.take != take_before_callback)
        return;
    actor.frame_time = increment(actor.frame_time);
    const int16_t duration = memory<int16_t>(actor.current_frame + frame_duration_offset);
    if (actor.frame_time < duration || !actor.animation)
        return;
    const auto &animation = memory<Animation>(actor.animation);
    const int32_t count = frame_count(animation);
    if (count <= 0)
        return;
    const int32_t next = increment(actor.frame_index);
    actor.frame_index = next;
    if (next == count) {
        if (animation.loops) {
            actor.frame_index = 0;
        } else {
            actor.frame_index = next - 1;
            actor.frame_time = 0;
            return;
        }
    }
    select_frame(actor, animation, actor.frame_index);
    actor.frame_time = 0;
}

// 45FE80 after instance extraction: only an upper-bound clamp exists in the original.
extern "C" void kinoko_actor_sync_animation_state(int32_t address, int32_t source_address) {
    auto &actor = memory<ActorAnimationState>(static_cast<uint32_t>(address));
    const auto &source = memory<ActorAnimationState>(static_cast<uint32_t>(source_address));
    int32_t frame = source.frame_index;
    actor.frame_time = source.frame_time;
    actor.frame_index = frame;
    const auto &animation = memory<Animation>(actor.animation);
    const int32_t count = frame_count(animation);
    if (frame >= count) {
        frame = count - 1;
        if (frame < 0)
            frame = 0;
        actor.frame_index = frame;
    }
    select_frame(actor, animation, frame);
}
