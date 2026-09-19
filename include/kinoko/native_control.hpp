#pragma once
#include "kinoko/native_record_view.hpp"
#include <cstddef>
#include <cstdint>

namespace kinoko::native {
// Layout schemas only. Count access is provided by the Win32 implementation;
// do not overlay std::shared_ptr or std::atomic on the existing C storage.
struct ControlRecord {
    std::uint32_t vtable;
    std::int32_t strong, weak;
    std::uint32_t allocation;
};
struct ControlTable {
    std::uint32_t unknown_entry, dispose, destroy;
};
struct ReferenceRecord {
    std::uint32_t allocation, control;
};
static_assert(sizeof(ControlRecord) == 16 && alignof(ControlRecord) == 4);
static_assert(offsetof(ControlRecord, strong) == 4 && offsetof(ControlRecord, weak) == 8);
static_assert(offsetof(ControlRecord, allocation) == 12);
static_assert(sizeof(ControlTable) == 12 && sizeof(ReferenceRecord) == 8);
}
