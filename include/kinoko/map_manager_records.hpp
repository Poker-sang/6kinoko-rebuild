#pragma once
#include "kinoko/map_manager.h"
#include "kinoko/camera_records.hpp"
#include "kinoko/native_record_view.hpp"
#include <array>
namespace kinoko::map {
struct Containers;
// Recovered 84-byte prefix. The host reserves more storage; that is not the
// original class size. Original STL bytes 24..51 now hold one native owner.
struct ManagerRecord {
    std::array<unsigned char, 12> script_object;
    KinokoActDocument *source_act;
    KinokoActSourceHolder *source_holder;
    KinokoActRuntime *player;
    Containers *containers;
    std::array<unsigned char, 24> old_container_storage;
    int32_t unknown52;
    int32_t last_id;
    camera::Bounds last_bounds;
    int32_t width, height;
};
using ManagerView = native::RecordView<ManagerRecord>;
static_assert(sizeof(ManagerRecord) == 84);
static_assert(offsetof(ManagerRecord, source_act) == 12);
static_assert(offsetof(ManagerRecord, source_holder) == 16);
static_assert(offsetof(ManagerRecord, player) == 20);
static_assert(offsetof(ManagerRecord, containers) == 24);
static_assert(offsetof(ManagerRecord, unknown52) == 52);
static_assert(offsetof(ManagerRecord, last_id) == 56);
static_assert(offsetof(ManagerRecord, last_bounds) == 60);
static_assert(offsetof(ManagerRecord, width) == 76);
static_assert(offsetof(ManagerRecord, height) == 80);
}
