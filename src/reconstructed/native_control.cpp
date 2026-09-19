#include "kinoko/native_control.h"
#include "kinoko/native_control.hpp"
#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Native control count slots and member callbacks require MSVC Win32.
#endif
extern "C" int32_t kinoko_actor_control_vtable(void);

namespace {
using namespace kinoko::native;
using Address = std::uint32_t;
void* pointer(Address value) noexcept { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(value)); }
Address address(const void* value) noexcept { return static_cast<Address>(reinterpret_cast<std::uintptr_t>(value)); }

class ControlView final {
    RecordView<ControlRecord> view_;
    volatile LONG* count(std::int32_t ControlRecord::* member) const noexcept {
        return reinterpret_cast<volatile LONG*>(view_.bytes(member));
    }
public:
    explicit ControlView(Address value) noexcept : view_(pointer(value)) {}
    Address get(Address ControlRecord::* member) const noexcept { return view_.get(member); }
    void set(Address ControlRecord::* member, Address value) const noexcept { view_.set(member, value); }
    LONG add_strong(LONG delta) const noexcept { return InterlockedExchangeAdd(count(&ControlRecord::strong), delta); }
    LONG add_weak(LONG delta) const noexcept { return InterlockedExchangeAdd(count(&ControlRecord::weak), delta); }
    bool try_add_strong() const noexcept {
        auto* strong = count(&ControlRecord::strong);
        // As in the recovered 45DAC0 path, zero is terminal. A weak owner must
        // keep the block alive throughout this operation. Count slots are
        // aligned even when the containing reference pair is unaligned.
        LONG expected = *strong;
        while (expected != 0) {
            const auto next = static_cast<LONG>(static_cast<Address>(expected) + 1u);
            if (InterlockedCompareExchange(strong, next, expected) == expected) return true;
            expected = *strong;
        }
        return false;
    }
};

void call_method(Address control, Address table, Address ControlTable::* member) {
    if (!table) return;
    const auto entry = RecordView<ControlTable>(pointer(table)).get(member);
    if (!entry) return;
    using Method = void (__thiscall*)(void*);
    reinterpret_cast<Method>(pointer(entry))(pointer(control));
}
struct FreeControl {
    void operator()(void* memory) const noexcept { std::free(memory); }
};
} // namespace

extern "C" int32_t kinoko_native_control_create(int32_t holder_address, int32_t allocation_address) {
    if (!holder_address) return 0;
    auto* holder = pointer(static_cast<Address>(holder_address));
    Address result = 0;
    std::memcpy(holder, &result, sizeof(result));
    std::unique_ptr<void, FreeControl> memory(std::malloc(sizeof(ControlRecord)));
    if (memory) {
        const RecordView<ControlRecord> control(memory.get());
        control.set(&ControlRecord::vtable, static_cast<Address>(kinoko_actor_control_vtable()));
        control.set(&ControlRecord::strong, std::int32_t{1});
        control.set(&ControlRecord::weak, std::int32_t{1}); // implicit weak owner
        control.set(&ControlRecord::allocation, static_cast<Address>(allocation_address));
        result = address(memory.release());
    }
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
    if (control && ControlView(control).try_add_strong()) {
        output.set(&ReferenceRecord::control, control);
        output.set(&ReferenceRecord::allocation, source.get(&ReferenceRecord::allocation));
    }
    return output_pair;
}

extern "C" void kinoko_native_add_weak(int32_t control_address) {
    if (control_address) ControlView(static_cast<Address>(control_address)).add_weak(1);
}

extern "C" void kinoko_native_release_weak(int32_t control_address) {
    if (!control_address) return;
    const auto control = static_cast<Address>(control_address);
    const ControlView view(control);
    if (view.add_weak(-1) != 1) return;
    const auto table = view.get(&ControlRecord::vtable);
    if (table == static_cast<Address>(kinoko_actor_control_vtable())) std::free(pointer(control));
    else call_method(control, table, &ControlTable::destroy);
}

extern "C" void kinoko_native_release_strong(int32_t control_address) {
    if (!control_address) return;
    const auto control = static_cast<Address>(control_address);
    const ControlView view(control);
    if (view.add_strong(-1) != 1) return;
    const auto table = view.get(&ControlRecord::vtable);
    if (table == static_cast<Address>(kinoko_actor_control_vtable())) {
        std::free(pointer(view.get(&ControlRecord::allocation)));
        view.set(&ControlRecord::allocation, 0);
        kinoko_native_release_weak(control_address);
        return;
    }
    call_method(control, table, &ControlTable::dispose);
    if (view.add_weak(-1) == 1) call_method(control, table, &ControlTable::destroy);
}
