#pragma once
#include "kinoko/collision_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/native_control.h"

namespace kinoko::collision {
using kinoko::legacy::address;
using kinoko::legacy::pointer;
// A successful lock owns precisely one temporary strong reference. The native
// pair ABI uses integer words; convert only here, never retain the borrowed Actor.
class LockedParent final {
    int32_t words_[2]{};
public:
    explicit LockedParent(ActorReference *reference) {
        kinoko_native_weak_pair_lock(address(reference), words_);
    }
    ~LockedParent() { kinoko_native_release_strong(words_[1]); }
    LockedParent(const LockedParent&) = delete;
    LockedParent& operator=(const LockedParent&) = delete;
    KinokoActor *actor() const {
        return words_[0] ? kinoko::legacy::load<KinokoActor *>(pointer(words_[0])) : nullptr;
    }
    bool has_slot() const { return words_[0] != 0; }
};

}
