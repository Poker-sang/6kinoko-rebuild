#pragma once
#include "kinoko/act_types.h"
#include "kinoko/act_array.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace kinoko::act {
// Representation schemas only, never constructed over legacy/test storage.
// Layer/resource implementations are still migrated independently.
struct LayerRecord;
struct ResourceRecord;
// act_array.cpp publishes begin/end views and owns a native vector in word 3.
// The last word is NOT an end-of-capacity pointer from the original VC8 vector.
template<class T> struct DocumentPointerSpan {
    T **begin, **end;
    kinoko::ActArray *storage;
};
struct DocumentRecord {
    const void *vtable;
    int32_t resolution_ms;
    int32_t screen_width, screen_height;
    kinoko::legacy::StringRecord name;
    uint32_t unknown40;
    kinoko::legacy::StringRecord resource_path; // 4289C0 stores its path here
    uint32_t unknown68;
    int32_t margin_left, margin_top, margin_right, margin_bottom;
    float offset_x, offset_y;
    uint8_t visible;
    std::array<uint8_t, 3> padding97;
    // CActScript owns its callbacks, string and buffer. Keep using its existing
    // source-backed constructor/destructor rather than resetting VM objects.
    std::array<uint8_t, 104> script;
    uint8_t resources_suspended; // 428AF0/428BD0
    std::array<uint8_t, 3> padding205;
    DocumentPointerSpan<LayerRecord> layers;
    uint32_t unknown220;
    DocumentPointerSpan<ResourceRecord> resources;
    uint32_t unknown236;
};
using DocumentView = kinoko::native::RecordView<DocumentRecord>;
static_assert(sizeof(void *) == 4, "CAct is a Win32 record");
static_assert(sizeof(DocumentRecord) == 240);
static_assert(sizeof(DocumentPointerSpan<LayerRecord>) == 12);
static_assert(offsetof(DocumentPointerSpan<LayerRecord>, storage) == 8);
#define KINOKO_DOCUMENT_FIELD(M, O) static_assert(offsetof(DocumentRecord, M) == O)
KINOKO_DOCUMENT_FIELD(vtable, 0);
KINOKO_DOCUMENT_FIELD(resolution_ms, 4);
KINOKO_DOCUMENT_FIELD(screen_width, 8);
KINOKO_DOCUMENT_FIELD(screen_height, 12);
KINOKO_DOCUMENT_FIELD(name, 16);
KINOKO_DOCUMENT_FIELD(unknown40, 40);
KINOKO_DOCUMENT_FIELD(resource_path, 44);
KINOKO_DOCUMENT_FIELD(unknown68, 68);
KINOKO_DOCUMENT_FIELD(margin_left, 72);
KINOKO_DOCUMENT_FIELD(margin_top, 76);
KINOKO_DOCUMENT_FIELD(margin_right, 80);
KINOKO_DOCUMENT_FIELD(margin_bottom, 84);
KINOKO_DOCUMENT_FIELD(offset_x, 88);
KINOKO_DOCUMENT_FIELD(offset_y, 92);
KINOKO_DOCUMENT_FIELD(visible, 96);
KINOKO_DOCUMENT_FIELD(script, 100);
KINOKO_DOCUMENT_FIELD(resources_suspended, 204);
KINOKO_DOCUMENT_FIELD(layers, 208);
KINOKO_DOCUMENT_FIELD(unknown220, 220);
KINOKO_DOCUMENT_FIELD(resources, 224);
KINOKO_DOCUMENT_FIELD(unknown236, 236);
#undef KINOKO_DOCUMENT_FIELD
}
