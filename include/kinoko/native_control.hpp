#pragma once
#include "kinoko/native_record_view.hpp"
#include "kinoko/native_control.h"
#include <cstddef>
#include <cstdint>

namespace kinoko::native {
// Layout schemas only. Count access is provided by the Win32 implementation;
// do not overlay std::shared_ptr or std::atomic on the existing C storage.
struct ControlRecord {
    const void* vtable;
    std::int32_t strong, weak;
    void* allocation;
};
struct ControlTable {
    const void *unknown_entry, *dispose, *destroy;
};
using ReferenceRecord = KinokoNativeReference;
static_assert(sizeof(ControlRecord) == 16 && alignof(ControlRecord) == 4);
static_assert(offsetof(ControlRecord, strong) == 4 && offsetof(ControlRecord, weak) == 8);
static_assert(offsetof(ControlRecord, allocation) == 12);
static_assert(sizeof(ControlTable) == 12 && sizeof(ReferenceRecord) == 8);
}
