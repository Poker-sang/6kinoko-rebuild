#pragma once
#include "kinoko/act_types.h"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstdint>

// 466100 publishes three independently allocated objects; 465F70 destroys
// holder, document (virtual deleting destructor), runtime, then this record.
// A schema only, never a C++ object overlaid on a C allocation/test fixture.
struct KinokoStageOwner {
    KinokoActDocument *document;
    KinokoActSourceHolder *holder;
    KinokoActRuntime *runtime;
};
struct KinokoActSourceHolder { KinokoActDocument *document; };
namespace kinoko::stage {
using OwnerRecord = KinokoStageOwner;
using SourceHolderRecord = KinokoActSourceHolder;
using DeleteDocument = int32_t (__thiscall *)(KinokoActDocument *, int32_t flags);
struct DocumentVirtuals {
    std::array<void *, 4> preceding_methods;
    DeleteDocument deleting_destructor;
};
struct DocumentPrefix { const DocumentVirtuals *vtable; };
using OwnerView = kinoko::native::RecordView<OwnerRecord>;
static_assert(sizeof(OwnerRecord) == 12);
static_assert(offsetof(OwnerRecord, holder) == 4);
static_assert(offsetof(OwnerRecord, runtime) == 8);
static_assert(sizeof(SourceHolderRecord) == 4);
static_assert(offsetof(DocumentVirtuals, deleting_destructor) == 16);
}
