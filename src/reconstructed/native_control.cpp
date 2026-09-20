#include "kinoko/native_control.h"
#include "kinoko/native_control.hpp"
#include "kinoko/boost_control.hpp"
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Native control count slots and member callbacks require MSVC Win32.
#endif

namespace {
using namespace kinoko::native;
using Address = std::uint32_t;
void* pointer(Address value) noexcept { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(value)); }
Address address(const void* value) noexcept { return static_cast<Address>(reinterpret_cast<std::uintptr_t>(value)); }

// Actor ownership is created only by kinoko_native_control_create. Its weak
// pairs copy that pointer; no original vtable/control storage crosses this API.
upstream::CountedControl* native_control(Address value) noexcept {
    return static_cast<upstream::CountedControl*>(pointer(value));
}
} // namespace

extern "C" int32_t kinoko_native_control_create(int32_t holder_address, int32_t allocation_address) {
    if (!holder_address) return 0;
    auto* holder = pointer(static_cast<Address>(holder_address));
    Address result = 0;
    std::memcpy(holder, &result, sizeof(result));
    if (auto* control = upstream::create_owner_control(pointer(static_cast<Address>(allocation_address))))
        result = address(control);
    // Allocation failure leaves the input allocation owned by the caller.
    std::memcpy(holder, &result, sizeof(result));
    return holder_address;
}

extern "C" int32_t* kinoko_native_weak_pair_lock(int32_t pair_address, int32_t* output_pair) {
    if (!output_pair) return nullptr;
    const RecordView<ReferenceRecord> output(output_pair);
    output.clear(); // Preserve clear-before-read, including aliased pairs.
    const RecordView<ReferenceRecord> source(pointer(static_cast<Address>(pair_address)));
    const auto control = source.get(&ReferenceRecord::control);
    const bool retained = control && upstream::lock(native_control(control));
    if (retained) {
        output.set(&ReferenceRecord::control, control);
        output.set(&ReferenceRecord::allocation, source.get(&ReferenceRecord::allocation));
    }
    return output_pair;
}

extern "C" void kinoko_native_add_strong(int32_t control_address) {
    if (control_address) upstream::add_strong(native_control(static_cast<Address>(control_address)));
}

extern "C" void kinoko_native_add_weak(int32_t control_address) {
    if (control_address) upstream::add_weak(native_control(static_cast<Address>(control_address)));
}

extern "C" void kinoko_native_release_weak(int32_t control_address) {
    if (control_address) upstream::release_weak(native_control(static_cast<Address>(control_address)));
}

extern "C" void kinoko_native_release_strong(int32_t control_address) {
    if (control_address) upstream::release_strong(native_control(static_cast<Address>(control_address)));
}
