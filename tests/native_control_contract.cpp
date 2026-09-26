#include "kinoko/native_control.h"
#include "kinoko/native_control.hpp"
#include "kinoko/boost_control.hpp"
#include <boost/smart_ptr/detail/sp_counted_base_w32.hpp>
#include <new>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using namespace kinoko::native;
using Address = std::uint32_t;
int32_t address(const void* pointer) { return static_cast<int32_t>(reinterpret_cast<std::uintptr_t>(pointer)); }
void* pointer(Address value) { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(value)); }
void require(bool result, const char* message) { if (!result) throw std::runtime_error(message); }
struct Fixture : boost::detail::sp_counted_base {
    std::atomic<unsigned> disposes{0}, destroys{0}, ordering_errors{0};
    long weak_count() const {
        std::int32_t value;
        std::memcpy(&value, reinterpret_cast<const unsigned char*>(this) + 8, sizeof(value));
        return value;
    }
    void dispose() override {
        if (use_count() != 0 || weak_count() < 1) ++ordering_errors;
        ++disposes;
    }
    void destroy() override {
        if (use_count() != 0 || weak_count() != 0 || disposes != 1) ++ordering_errors;
        ++destroys; // fixture storage belongs to its enclosing scope
    }
    void* get_deleter(const boost::detail::sp_typeinfo&) override { return nullptr; }
};
void initialize(Fixture& fixture, int strong = 1, int weak = 2) {
    fixture.~Fixture();
    new (&fixture) Fixture;
    for (int i = 1; i < strong; ++i) fixture.add_ref_copy();
    for (int i = 1; i < weak; ++i) fixture.weak_add_ref();
}

void owner_slot_and_unaligned_pairs() {
    int borrowed_actor = 0x12345678;
    auto* slot = static_cast<int32_t*>(std::malloc(sizeof(int32_t)));
    require(slot != nullptr, "allocate owner slot");
    *slot = address(&borrowed_actor);
    std::array<unsigned char, 6> holder; holder.fill(0xa7);
    require(kinoko_native_control_create((void*)(uintptr_t)(address(holder.data() + 1)), (void*)(uintptr_t)(address(slot))) == holder.data() + 1,
            "constructor returns holder address");
    Address control_address = 0; std::memcpy(&control_address, holder.data() + 1, sizeof(control_address));
    require(control_address != 0, "create real native control");
    const RecordView<ControlRecord> control(pointer(control_address));
    require(control.get(&ControlRecord::strong) == 1 && control.get(&ControlRecord::weak) == 1,
            "one strong and one implicit weak reference");
    require(upstream::owns_control(pointer(control_address)) &&
            control.get(&ControlRecord::allocation) == static_cast<void*>(slot), "original owner-slot layout");
    require(holder.front() == 0xa7 && holder.back() == 0xa7, "unaligned holder canaries");
    kinoko_native_add_weak((void*)(uintptr_t)(static_cast<int32_t>(control_address)));
    std::array<unsigned char, 10> weak_bytes, out_bytes; weak_bytes.fill(0xb8); out_bytes.fill(0xc9);
    const RecordView<ReferenceRecord> weak(weak_bytes.data() + 1), output(out_bytes.data() + 1);
    weak.set(&ReferenceRecord::allocation, static_cast<void*>(slot));
    weak.set(&ReferenceRecord::control, pointer(control_address));
    auto* out = reinterpret_cast<int32_t*>(out_bytes.data() + 1);
    require(kinoko_native_weak_pair_lock((const void*)(uintptr_t)(address(weak_bytes.data() + 1)), out) == out, "lock returns output");
    require(output.get(&ReferenceRecord::allocation) == static_cast<void*>(slot) &&
            output.get(&ReferenceRecord::control) == pointer(control_address) && control.get(&ControlRecord::strong) == 2,
            "weak lock owns exactly one strong reference");
    kinoko_native_release_strong((void*)(uintptr_t)(static_cast<int32_t>(control_address)));
    require(control.get(&ControlRecord::strong) == 1, "nonfinal strong release");
    kinoko_native_release_strong((void*)(uintptr_t)(static_cast<int32_t>(control_address)));
    require(control.get(&ControlRecord::strong) == 0 && control.get(&ControlRecord::weak) == 1 &&
            control.get(&ControlRecord::allocation) == 0, "dispose owner slot before remaining weak owner");
    require(borrowed_actor == 0x12345678, "the Actor itself is not disposed with its owner slot");
    kinoko_native_weak_pair_lock((const void*)(uintptr_t)(address(weak_bytes.data() + 1)), out);
    require(output.get(&ReferenceRecord::allocation) == 0 && output.get(&ReferenceRecord::control) == 0,
            "expired weak reference does not resurrect or expose a dangling allocation");
    require(weak_bytes.front() == 0xb8 && weak_bytes.back() == 0xb8 &&
            out_bytes.front() == 0xc9 && out_bytes.back() == 0xc9, "unaligned pair canaries");
    kinoko_native_release_weak((void*)(uintptr_t)(static_cast<int32_t>(control_address)));
    require(kinoko_native_control_create((void*)(uintptr_t)(0), (void*)(uintptr_t)(address(&borrowed_actor))) == 0, "null holder is a no-op");
    require(kinoko_native_weak_pair_lock((const void*)(uintptr_t)(0), nullptr) == nullptr, "null output is checked before source access");
    kinoko_native_add_weak((void*)(uintptr_t)(0)); kinoko_native_release_weak((void*)(uintptr_t)(0)); kinoko_native_release_strong((void*)(uintptr_t)(0));
}

void callback_order_and_aliases() {
    Fixture fixture; initialize(fixture, 2, 2);
    kinoko_native_release_strong((void*)(uintptr_t)(address(&fixture)));
    require(fixture.use_count() == 1 && fixture.disposes == 0, "custom nonfinal strong release");
    ReferenceRecord pair{reinterpret_cast<void*>(0x11223344), &fixture};
    kinoko_native_weak_pair_lock((const void*)(uintptr_t)(address(&pair)), reinterpret_cast<int32_t*>(&pair));
    require(pair.allocation == 0 && pair.control == 0 && fixture.use_count() == 1,
            "retain historical clear-before-read for an aliased pair");
    kinoko_native_release_strong((void*)(uintptr_t)(address(&fixture)));
    require(fixture.disposes == 1 && fixture.destroys == 0 && fixture.weak_count() == 1,
            "custom dispose precedes implicit weak decrement");
    kinoko_native_release_weak((void*)(uintptr_t)(address(&fixture)));
    require(fixture.destroys == 1 && fixture.ordering_errors == 0, "last weak owner invokes exact thiscall destroy");
    initialize(fixture, 1, 1);
    kinoko_native_release_strong((void*)(uintptr_t)(address(&fixture)));
    require(fixture.disposes == 1 && fixture.destroys == 1 && fixture.ordering_errors == 0,
            "no explicit weak owner: dispose then immediate destroy");
}

void concurrent_locks(bool race_final_release) {
    for (int pass = 0; pass != 32; ++pass) {
        Fixture fixture; initialize(fixture);
        const ReferenceRecord weak{reinterpret_cast<void*>(0x11223344), &fixture};
        std::atomic<bool> begin{false};
        std::atomic<unsigned> errors{0}, successes{0};
        std::vector<std::thread> workers;
        for (int worker = 0; worker != 4; ++worker) workers.emplace_back([&] {
            while (!begin.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int iteration = 0; iteration != 1024; ++iteration) {
                ReferenceRecord locked{};
                kinoko_native_weak_pair_lock((const void*)(uintptr_t)(address(&weak)), reinterpret_cast<int32_t*>(&locked));
                if (locked.control) {
                    if (locked.control != weak.control || locked.allocation != weak.allocation) ++errors;
                    ++successes;
                    kinoko_native_release_strong(locked.control);
                } else if (locked.allocation || !race_final_release) ++errors;
            }
        });
        begin.store(true, std::memory_order_release);
        if (race_final_release) kinoko_native_release_strong((void*)(uintptr_t)(address(&fixture)));
        for (auto& worker : workers) worker.join();
        if (!race_final_release) {
            require(successes == 4096 && fixture.use_count() == 1 && fixture.disposes == 0,
                    "balanced strong refs under contention with a surviving strong owner");
            kinoko_native_release_strong((void*)(uintptr_t)(address(&fixture)));
        }
        require(errors == 0 && fixture.use_count() == 0 && fixture.weak_count() == 1 &&
                fixture.disposes == 1 && fixture.destroys == 0, "race releases exactly once, weak owner retains control");
        ReferenceRecord expired{reinterpret_cast<void*>(1), reinterpret_cast<void*>(1)};
        kinoko_native_weak_pair_lock((const void*)(uintptr_t)(address(&weak)), reinterpret_cast<int32_t*>(&expired));
        require(expired.allocation == 0 && expired.control == 0, "post-race lock fails without resurrection");
        kinoko_native_release_weak((void*)(uintptr_t)(address(&fixture)));
        require(fixture.destroys == 1 && fixture.ordering_errors == 0, "post-race destroy order");
    }
}
}

int main() {
    try {
        owner_slot_and_unaligned_pairs();
        callback_order_and_aliases();
        concurrent_locks(false); concurrent_locks(true);
        std::puts("native control: owner slot, unaligned pairs, aliases, exact callbacks and 262144 contended lock attempts OK");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
