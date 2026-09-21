#include "kinoko/act_document_association.hpp"
#include "kinoko/act_layer_access.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_array.h"
#include "kinoko/legacy_memory.hpp"

namespace kinoko::act {
using LayerView = kinoko::native::RecordView<LayerAssociationRecord>;
using kinoko::legacy::load;
using kinoko::legacy::address;

void DocumentLoadAssociations::add_layer(KinokoActLayer *layer) {
    layers_.emplace(LayerView(layer).get(&LayerAssociationRecord::layer_id), layer);
}
void DocumentLoadAssociations::add_resource(KinokoActResource *resource) {
    resources_->emplace(load<ResourceIdentityRecord>(resource).id, resource);
}
void DocumentLoadAssociations::begin_resources() {
    resources_.emplace();
}
void DocumentLoadAssociations::bind_loaded_parents(KinokoActDocument *document, uint32_t count) {
    for (uint32_t i = 0; i != count; ++i) {
        auto *layer = layer_at(DocumentView(document).get(&DocumentRecord::layers), i);
        const LayerView child(layer);
        const auto parent_id = child.get(&LayerAssociationRecord::parent_id);
        if (parent_id < 0) continue;
        // 4296A0 is operator[]: a missing parent ID inserts a null value.
        auto *parent = layers_[parent_id];
        // 455FD0's old-parent unlink is inapplicable to these freshly created
        // layers (+88 was initialized to zero). Do not apply this API to clones.
        child.set(&LayerAssociationRecord::parent, static_cast<KinokoActLayer *>(nullptr));
        child.set(&LayerAssociationRecord::parent_id, int32_t{-1});
        if (parent) {
            const LayerView parent_view(parent);
            kinoko_act_array_append(address(parent_view.bytes(&LayerAssociationRecord::children)), address(layer));
            child.set(&LayerAssociationRecord::parent_id, parent_view.get(&LayerAssociationRecord::layer_id));
        }
        child.set(&LayerAssociationRecord::parent, parent);
    }
}
void DocumentLoadAssociations::bind_resources(KinokoActDocument *document, uint32_t count) const {
    using SetResource = int32_t (__thiscall *)(KinokoActLayer *, KinokoActResource *);
    for (uint32_t i = 0; i != count; ++i) {
        auto *layer = layer_at(DocumentView(document).get(&DocumentRecord::layers), i);
        const LayerView view(layer);
        const auto found = resources_->find(view.get(&LayerAssociationRecord::resource_id));
        if (found == resources_->end()) continue; // original leaves fields untouched
        const auto *table = static_cast<const unsigned char *>(view.get(&LayerAssociationRecord::vtable));
        load<SetResource>(table + 6 * sizeof(void *))(layer, found->second);
    }
}
}

extern "C" int32_t __fastcall kinoko_act_layer_set_resource(
    KinokoActLayer *layer, void *, KinokoActResource *resource) {
    if (!resource) return static_cast<int32_t>(0x80004005u);
    const kinoko::act::LayerView view(layer);
    view.set(&kinoko::act::LayerAssociationRecord::resource, resource);
    view.set(&kinoko::act::LayerAssociationRecord::resource_id,
        kinoko::legacy::load<kinoko::act::ResourceIdentityRecord>(resource).id);
    return 0;
}
