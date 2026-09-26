#include "kinoko/native_control.h"
#include "kinoko/native_control.hpp"
#include "kinoko/boost_control.hpp"
#include <cstring>

namespace {
using namespace kinoko::native;
upstream::CountedControl* native_control(void* value) noexcept {
    return static_cast<upstream::CountedControl*>(value);
}
}
extern "C" void* kinoko_native_control_create(void* holder, void* allocation) {
    if (!holder) return nullptr;
    void* result = nullptr;
    std::memcpy(holder, &result, sizeof result);
    result = upstream::create_owner_control(allocation);
    // Failure leaves the allocation owned by the caller; preserve initial clear.
    std::memcpy(holder, &result, sizeof result);
    return holder;
}
extern "C" void* kinoko_native_weak_pair_lock(const void* pair, void* destination) {
    if (!destination) return nullptr;
    const RecordView<ReferenceRecord> output(destination);
    output.clear(); // Clear-before-read is observable for aliased pairs.
    const RecordView<ReferenceRecord> source(const_cast<void*>(pair));
    auto* control = source.get(&ReferenceRecord::control);
    if (control && upstream::lock(native_control(control))) {
        output.set(&ReferenceRecord::control, control);
        output.set(&ReferenceRecord::allocation, source.get(&ReferenceRecord::allocation));
    }
    return destination;
}
extern "C" void kinoko_native_add_strong(void* control) {
    if (control) upstream::add_strong(native_control(control));
}
extern "C" void kinoko_native_add_weak(void* control) {
    if (control) upstream::add_weak(native_control(control));
}
extern "C" void kinoko_native_release_weak(void* control) {
    if (control) upstream::release_weak(native_control(control));
}
extern "C" void kinoko_native_release_strong(void* control) {
    if (control) upstream::release_strong(native_control(control));
}
