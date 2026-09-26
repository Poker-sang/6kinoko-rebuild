#pragma once
#include "kinoko/collision_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/native_control.h"

namespace kinoko::collision {
using kinoko::legacy::address;
using kinoko::legacy::pointer;
// A successful lock owns precisely one temporary strong reference. The native
// pair borrows an allocation slot; never retain the borrowed Actor itself.
class LockedParent final {
    KinokoNativeReference locked_{};
public:
    explicit LockedParent(ActorReference *reference) {
        kinoko_native_weak_pair_lock(reference, &locked_);
    }
    ~LockedParent() { kinoko_native_release_strong(locked_.control); }
    LockedParent(const LockedParent&) = delete;
    LockedParent& operator=(const LockedParent&) = delete;
    KinokoActor *actor() const {
        return locked_.allocation ? kinoko::legacy::load<KinokoActor *>(locked_.allocation) : nullptr;
    }
    bool has_slot() const { return locked_.allocation != nullptr; }
};

}
