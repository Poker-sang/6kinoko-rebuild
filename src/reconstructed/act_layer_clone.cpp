#include "kinoko/act_array.h"
#include "kinoko/act_runtime.h"
#include "kinoko/act_script_text.hpp"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_layer_lifecycle.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/upstream_bindings.hpp"
#include <memory>
#include <unordered_set>

extern "C" int32_t g664;

namespace {
using kinoko::legacy::address;
using kinoko::legacy::pointer;
inline int32_t& act_layer_vm_slot = g664;
using kinoko::legacy::field;
namespace sqrat = kinoko::script::upstream;

struct LayerDelete {
    void operator()(unsigned char* layer) const {
        kinoko_act_layer_clear(reinterpret_cast<KinokoActLayer*>(layer));
        std::free(layer);
    }
};
struct KeyDelete {
    void operator()(unsigned char* key) const { retdec_destroy_cact_key(address(key)); }
};

void assign_object(kinoko::act::LayerObjectView destination, kinoko::act::LayerObjectView source) {
    using kinoko::act::LayerObjectRecord;
    const auto old_vm = destination.get(&LayerObjectRecord::vm);
    const auto old_value = kinoko::legacy::load<HSQOBJECT>(destination.bytes(&LayerObjectRecord::value));
    if (old_vm) sqrat::sqrat_destroy_object(old_vm, old_value,
        destination.get(&LayerObjectRecord::owns_reference) != 0);
    // 41ECA0 copies exactly VM, pair and ownership (13 bytes); neither
    // destination vtable nor trailing padding participates in the copy.
    destination.set(&LayerObjectRecord::vm, source.get(&LayerObjectRecord::vm));
    destination.set(&LayerObjectRecord::value, source.get(&LayerObjectRecord::value));
    destination.set(&LayerObjectRecord::owns_reference, source.get(&LayerObjectRecord::owns_reference));
    if (auto* vm = destination.get(&LayerObjectRecord::vm))
        sqrat::sqrat_retain(vm, kinoko::legacy::load<HSQOBJECT>(destination.bytes(&LayerObjectRecord::value)));
}

void clone_list(int32_t destination, int32_t source, int32_t offset, bool bind) {
    const auto head=field<int32_t>(source+offset);
    if (!head) return;
    std::unordered_set<int32_t> visited;
    for (auto node=field<int32_t>(head);node!=head;node=field<int32_t>(node)) {
        if (!node || visited.size()>=0x10000 || !visited.insert(node).second) throw std::bad_alloc();
        const auto original=field<int32_t>(node+8);
        if (!original) throw std::bad_alloc();
        const auto copy=retdec_call_thiscall0_result(pointer<void>(original),
            field<void*>(field<int32_t>(original)+20));
        std::unique_ptr<unsigned char,KeyDelete> owner(pointer<unsigned char>(copy));
        if (!copy || !retdec_act_append_list(destination+offset,copy)) throw std::bad_alloc();
        owner.release();
        ++field<int32_t>(destination+offset+4);
        const auto layout=field<int32_t>(copy+4);
        if (bind && layout) retdec_call_thiscall1_result(pointer<void>(layout),
            field<void*>(field<int32_t>(layout)+24),destination);
    }
}
}

// Original 41EA50 and its 41ECA0 assignment: constructor-owned containers,
// shallow hierarchy links, deep keys, and source-backed Sqrat reference copies.
extern "C" int32_t __fastcall kinoko_method_clone_act_layer(int32_t source, void*) {
    if (!source) return 0;
    try {
        kinoko::legacy::Allocation<unsigned char> storage(static_cast<unsigned char*>(std::calloc(1,sizeof(kinoko::act::LayerStorageRecord))));
        if (!storage || !kinoko_act_layer_initialize(reinterpret_cast<KinokoActLayer*>(storage.get()),pointer<SQVM>(act_layer_vm_slot))) return 0;
        std::unique_ptr<unsigned char,LayerDelete> owned(storage.release());
        const auto result=address(owned.get());
        using namespace kinoko::act;
        const LayerStorageView destination(owned.get()), original(pointer<void>(source));
        const auto target_links = destination.view(&LayerStorageRecord::association);
        const auto source_links = original.view(&LayerStorageRecord::association);
        target_links.set(&LayerAssociationRecord::property_aliases, source_links.get(&LayerAssociationRecord::property_aliases));
        kinoko_act_array_clone(address(target_links.bytes(&LayerAssociationRecord::children)),
                               address(source_links.bytes(&LayerAssociationRecord::children)));
        target_links.set(&LayerAssociationRecord::parent, source_links.get(&LayerAssociationRecord::parent));
        *target_links.bytes(&LayerAssociationRecord::flags92) =
            *source_links.bytes(&LayerAssociationRecord::flags92);
        target_links.set(&LayerAssociationRecord::resource_id, source_links.get(&LayerAssociationRecord::resource_id));
        target_links.set(&LayerAssociationRecord::resource, source_links.get(&LayerAssociationRecord::resource));
        target_links.set(&LayerAssociationRecord::layer_id, source_links.get(&LayerAssociationRecord::layer_id));
        target_links.set(&LayerAssociationRecord::parent_id, source_links.get(&LayerAssociationRecord::parent_id));
        const kinoko::legacy::StringView input(original.bytes(&LayerStorageRecord::name)),
                                         output(destination.bytes(&LayerStorageRecord::name));
        output.assign(input.data(),input.length());
        if (output.length()!=input.length()) return 0;
        destination.set(&LayerStorageRecord::visibility_flags, original.get(&LayerStorageRecord::visibility_flags));
        destination.set(&LayerStorageRecord::position, original.get(&LayerStorageRecord::position));
        destination.set(&LayerStorageRecord::unknown156, original.get(&LayerStorageRecord::unknown156));
        destination.set(&LayerStorageRecord::previous_position, original.get(&LayerStorageRecord::previous_position));
        const ScriptTextView target_script(destination.bytes(&LayerStorageRecord::script));
        const ScriptTextView source_script(original.bytes(&LayerStorageRecord::script));
        assign_script_payload(target_script, source_script);
        assign_object(destination.view(&LayerStorageRecord::script_object), original.view(&LayerStorageRecord::script_object));
        assign_object(destination.view(&LayerStorageRecord::layout_object), original.view(&LayerStorageRecord::layout_object));
        copy_script_text(target_script, source_script);
        clone_list(result,source,offsetof(LayerStorageRecord, keys),true);
        clone_list(result,source,offsetof(LayerStorageRecord, timelines),false);
        return address(owned.release());
    } catch (...) { return 0; }
}
