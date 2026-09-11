#include "kinoko/actor_methods.h"

#include <cstring>

extern "C" {
int32_t function_469780(int32_t actor, int32_t unused);
int32_t function_4697a0(int32_t actor);
int32_t function_4697c0(int32_t actor);
int32_t function_4697e0(int32_t actor, float left, float top, float right, float bottom);
int32_t function_4698a0(int32_t x, int32_t y, int32_t layer, int32_t cached_id);
int32_t function_4a9b40_this(int32_t object, int32_t index);
int32_t function_4a9d70_this(int32_t object);
void retdec_trace_star_state(const char *phase, int32_t actor);
}

namespace {
static_assert(sizeof(void *) == 4, "Actor methods require the original Win32 layout.");

class ActorView {
public:
    explicit ActorView(int32_t address) : address_(address) {}

    int64_t set_chip_flags(int32_t flags) const {
        const int64_t extended = flags;
        write(chip_flags_offset, extended);
        return extended;
    }

    int32_t set_chip_bound_type(uint16_t shape) const {
        write(chip_bound_type_offset, shape);
        return shape;
    }

    int32_t chip_id(int32_t layer) const {
        return function_4698a0(static_cast<int32_t>(read<float>(x_offset)),
            static_cast<int32_t>(read<float>(y_offset)), layer,
            address_ + chip_ids_offset + 4 * layer);
    }

    int32_t reset_priority(int32_t priority) const {
        if (!address_)
            return 0;
        write(priority_offset, priority);
        return function_469780(address_, address_);
    }

    int32_t release() const {
        retdec_trace_star_state("release", address_);
        write<uint8_t>(release_pending_offset, 1);
        ActorView(read<int32_t>(manager_offset)).write<uint8_t>(
            manager_cleanup_pending_offset, 1);
        return 1;
    }

    int32_t set_init_data(int32_t source) const {
        if (!address_ || !source)
            return 0;
        std::memcpy(bytes() + init_data_offset, ActorView(source).bytes(), 48);
        return address_;
    }

    int32_t sync_animation(int32_t vtable, int32_t type, int32_t value) const {
        int32_t incoming[3] = {vtable, type, value};
        const auto incoming_address = static_cast<int32_t>(
            reinterpret_cast<uintptr_t>(incoming));
        if (type == 0x0a008000 && read<int32_t>(animation_offset)) {
            const ActorView source(function_4a9b40_this(incoming_address, 0));
            int32_t frame = source.read<int32_t>(frame_index_offset);
            write(frame_time_offset, source.read<int32_t>(frame_time_offset));
            write(frame_index_offset, frame);
            const ActorView animation(read<int32_t>(animation_offset));
            const uint32_t begin = animation.read<uint32_t>(frames_begin_offset);
            const uint32_t end = animation.read<uint32_t>(frames_end_offset);
            const int32_t count = static_cast<int32_t>(end - begin) / frame_stride;
            if (frame >= count) {
                frame = count - 1;
                if (frame < 0)
                    frame = 0;
                write(frame_index_offset, frame);
            }
            // 462250 updates both aliases, with the original 32-bit arithmetic.
            const uint32_t selected = begin + static_cast<uint32_t>(frame) * frame_stride;
            write(current_frame_offset, selected);
            write(sprite_frame_offset, selected);
        }
        // The by-value SqPlus object owns an external VM reference on entry.
        return function_4a9d70_this(incoming_address);
    }

private:
    enum Offset {
        release_pending_offset = 22, manager_offset = 148,
        manager_cleanup_pending_offset = 120, init_data_offset = 376,
        sprite_frame_offset = 152, animation_offset = 200,
        current_frame_offset = 204, frame_index_offset = 212,
        frame_time_offset = 216, frames_begin_offset = 8, frames_end_offset = 12,
        frame_stride = 248,
        priority_offset = 228, x_offset = 240, y_offset = 244,
        chip_flags_offset = 392, chip_bound_type_offset = 410,
        chip_ids_offset = 512
    };

    template <typename T>
    T read(int32_t offset) const {
        T value;
        std::memcpy(&value, bytes() + offset, sizeof(value));
        return value;
    }

    template <typename T>
    void write(int32_t offset, T value) const {
        std::memcpy(bytes() + offset, &value, sizeof(value));
    }

    unsigned char *bytes() const {
        return reinterpret_cast<unsigned char *>(static_cast<uintptr_t>(
            static_cast<uint32_t>(address_)));
    }

    int32_t address_;
};
}

// 45F760: Actor.SetChipFlag sign-extends its argument into the 64-bit field.
extern "C" int64_t __fastcall kinoko_actor_set_chip_flags(
    int32_t actor, void *, int32_t flags) {
    return ActorView(actor).set_chip_flags(flags);
}

// 45F780: Actor.SetChipBoundType.
extern "C" int32_t __fastcall kinoko_actor_set_chip_bound_type(
    int32_t actor, void *, uint16_t shape) {
    return ActorView(actor).set_chip_bound_type(shape);
}

// 45F7A0: Actor.GetChipID updates the actor's per-layer cache.
extern "C" int32_t __fastcall kinoko_actor_get_chip_id(
    int32_t actor, void *, int32_t layer) {
    return ActorView(actor).chip_id(layer);
}

// 45F7E0: Actor.ResetPriority also reorders the manager's actor list.
extern "C" int32_t kinoko_actor_reset_priority(int32_t actor, int32_t priority) {
    return ActorView(actor).reset_priority(priority);
}

extern "C" int32_t __fastcall kinoko_actor_reset_priority_method(
    int32_t actor, void *, int32_t priority) {
    return kinoko_actor_reset_priority(actor, priority);
}

// 45F800: retain the original script spelling "InterrputCollisionCallback".
extern "C" int32_t __fastcall kinoko_actor_interrupt_collision(int32_t actor, void *) {
    return function_4697a0(actor);
}

// 45F810: Actor.GetChipFlag queries the current collision scene.
extern "C" int32_t __fastcall kinoko_actor_get_chip_flags(int32_t actor, void *) {
    return function_4697c0(actor);
}

// 45F820: Actor.IsExistChip preserves the original rectangle query.
extern "C" int32_t __fastcall kinoko_actor_has_chip(int32_t actor, void *,
    float left, float top, float right, float bottom) {
    return function_4697e0(actor, left, top, right, bottom);
}

// 45DBC0: deferred release; the manager performs the actual destruction.
extern "C" int32_t __fastcall kinoko_actor_release(int32_t actor, void *) {
    return ActorView(actor).release();
}

// 45DE70: retain the existing explicit-receiver C interface for native callers.
extern "C" int32_t kinoko_actor_set_init_data(int32_t actor, int32_t source) {
    return ActorView(actor).set_init_data(source);
}

// 45FE80: consume one 12-byte SquirrelObject with the original thiscall ABI.
extern "C" int32_t __fastcall kinoko_actor_sync_animation(int32_t actor, void *,
    int32_t vtable, int32_t type, int32_t value) {
    return ActorView(actor).sync_animation(vtable, type, value);
}
