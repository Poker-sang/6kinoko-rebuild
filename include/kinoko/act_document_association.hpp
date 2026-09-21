#pragma once
#include "kinoko/act_document_records.hpp"
#include <map>
#include <optional>

namespace kinoko::act {
// Prefix of the existing 348-byte layer, never constructed over game storage.
struct LayerAssociationRecord {
    const void *vtable;
    std::array<unsigned char, 68> property_aliases;
    DocumentPointerSpan<KinokoActLayer> children;
    uint32_t unknown84;
    KinokoActLayer *parent;
    std::array<unsigned char, 4> flags92;
    int32_t resource_id;
    KinokoActResource *resource;
    int32_t layer_id, parent_id;
};
struct ResourceIdentityRecord { const void *vtable; int32_t id; };
static_assert(sizeof(LayerAssociationRecord) == 112);
static_assert(offsetof(LayerAssociationRecord, children) == 72);
static_assert(offsetof(LayerAssociationRecord, parent) == 88);
static_assert(offsetof(LayerAssociationRecord, resource_id) == 96);
static_assert(offsetof(LayerAssociationRecord, resource) == 100);
static_assert(offsetof(LayerAssociationRecord, layer_id) == 104);
static_assert(offsetof(LayerAssociationRecord, parent_id) == 108);
static_assert(offsetof(ResourceIdentityRecord, id) == 4);

// 428150 owns these borrowed indices only during deserialization. Insertion
// keeps the first duplicate ID (42A310); the document owns every parsed object.
class DocumentLoadAssociations {
public:
    void add_layer(KinokoActLayer *layer);
    void begin_resources();
    void add_resource(KinokoActResource *resource);
    // Called on freshly constructed layers, before reading the resource count.
    void bind_loaded_parents(KinokoActDocument *document, uint32_t count);
    // Only SetResource virtual calls; never traverses or rebinds key layouts.
    void bind_resources(KinokoActDocument *document, uint32_t count) const;
private:
    std::map<int32_t, KinokoActLayer *> layers_;
    // Original resource index is constructed after the parent pass/count read.
    std::optional<std::map<int32_t, KinokoActResource *>> resources_;
};
}
