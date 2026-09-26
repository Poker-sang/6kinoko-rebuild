#include "kinoko/act_layer_lifecycle.h"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/act_list.h"
#include "kinoko/act_array.h"
#include "kinoko/sqrat_object_bridge.h"
#include <squirrel.h>
#include <cstdlib>
#include <cstring>
#include <memory>

using kinoko::legacy::address;
using kinoko::legacy::pointer;

extern "C" KinokoActLayer* kinoko_act_layer_initialize(KinokoActLayer* layer, SQVM* vm) {
    using namespace kinoko::act;
    if (!layer) return nullptr;
    const LayerStorageView record(layer);
    const auto association = record.view(&LayerStorageRecord::association);
    association.set(&LayerAssociationRecord::vtable, kinoko_act_host_symbols()->layer_vtable);
    association.set(&LayerAssociationRecord::children, DocumentPointerSpan<KinokoActLayer>{});
    association.set(&LayerAssociationRecord::parent, static_cast<KinokoActLayer*>(nullptr));
    association.set(&LayerAssociationRecord::resource_id, int32_t{-1});
    association.set(&LayerAssociationRecord::resource, static_cast<KinokoActResource*>(nullptr));
    association.set(&LayerAssociationRecord::layer_id, int32_t{-1});
    association.set(&LayerAssociationRecord::parent_id, int32_t{-1});
    const auto name = record.view(&LayerStorageRecord::name);
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, kinoko::legacy::StringView::inline_capacity);
    *name.bytes(&kinoko::legacy::StringRecord::characters) = 0;
    kinoko_string_assign_cstr(reinterpret_cast<int32_t*>(record.bytes(&LayerStorageRecord::name)), "Layer_");
    record.set(&LayerStorageRecord::visibility_flags, uint16_t{1});
    *association.bytes(&LayerAssociationRecord::flags92) = 1;
    record.set(&LayerStorageRecord::position, std::array<uint32_t, 3>{});
    record.set(&LayerStorageRecord::origin_bits, std::array<uint8_t, 12>{});
    record.set(&LayerStorageRecord::previous_position, std::array<uint32_t, 3>{});
    const auto keys = record.view(&LayerStorageRecord::keys);
    const auto timelines = record.view(&LayerStorageRecord::timelines);
    keys.set(&LayerListRecord::head, static_cast<KeyNode*>(nullptr));
    keys.set(&LayerListRecord::count, int32_t{0});
    timelines.set(&LayerListRecord::head, static_cast<KeyNode*>(nullptr));
    timelines.set(&LayerListRecord::count, int32_t{0});
    if (!kinoko_act_make_list(reinterpret_cast<int32_t*>(keys.bytes(&LayerListRecord::head))) ||
        !kinoko_act_make_list(reinterpret_cast<int32_t*>(timelines.bytes(&LayerListRecord::head)))) {
        kinoko_act_list_drop_storage(address(keys.get(&LayerListRecord::head)));
        kinoko_act_list_drop_storage(address(timelines.get(&LayerListRecord::head)));
        keys.set(&LayerListRecord::head, static_cast<KeyNode*>(nullptr));
        timelines.set(&LayerListRecord::head, static_cast<KeyNode*>(nullptr));
        kinoko_string_destroy(record.bytes(&LayerStorageRecord::name));
        return nullptr;
    }
    const auto script = record.view(&LayerStorageRecord::script);
    kinoko_construct_cact_script(address(script.data()));
    for (const auto member : {&ScriptStorageRecord::initialize, &ScriptStorageRecord::update,
                              &ScriptStorageRecord::release})
        script.view(member).set(&ActCallbackRecord::vm, vm);
    auto initialize_object = [vm](LayerObjectView object, const void* methods) {
        object.set(&LayerObjectRecord::methods, methods);
        object.set(&LayerObjectRecord::vm, vm);
        object.set(&LayerObjectRecord::owns_reference, uint8_t{1});
        HSQOBJECT empty;
        sq_resetobject(&empty);
        std::memcpy(object.bytes(&LayerObjectRecord::value), &empty, sizeof(empty));
    };
    const auto script_object = record.view(&LayerStorageRecord::script_object);
    initialize_object(script_object, kinoko_act_host_symbols()->layer_ref_vtable);
    initialize_object(record.view(&LayerStorageRecord::layout_object),
                      kinoko_act_host_symbols()->layer_layout_vtable);
    if (vm && !kinoko_sqrat_new_table(vm,
            reinterpret_cast<int32_t*>(script_object.bytes(&LayerObjectRecord::value)))) {
        kinoko_destroy_cact_layer(address(layer));
        return nullptr;
    }
    association.set(&LayerAssociationRecord::property_aliases, LayerPropertyAliases{});
    return layer;
}

int32_t kinoko_construct_cact_layer(int32_t layer, int32_t vm) {
    return address(kinoko_act_layer_initialize(pointer<KinokoActLayer>(layer), pointer<SQVM>(vm)));
}

int32_t kinoko_act_make_layer(void) {
    // Parent document owns the allocation; containers and script references
    // are initialized by the same constructor used by cloning/publication.
    auto layer = std::unique_ptr<KinokoActLayer, decltype(&std::free)>(
        static_cast<KinokoActLayer*>(std::calloc(1u, sizeof(kinoko::act::LayerStorageRecord))), &std::free);
    // Archive parsing precedes VM creation. Publication creates the script
    // table later; original native callers supply g664 at construction.
    if (!layer || !kinoko_act_layer_initialize(layer.get(), nullptr)) return 0;
    return address(layer.release());
}

extern "C" void kinoko_act_layer_clear(KinokoActLayer* layer)
{
    using namespace kinoko::act;
    if (!layer) return;
    const LayerStorageView record(layer);
    const auto association = record.view(&LayerStorageRecord::association);
    association.set(&LayerAssociationRecord::vtable, kinoko_act_host_symbols()->layer_vtable);
    // 41E5C0: owned keys/timelines go first, while both lists and VM wrappers
    // still exist. Their node storage is released after the embedded script.
    const auto keys = record.view(&LayerStorageRecord::keys);
    const auto timelines = record.view(&LayerStorageRecord::timelines);
    kinoko_act_list_dispose_payloads(address(keys.get(&LayerListRecord::head)));
    kinoko_act_list_dispose_payloads(address(timelines.get(&LayerListRecord::head)));
    // Layout object before script object; only owned pairs are reset/released.
    for (const auto member : {&LayerStorageRecord::layout_object, &LayerStorageRecord::script_object}) {
        const auto object = record.view(member);
        if (!object.get(&LayerObjectRecord::owns_reference)) continue;
        if (auto* vm = object.get(&LayerObjectRecord::vm))
            kinoko_sqrat_release_pair(vm, reinterpret_cast<int32_t*>(object.bytes(&LayerObjectRecord::value)));
        HSQOBJECT empty;
        sq_resetobject(&empty);
        std::memcpy(object.bytes(&LayerObjectRecord::value), &empty, sizeof(empty));
        object.set(&LayerObjectRecord::owns_reference, uint8_t{0});
    }
    kinoko_destroy_cact_script(address(record.bytes(&LayerStorageRecord::script)));
    kinoko_act_list_drop_storage(address(timelines.get(&LayerListRecord::head)));
    timelines.set(&LayerListRecord::head, static_cast<KeyNode *>(nullptr));
    kinoko_act_list_drop_storage(address(keys.get(&LayerListRecord::head)));
    keys.set(&LayerListRecord::head, static_cast<KeyNode *>(nullptr));
    keys.set(&LayerListRecord::count, int32_t{0});
    timelines.set(&LayerListRecord::count, int32_t{0});
    kinoko_string_destroy(record.bytes(&LayerStorageRecord::name));
    const auto name = record.view(&LayerStorageRecord::name);
    const int32_t empty_word = 0;
    std::memcpy(name.bytes(&kinoko::legacy::StringRecord::characters), &empty_word, sizeof(empty_word));
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, kinoko::legacy::StringView::inline_capacity);
    kinoko_act_array_destroy(address(association.bytes(&LayerAssociationRecord::children)));
    association.set(&LayerAssociationRecord::children, DocumentPointerSpan<KinokoActLayer>{});
}

void kinoko_destroy_cact_layer(int32_t layer) {
    kinoko_act_layer_clear(pointer<KinokoActLayer>(layer));
}
