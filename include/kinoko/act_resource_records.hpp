#pragma once
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstdint>
#include <windows.h>

namespace kinoko::act {
using Address = uint32_t;
struct VectorStorage { Address begin, end, capacity; };
// Representation only: the actual Squirrel API owns the external reference.
using ObjectStorage = std::array<int32_t, 2>;
struct RuntimeRecord {
    Address source_holder; // borrowed holder of the source ACT
    int32_t current_time;
    uint8_t stage_active;
    std::array<uint8_t, 3> unknown9;
    Address act; // owned clone, or a legacy borrowed fixture
    Address owned_storage;
    CRITICAL_SECTION lock;
    VectorStorage draw_commands;
    uint32_t unknown56;
    VectorStorage draw_sprites;
    uint32_t unknown72, field76, unknown80;
    Address find_storage; // owned C++ find map, never an emulated STL tree
    uint32_t find_count, unknown92, next_find_id, wake_time;
    uint8_t hidden;
    std::array<uint8_t, 3> unknown105;
    std::array<uint32_t, 11> stage_state;
    Address vm; // borrowed VM, owns environment through an external reference
    ObjectStorage environment;
    std::array<uint8_t, 16> name_storage;
    uint32_t name_length, name_capacity, unknown188;
};
static_assert(sizeof(void*) == 4 && sizeof(CRITICAL_SECTION) == 24);
static_assert(sizeof(RuntimeRecord) == 192);
#define KINOKO_ACT_FIELD(T, M, O) static_assert(offsetof(T, M) == O)
KINOKO_ACT_FIELD(RuntimeRecord, act, 12);
KINOKO_ACT_FIELD(RuntimeRecord, lock, 20);
KINOKO_ACT_FIELD(RuntimeRecord, draw_commands, 44);
KINOKO_ACT_FIELD(RuntimeRecord, draw_sprites, 60);
KINOKO_ACT_FIELD(RuntimeRecord, find_storage, 84);
KINOKO_ACT_FIELD(RuntimeRecord, next_find_id, 96);
KINOKO_ACT_FIELD(RuntimeRecord, stage_state, 108);
KINOKO_ACT_FIELD(RuntimeRecord, vm, 152);
KINOKO_ACT_FIELD(RuntimeRecord, environment, 156);
KINOKO_ACT_FIELD(RuntimeRecord, name_storage, 164);
KINOKO_ACT_FIELD(RuntimeRecord, name_length, 180);
KINOKO_ACT_FIELD(RuntimeRecord, name_capacity, 184);
#undef KINOKO_ACT_FIELD
}
