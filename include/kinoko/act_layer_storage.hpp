#pragma once
#include "kinoko/act_document_association.hpp"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_script_text.hpp"
#include "kinoko/act_script_payload.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

struct SQVM;
namespace kinoko::act {
// Host schemas, NOT on-disk ACT/CV4 structs. A callback contains two bare
// Sqrat pairs, not the three-word externally owned SqPlus argument record.
struct ActCallbackRecord {
    SQVM* vm;
    std::array<int32_t, 2> environment, closure;
};
struct ScriptStorageRecord {
    const void* methods;
    ActCallbackRecord initialize, update, release;
    legacy::StringRecord file_name;
    uint32_t unknown88;
    void* bytes;
    uint32_t size;
    uint8_t loaded, compiled;
    std::array<uint8_t, 2> padding102;
};
// Table/Instance owns an external reference only when owns_reference is set.
// Never copy its vtable or padding as part of reference assignment.
struct LayerObjectRecord {
    const void* methods;
    SQVM* vm;
    std::array<int32_t, 2> value;
    uint8_t owns_reference;
    std::array<uint8_t, 3> padding17;
};
struct LayerListRecord {
    KeyNode* head;
    int32_t count;
    uint32_t unknown8;
};
struct LayerStorageRecord {
    LayerAssociationRecord association;
    legacy::StringRecord name;
    uint32_t unknown136;
    // Low byte is visible, high byte is debugOnly. Preserve the original
    // two-byte copy in 41ECA0 and leave the following padding untouched.
    uint16_t visibility_flags;
    std::array<uint8_t, 2> padding142;
    std::array<uint32_t, 3> position;
    std::array<uint8_t, 12> unknown156;
    std::array<uint32_t, 3> previous_position;
    LayerListRecord keys, timelines;
    ScriptStorageRecord script;
    LayerObjectRecord script_object, layout_object;
};
using LayerStorageView = native::RecordView<LayerStorageRecord>;
using ScriptStorageView = native::RecordView<ScriptStorageRecord>;
using LayerObjectView = native::RecordView<LayerObjectRecord>;
using LayerListView = native::RecordView<LayerListRecord>;

static_assert(sizeof(void*) == 4, "Recovered ACT host storage is still Win32");
static_assert(sizeof(ActCallbackRecord) == 20);
static_assert(offsetof(ActCallbackRecord, environment) == 4);
static_assert(offsetof(ActCallbackRecord, closure) == 12);
static_assert(sizeof(ScriptStorageRecord) == sizeof(ScriptTextRecord));
static_assert(sizeof(ScriptStorageRecord) == sizeof(ScriptPayloadRecord));
static_assert(offsetof(ScriptStorageRecord, initialize) == 4);
static_assert(offsetof(ScriptStorageRecord, update) == 24);
static_assert(offsetof(ScriptStorageRecord, release) == 44);
static_assert(offsetof(ScriptStorageRecord, file_name) == 64);
static_assert(offsetof(ScriptStorageRecord, bytes) == 92);
static_assert(offsetof(ScriptStorageRecord, size) == 96);
static_assert(offsetof(ScriptStorageRecord, loaded) == 100);
static_assert(offsetof(ScriptStorageRecord, compiled) == 101);
static_assert(sizeof(LayerObjectRecord) == 20);
static_assert(offsetof(LayerObjectRecord, vm) == 4);
static_assert(offsetof(LayerObjectRecord, value) == 8);
static_assert(offsetof(LayerObjectRecord, owns_reference) == 16);
static_assert(sizeof(LayerListRecord) == 12);
static_assert(offsetof(LayerListRecord, count) == 4);
static_assert(sizeof(LayerStorageRecord) == 348);
#define KINOKO_LAYER_STORAGE_FIELD(M, O) static_assert(offsetof(LayerStorageRecord, M) == O)
KINOKO_LAYER_STORAGE_FIELD(association, 0);
KINOKO_LAYER_STORAGE_FIELD(name, 112);
KINOKO_LAYER_STORAGE_FIELD(visibility_flags, 140);
KINOKO_LAYER_STORAGE_FIELD(position, 144);
KINOKO_LAYER_STORAGE_FIELD(previous_position, 168);
KINOKO_LAYER_STORAGE_FIELD(keys, 180);
KINOKO_LAYER_STORAGE_FIELD(timelines, 192);
KINOKO_LAYER_STORAGE_FIELD(script, 204);
KINOKO_LAYER_STORAGE_FIELD(script_object, 308);
KINOKO_LAYER_STORAGE_FIELD(layout_object, 328);
#undef KINOKO_LAYER_STORAGE_FIELD
// Tie lifecycle storage to the older query prefixes; neither is a C++ object
// overlaid on the allocation. Allocation size must come from the full schema.
static_assert(offsetof(LayerStorageRecord, name) == offsetof(LayerKeys, name));
static_assert(offsetof(LayerStorageRecord, keys) == offsetof(LayerKeys, key_head));
static_assert(offsetof(LayerStorageRecord, timelines) == offsetof(LayerKeys, timeline_head));
static_assert(offsetof(LayerStorageRecord, script) + offsetof(ScriptStorageRecord, update)
              == offsetof(LayerKeys, update_callback));
} // namespace kinoko::act
