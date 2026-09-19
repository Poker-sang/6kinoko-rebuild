#include "kinoko/native_control.h"
#include "kinoko/native_control.hpp"
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
Address known_table_tag = 0;

struct Fixture {
    ControlRecord record;
    std::atomic<unsigned> disposes{0}, destroys{0}, ordering_errors{0};
};
void __fastcall dispose(Fixture* fixture, void*) {
    if (fixture->record.strong != 0 || fixture->record.weak < 1) ++fixture->ordering_errors;
    ++fixture->disposes;
}
void __fastcall destroy(Fixture* fixture, void*) {
    if (fixture->record.strong != 0 || fixture->record.weak != 0 || fixture->disposes != 1)
        ++fixture->ordering_errors;
    ++fixture->destroys;
}
const ControlTable custom_table{0, static_cast<Address>(address(reinterpret_cast<void*>(dispose))),
                                  static_cast<Address>(address(reinterpret_cast<void*>(destroy)))};
void initialize(Fixture& fixture, int strong = 1, int weak = 2) {
    fixture.record = {static_cast<Address>(address(&custom_table)), strong, weak, 0};
    fixture.disposes = 0; fixture.destroys = 0; fixture.ordering_errors = 0;
}

void owner_slot_and_unaligned_pairs() {
    int borrowed_actor = 0x12345678;
    auto* slot = static_cast<int32_t*>(std::malloc(sizeof(int32_t)));
    require(slot != nullptr, "allocate owner slot");
    *slot = address(&borrowed_actor);
    std::array<unsigned char, 6> holder; holder.fill(0xa7);
    require(kinoko_native_control_create(address(holder.data() + 1), address(slot)) == address(holder.data() + 1),
            "constructor returns holder address");
    Address control_address = 0; std::memcpy(&control_address, holder.data() + 1, sizeof(control_address));
    require(control_address != 0, "create real native control");
    const RecordView<ControlRecord> control(pointer(control_address));
    require(control.get(&ControlRecord::strong) == 1 && control.get(&ControlRecord::weak) == 1,
            "one strong and one implicit weak reference");
    require(control.get(&ControlRecord::vtable) == static_cast<Address>(address(&known_table_tag)) &&
            control.get(&ControlRecord::allocation) == static_cast<Address>(address(slot)), "original owner-slot layout");
    require(holder.front() == 0xa7 && holder.back() == 0xa7, "unaligned holder canaries");
    kinoko_native_add_weak(static_cast<int32_t>(control_address));
    std::array<unsigned char, 10> weak_bytes, out_bytes; weak_bytes.fill(0xb8); out_bytes.fill(0xc9);
    const RecordView<ReferenceRecord> weak(weak_bytes.data() + 1), output(out_bytes.data() + 1);
    weak.set(&ReferenceRecord::allocation, static_cast<Address>(address(slot)));
    weak.set(&ReferenceRecord::control, control_address);
    auto* out = reinterpret_cast<int32_t*>(out_bytes.data() + 1);
    require(kinoko_native_weak_pair_lock(address(weak_bytes.data() + 1), out) == out, "lock returns output");
    require(output.get(&ReferenceRecord::allocation) == static_cast<Address>(address(slot)) &&
            output.get(&ReferenceRecord::control) == control_address && control.get(&ControlRecord::strong) == 2,
            "weak lock owns exactly one strong reference");
    kinoko_native_release_strong(static_cast<int32_t>(control_address));
    require(control.get(&ControlRecord::strong) == 1, "nonfinal strong release");
    kinoko_native_release_strong(static_cast<int32_t>(control_address));
    require(control.get(&ControlRecord::strong) == 0 && control.get(&ControlRecord::weak) == 1 &&
            control.get(&ControlRecord::allocation) == 0, "dispose owner slot before remaining weak owner");
    require(borrowed_actor == 0x12345678, "the Actor itself is not disposed with its owner slot");
    kinoko_native_weak_pair_lock(address(weak_bytes.data() + 1), out);
    require(output.get(&ReferenceRecord::allocation) == 0 && output.get(&ReferenceRecord::control) == 0,
            "expired weak reference does not resurrect or expose a dangling allocation");
    require(weak_bytes.front() == 0xb8 && weak_bytes.back() == 0xb8 &&
            out_bytes.front() == 0xc9 && out_bytes.back() == 0xc9, "unaligned pair canaries");
    kinoko_native_release_weak(static_cast<int32_t>(control_address));
    require(kinoko_native_control_create(0, address(&borrowed_actor)) == 0, "null holder is a no-op");
    require(kinoko_native_weak_pair_lock(0, nullptr) == nullptr, "null output is checked before source access");
    kinoko_native_add_weak(0); kinoko_native_release_weak(0); kinoko_native_release_strong(0);
}

void callback_order_and_aliases() {
    Fixture fixture; initialize(fixture, 2, 2);
    kinoko_native_release_strong(address(&fixture));
    require(fixture.record.strong == 1 && fixture.disposes == 0, "custom nonfinal strong release");
    ReferenceRecord pair{0x11223344, static_cast<Address>(address(&fixture))};
    kinoko_native_weak_pair_lock(address(&pair), reinterpret_cast<int32_t*>(&pair));
    require(pair.allocation == 0 && pair.control == 0 && fixture.record.strong == 1,
            "retain historical clear-before-read for an aliased pair");
    kinoko_native_release_strong(address(&fixture));
    require(fixture.disposes == 1 && fixture.destroys == 0 && fixture.record.weak == 1,
            "custom dispose precedes implicit weak decrement");
    kinoko_native_release_weak(address(&fixture));
    require(fixture.destroys == 1 && fixture.ordering_errors == 0, "last weak owner invokes exact thiscall destroy");
    ControlRecord no_table{0, 1, 1, 0x43214321};
    kinoko_native_release_strong(address(&no_table));
    require(no_table.strong == 0 && no_table.weak == 0 && no_table.allocation == 0x43214321,
            "missing methods keep baseline count transitions and do not invent a deleter");
    initialize(fixture, 1, 1);
    kinoko_native_release_strong(address(&fixture));
    require(fixture.disposes == 1 && fixture.destroys == 1 && fixture.ordering_errors == 0,
            "no explicit weak owner: dispose then immediate destroy");
}

void concurrent_locks(bool race_final_release) {
    for (int pass = 0; pass != 32; ++pass) {
        Fixture fixture; initialize(fixture);
        const ReferenceRecord weak{0x11223344, static_cast<Address>(address(&fixture))};
        std::atomic<bool> begin{false};
        std::atomic<unsigned> errors{0}, successes{0};
        std::vector<std::thread> workers;
        for (int worker = 0; worker != 4; ++worker) workers.emplace_back([&] {
            while (!begin.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int iteration = 0; iteration != 1024; ++iteration) {
                ReferenceRecord locked{};
                kinoko_native_weak_pair_lock(address(&weak), reinterpret_cast<int32_t*>(&locked));
                if (locked.control) {
                    if (locked.control != weak.control || locked.allocation != weak.allocation) ++errors;
                    ++successes;
                    kinoko_native_release_strong(static_cast<int32_t>(locked.control));
                } else if (locked.allocation || !race_final_release) ++errors;
            }
        });
        begin.store(true, std::memory_order_release);
        if (race_final_release) kinoko_native_release_strong(address(&fixture));
        for (auto& worker : workers) worker.join();
        if (!race_final_release) {
            require(successes == 4096 && fixture.record.strong == 1 && fixture.disposes == 0,
                    "balanced strong refs under contention with a surviving strong owner");
            kinoko_native_release_strong(address(&fixture));
        }
        require(errors == 0 && fixture.record.strong == 0 && fixture.record.weak == 1 &&
                fixture.disposes == 1 && fixture.destroys == 0, "race releases exactly once, weak owner retains control");
        ReferenceRecord expired{1, 1};
        kinoko_native_weak_pair_lock(address(&weak), reinterpret_cast<int32_t*>(&expired));
        require(expired.allocation == 0 && expired.control == 0, "post-race lock fails without resurrection");
        kinoko_native_release_weak(address(&fixture));
        require(fixture.destroys == 1 && fixture.ordering_errors == 0, "post-race destroy order");
    }
}
}
extern "C" int32_t kinoko_actor_control_vtable(void) { return address(&known_table_tag); }

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
