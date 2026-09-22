#pragma once
#include "kinoko/native_record_view.hpp"
#include "kinoko/act_types.h"
#include "kinoko/legacy_string.hpp"

#include <array>
#include <cstdint>
#include <windows.h>

struct SQVM;

namespace kinoko::act {
using Address = uint32_t;
struct FindState; // owned native implementation, defined in act_runtime_lifecycle.cpp
struct VectorStorage { Address begin, end, capacity; };
// Representation only: the actual Squirrel API owns the external reference.
using ObjectStorage = std::array<int32_t, 2>;
struct RuntimeRecord {
    KinokoActSourceHolder *source_holder; // borrowed; never owns the holder or source ACT
    int32_t current_time;
    uint8_t stage_active;
    std::array<uint8_t, 3> unknown9;
    KinokoActDocument *active_document; // owned clone; destructor never consults source_holder
    KinokoActSourceHolder *active_holder; // owned wrapper; borrows active_document
    CRITICAL_SECTION lock;
    VectorStorage draw_commands;
    uint32_t unknown56;
    VectorStorage draw_sprites;
    uint32_t unknown72;
    Address render_target; // borrowed CActRenderTarget, texture handle at +68
    uint32_t unknown80;
    FindState *find_state; // owned C++ find map, never an emulated STL tree
    uint32_t find_count, unknown92, next_find_id, wake_time;
    uint8_t hidden;
    std::array<uint8_t, 3> unknown105;
    std::array<uint32_t, 11> stage_state;
    SQVM *vm; // borrowed VM, owns environment through an external reference
    ObjectStorage environment;
    kinoko::legacy::StringRecord name;
    uint32_t unknown188;
};
static_assert(sizeof(void*) == 4 && sizeof(CRITICAL_SECTION) == 24);
static_assert(sizeof(RuntimeRecord) == 192);
#define KINOKO_ACT_FIELD(T, M, O) static_assert(offsetof(T, M) == O)
KINOKO_ACT_FIELD(RuntimeRecord, source_holder, 0);
KINOKO_ACT_FIELD(RuntimeRecord, active_document, 12);
KINOKO_ACT_FIELD(RuntimeRecord, active_holder, 16);
KINOKO_ACT_FIELD(RuntimeRecord, lock, 20);
KINOKO_ACT_FIELD(RuntimeRecord, draw_commands, 44);
KINOKO_ACT_FIELD(RuntimeRecord, draw_sprites, 60);
KINOKO_ACT_FIELD(RuntimeRecord, render_target, 76);
KINOKO_ACT_FIELD(RuntimeRecord, find_state, 84);
KINOKO_ACT_FIELD(RuntimeRecord, next_find_id, 96);
KINOKO_ACT_FIELD(RuntimeRecord, stage_state, 108);
KINOKO_ACT_FIELD(RuntimeRecord, vm, 152);
KINOKO_ACT_FIELD(RuntimeRecord, environment, 156);
KINOKO_ACT_FIELD(RuntimeRecord, name, 164);
static_assert(offsetof(RuntimeRecord, name) + offsetof(kinoko::legacy::StringRecord, length) == 180);
static_assert(offsetof(RuntimeRecord, name) + offsetof(kinoko::legacy::StringRecord, capacity) == 184);
#undef KINOKO_ACT_FIELD
}
