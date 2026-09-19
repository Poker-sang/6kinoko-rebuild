#include "kinoko/actor_records.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace kinoko::actor;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "record line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main() {
    // Every byte alignment is supported. These are byte allocations, not live
    // ActorRecord objects; the view must not acquire alignment/lifetime UB.
    for (std::size_t shift = 1; shift <= 16; ++shift) {
        std::array<unsigned char, sizeof(ActorRecord) + 32> storage;
        storage.fill(0xa7);
        ActorView view(storage.data() + shift);
        view.clear();
        view.set(&ActorRecord::manager, Address{0xfedcba98});
        view.set(&ActorRecord::x, -123.5f);
        const auto initial = view.view(&ActorRecord::initial);
        initial.set(&InitialData::chip_flags, std::int64_t{-42});
        initial.set(&InitialData::chip_bound_type, std::uint16_t{0xffff});
        CHECK(view.get(&ActorRecord::manager) == 0xfedcba98);
        CHECK(view.get(&ActorRecord::x) == -123.5f);
        CHECK(initial.get(&InitialData::chip_flags) == -42);
        CHECK(initial.get(&InitialData::chip_bound_type) == 0xffff);
        CHECK(view.bytes(&ActorRecord::initial) == storage.data() + shift + 376);
        CHECK(initial.bytes(&InitialData::chip_flags) == storage.data() + shift + 392);
        CHECK(initial.bytes(&InitialData::chip_bound_type) == storage.data() + shift + 410);
        const std::array<std::size_t, 7> expected{44, 56, 68, 96, 108, 124, 136};
        for (std::size_t index = 0; index < script_members.size(); ++index) {
            CHECK(view.bytes(script_members[index]) == storage.data() + shift + expected[index]);
            ScriptStorage object{}; object.fill(static_cast<unsigned char>(index + 1));
            view.set(script_members[index], object);
            CHECK(view.get(script_members[index]) == object);
        }
        for (std::size_t index = 0; index < shift; ++index) CHECK(storage[index] == 0xa7);
        for (std::size_t index = shift + sizeof(ActorRecord); index < storage.size(); ++index)
            CHECK(storage[index] == 0xa7);
        const auto copy = view.load();
        CHECK(copy.manager == 0xfedcba98 && copy.initial.chip_flags == -42);
        CHECK(copy.x == -123.5f);
    }
    std::array<unsigned char, sizeof(ManagerPrefix) + 2> manager{};
    const ManagerView view(manager.data() + 1);
    const auto index = view.view(&ManagerPrefix::actors);
    index.set(&TreeIndex::head, Address{0x87654321});
    index.set(&TreeIndex::count, std::int32_t{123});
    CHECK(view.bytes(&ManagerPrefix::actors) == manager.data() + 85);
    CHECK(index.bytes(&TreeIndex::head) == manager.data() + 89);
    CHECK(index.get(&TreeIndex::count) == 123);
    std::puts("PASS: shared Actor/manager schemas, typed members, high-bit addresses and 16 unaligned byte views");
}
