#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/legacy_memory.hpp"

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

using namespace kinoko::actor;
using kinoko::legacy::pointer;
using kinoko::legacy::address;

class ActorMethods {
public:
    explicit ActorMethods(int32_t address) : address_(address), view_(pointer(address)) {}

    int64_t set_chip_flags(int32_t flags) const {
        const int64_t extended = flags;
        view_.view(&ActorRecord::initial).set(&InitialData::chip_flags, extended);
        return extended;
    }

    int32_t set_chip_bound_type(uint16_t shape) const {
        view_.view(&ActorRecord::initial).set(&InitialData::chip_bound_type, shape);
        return shape;
    }

    int32_t chip_id(int32_t layer) const {
        return function_4698a0(static_cast<int32_t>(view_.get(&ActorRecord::x)),
            static_cast<int32_t>(view_.get(&ActorRecord::y)), layer,
            address(view_.bytes(&ActorRecord::chip_cache_storage) + sizeof(int32_t) * layer));
    }

    int32_t reset_priority(int32_t priority) const {
        if (!address_)
            return 0;
        view_.set(&ActorRecord::priority, priority);
        return function_469780(address_, address_);
    }

    int32_t release() const {
        retdec_trace_star_state("release", address_);
        view_.set(&ActorRecord::release_pending, uint8_t{1});
        ManagerView(pointer(static_cast<int32_t>(view_.get(&ActorRecord::manager))))
            .set(&ManagerPrefix::cleanup_pending, uint8_t{1});
        return 1;
    }

    int32_t set_init_data(int32_t source) const {
        if (!address_ || !source)
            return 0;
        std::memcpy(view_.bytes(&ActorRecord::initial), pointer(source), sizeof(InitialData));
        return address_;
    }

    int32_t sync_animation(int32_t vtable, int32_t type, int32_t value) const {
        int32_t incoming[3] = {vtable, type, value};
        const auto incoming_address = static_cast<int32_t>(
            reinterpret_cast<uintptr_t>(incoming));
        if (type == 0x0a008000 && view_.get(&ActorRecord::animation)) {
            kinoko_actor_sync_animation_state(address_, function_4a9b40_this(incoming_address, 0));
        }
        // The by-value SqPlus object owns an external VM reference on entry.
        return function_4a9d70_this(incoming_address);
    }

private:
    int32_t address_;
    ActorView view_;
};
}

// 45F760: Actor.SetChipFlag sign-extends its argument into the 64-bit field.
extern "C" int64_t __fastcall kinoko_actor_set_chip_flags(
    int32_t actor, void *, int32_t flags) {
    return ActorMethods(actor).set_chip_flags(flags);
}

// 45F780: Actor.SetChipBoundType.
extern "C" int32_t __fastcall kinoko_actor_set_chip_bound_type(
    int32_t actor, void *, uint16_t shape) {
    return ActorMethods(actor).set_chip_bound_type(shape);
}

// 45F7A0: Actor.GetChipID updates the actor's per-layer cache.
extern "C" int32_t __fastcall kinoko_actor_get_chip_id(
    int32_t actor, void *, int32_t layer) {
    return ActorMethods(actor).chip_id(layer);
}

// 45F7E0: Actor.ResetPriority also reorders the manager's actor list.
extern "C" int32_t kinoko_actor_reset_priority(int32_t actor, int32_t priority) {
    return ActorMethods(actor).reset_priority(priority);
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
    return ActorMethods(actor).release();
}

// 45DE70: retain the existing explicit-receiver C interface for native callers.
extern "C" int32_t kinoko_actor_set_init_data(int32_t actor, int32_t source) {
    return ActorMethods(actor).set_init_data(source);
}

// 45FE80: consume one 12-byte SquirrelObject with the original thiscall ABI.
extern "C" int32_t __fastcall kinoko_actor_sync_animation(int32_t actor, void *,
    int32_t vtable, int32_t type, int32_t value) {
    return ActorMethods(actor).sync_animation(vtable, type, value);
}
