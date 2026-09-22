#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/game_host.h"
#include "kinoko/game_script_host.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/legacy_memory.hpp"

#include <cstring>

extern "C" {
void * kinoko_sqplus_object_instance(void * object, void * index);
int32_t  kinoko_sqplus_object_destroy(void * object);
void retdec_trace_star_state(const char *phase, int32_t actor);
}

namespace {
static_assert(sizeof(void *) == 4, "Actor methods require the original Win32 layout.");

using namespace kinoko::actor;
using kinoko::legacy::pointer;
using kinoko::legacy::address;

class ActorMethods {
public:
    explicit ActorMethods(KinokoActor* actor) : actor_(actor), view_(actor) {}

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
        return kinoko_collision_event_at_point(kinoko_game_objects()->map,static_cast<int32_t>(view_.get(&ActorRecord::x)),
            static_cast<int32_t>(view_.get(&ActorRecord::y)), layer,
            reinterpret_cast<int32_t*>(view_.bytes(&ActorRecord::chip_cache_storage) + sizeof(int32_t) * layer));
    }

    int32_t reset_priority(int32_t priority) const {
        if (!actor_)
            return 0;
        view_.set(&ActorRecord::priority, priority);
        return kinoko_actor_manager_reindex(kinoko_game_objects()->actors, actor_);
    }

    int32_t release() const {
        retdec_trace_star_state("release", address(actor_));
        view_.set(&ActorRecord::release_pending, uint8_t{1});
        ManagerView(view_.get(&ActorRecord::manager))
            .set(&ManagerPrefix::cleanup_pending, uint8_t{1});
        return 1;
    }

    KinokoActor* set_init_data(const void* source) const {
        if (!actor_ || !source)
            return 0;
        std::memcpy(view_.bytes(&ActorRecord::initial), source, sizeof(InitialData));
        return actor_;
    }

    int32_t sync_animation(int32_t vtable, int32_t type, int32_t value) const {
        int32_t incoming[3] = {vtable, type, value};
        const auto incoming_address = static_cast<int32_t>(
            reinterpret_cast<uintptr_t>(incoming));
        if (type == 0x0a008000 && view_.get(&ActorRecord::animation)) {
            kinoko_actor_sync_animation_state(actor_, pointer<KinokoActor>((int32_t)(intptr_t)(kinoko_sqplus_object_instance((void *)(intptr_t)(incoming_address), (void *)(intptr_t)(0)))));
        }
        // The by-value SqPlus object owns an external VM reference on entry.
        return kinoko_sqplus_object_destroy((void *)(intptr_t)(incoming_address));
    }

private:
    KinokoActor* actor_;
    ActorView view_;
};
}

// 45F760: Actor.SetChipFlag sign-extends its argument into the 64-bit field.
extern "C" int64_t __fastcall kinoko_actor_set_chip_flags(
    KinokoActor* actor, void *, int32_t flags) {
    return ActorMethods(actor).set_chip_flags(flags);
}

// 45F780: Actor.SetChipBoundType.
extern "C" int32_t __fastcall kinoko_actor_set_chip_bound_type(
    KinokoActor* actor, void *, uint16_t shape) {
    return ActorMethods(actor).set_chip_bound_type(shape);
}

// 45F7A0: Actor.GetChipID updates the actor's per-layer cache.
extern "C" int32_t __fastcall kinoko_actor_get_chip_id(
    KinokoActor* actor, void *, int32_t layer) {
    return ActorMethods(actor).chip_id(layer);
}

// 45F7E0: Actor.ResetPriority also reorders the manager's actor list.
extern "C" int32_t kinoko_actor_reset_priority(KinokoActor* actor, int32_t priority) {
    return ActorMethods(actor).reset_priority(priority);
}

extern "C" int32_t __fastcall kinoko_actor_reset_priority_method(
    KinokoActor* actor, void *, int32_t priority) {
    return kinoko_actor_reset_priority(actor, priority);
}

// 45F800: retain the original script spelling "InterrputCollisionCallback".
extern "C" int32_t __fastcall kinoko_actor_interrupt_collision(KinokoActor* actor, void *) {
    return kinoko_collision_dispatch_actor(kinoko_game_objects()->actors, actor);
}

// 45F810: Actor.GetChipFlag queries the current collision scene.
extern "C" int32_t __fastcall kinoko_actor_get_chip_flags(KinokoActor* actor, void *) {
    return static_cast<int32_t>(kinoko_collision_chip_flags(kinoko_game_collision_state(), actor));
}

// 45F820: Actor.IsExistChip preserves the original rectangle query.
extern "C" int32_t __fastcall kinoko_actor_has_chip(KinokoActor* actor, void *,
    float left, float top, float right, float bottom) {
    return kinoko_collision_has_chip(kinoko_game_collision_state(), actor, left, top, right, bottom);
}

// 45DBC0: deferred release; the manager performs the actual destruction.
extern "C" int32_t __fastcall kinoko_actor_release(KinokoActor* actor, void *) {
    return ActorMethods(actor).release();
}

// 45DE70: retain the existing explicit-receiver C interface for native callers.
extern "C" KinokoActor* kinoko_actor_set_init_data(KinokoActor* actor, const void* source) {
    return ActorMethods(actor).set_init_data(source);
}

// 45FE80: consume one 12-byte SquirrelObject with the original thiscall ABI.
extern "C" int32_t __fastcall kinoko_actor_sync_animation(KinokoActor* actor, void *,
    int32_t vtable, int32_t type, int32_t value) {
    return ActorMethods(actor).sync_animation(vtable, type, value);
}
