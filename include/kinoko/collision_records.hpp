#pragma once
#include "kinoko/map_collision.h"
#include "kinoko/map_activation.h"
#include "kinoko/actor_collision.h"
#include "kinoko/native_record_view.hpp"
#include "kinoko/native_control.hpp"
#include <cstddef>

namespace kinoko::collision {
// Schemas of existing Win32 storage, not constructed C++ objects. The third
// buffer word is native_buffer ownership, never the original vector capacity.
template<class T> struct Buffer { T *begin, *end; void *storage_owner; };
struct ActorReference {
    KinokoActor **slot; // control-owned slot, never an owned Actor
    kinoko::native::ControlRecord *control;
};
using HitBuffer = Buffer<KinokoCollisionRecord>;
struct StateRecord {
    KinokoActorManager *manager;
    Buffer<KinokoActLayout *> layouts; // borrowed; ordered with map_parents
    uint32_t unknown16;
    Buffer<int32_t> layer_hit_ends;
    uint32_t unknown32;
    HitBuffer hits;
    uint32_t unknown48;
    Buffer<ActorReference> map_parents; // owns one weak reference per entry
    uint32_t unknown64;
    Buffer<KinokoActor *> actors; // borrowed active collision candidates
    uint32_t unknown80;
    int32_t actor_count;
};
using StateView = kinoko::native::RecordView<StateRecord>;
using ReferenceView = kinoko::native::RecordView<ActorReference>;
static_assert(sizeof(ActorReference) == 8 && sizeof(HitBuffer) == 12);
static_assert(offsetof(StateRecord, layouts) == 4);
static_assert(offsetof(StateRecord, layer_hit_ends) == 20);
static_assert(offsetof(StateRecord, hits) == 36);
static_assert(offsetof(StateRecord, map_parents) == 52);
static_assert(offsetof(StateRecord, actors) == 68);
static_assert(offsetof(StateRecord, actor_count) == 84 && sizeof(StateRecord) == 88);
}
