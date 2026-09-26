#pragma once
#include "kinoko/act_document_records.hpp"
#include <map>
#include <optional>

namespace kinoko::act {
// Prefix of the existing 348-byte layer, never constructed over game storage.
struct LayerPropertyAliases {
    float *rotation_x, *rotation_y, *rotation_z;
    float *rotation_pivot_x, *rotation_pivot_y, *rotation_pivot_z;
    float *scale_x, *scale_y, *scale_z;
    float *scale_pivot_x, *scale_pivot_y, *scale_pivot_z;
    float *alpha;
    int32_t *blend, *red, *green, *blue;
};
static_assert(sizeof(LayerPropertyAliases) == 68);
struct LayerAssociationRecord {
    const void *vtable;
    LayerPropertyAliases property_aliases;
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

// 427950 uses cloned IDs to replace the shallow hierarchy references. These
// indices borrow objects owned by Clone; they never detach or append children.
class DocumentCloneAssociations {
public:
    void add_layer(KinokoActLayer *layer);
    void add_resource(KinokoActResource *resource);
    void bind(KinokoActDocument *document);
private:
    std::map<int32_t, KinokoActResource *> resources_;
    std::map<int32_t, KinokoActLayer *> layers_;
};
}
