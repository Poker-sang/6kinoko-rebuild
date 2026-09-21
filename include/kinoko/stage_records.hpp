#pragma once
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstdint>

namespace kinoko::stage {
// 466100 publishes three independently allocated objects; 465F70 destroys
// holder, document (virtual deleting destructor), runtime, then this record.
// A schema only, never a C++ object overlaid on a C allocation/test fixture.
struct OwnerRecord {
    uint32_t document;
    uint32_t holder;
    uint32_t runtime;
};
struct SourceHolderRecord { uint32_t document; };
struct DocumentVirtuals {
    std::array<uint32_t, 4> preceding_methods;
    uint32_t deleting_destructor;
};
struct DocumentPrefix { uint32_t vtable; };
using OwnerView = kinoko::native::RecordView<OwnerRecord>;
static_assert(sizeof(OwnerRecord) == 12);
static_assert(offsetof(OwnerRecord, holder) == 4);
static_assert(offsetof(OwnerRecord, runtime) == 8);
static_assert(sizeof(SourceHolderRecord) == 4);
static_assert(offsetof(DocumentVirtuals, deleting_destructor) == 16);
}
