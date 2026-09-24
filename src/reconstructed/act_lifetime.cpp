#include "kinoko/legacy_string.h"
#include "kinoko/native_buffer.h"
#include "kinoko/act_array.h"
#include "kinoko/act_list.h"
#include "kinoko/map_render.h"
#include "kinoko/string_layout.h"
#include "kinoko/squirrel_api_types.h"
// Native C++ continuation of the recovered ACT path. Original function names
// remain C ABI ports until the surrounding decompiled host is migrated.
#include "kinoko/act_runtime.h"
#include "kinoko/act_script_payload.hpp"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_key_records.hpp"
#include "kinoko/act_mesh.hpp"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/native_property_bridge.h"
#include "kinoko/squirrel_binding.h"
#include "kinoko/squirrel_native_calls.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_legacy_api.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_compile_bridge.h"
#include "kinoko/squirrel_value_bridge.h"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/act_clone.h"
#include "kinoko/act_resource.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_object.h"
#include "kinoko/texture_store.h"
#include "kinoko/map_render.h"
#include "kinoko/sprite.h"
#include "kinoko/game_math.h"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::field;


extern "C" int32_t __fastcall kinoko_method_delete_act_script(int32_t script, void *) {
    if (script) {
        retdec_destroy_cact_script(script);
        std::free(pointer<void>(script));
    }
    return 0;
}

namespace {
void clear_layout(int32_t layout) {
    if (!layout) return;
    if (field<int32_t>(layout)==address(kinoko_string_layout_methods())) { kinoko_clear_string_layout(layout);return; }
    if (field<int32_t>(layout)==address(kinoko_act_host_symbols()->map_layout_vtable)) {
        kinoko_clear_map_layout(layout);
    }
}
void clear_key(int32_t value) {
    if (!value) return;
    field<int32_t>(value)=address(kinoko_act_host_symbols()->key_vtable);
    const auto layout=field<int32_t>(value+4);
    clear_layout(layout);
    std::free(pointer<void>(layout));
    field<int32_t>(value+4)=0;
    kinoko_string_destroy(kinoko::act::KeyView(pointer<void>(value)).bytes(&kinoko::act::KeyRecord::script_name));
    field<uint8_t>(value+8)=0;
    field<uint32_t>(value+24)=0;
    field<uint32_t>(value+28)=15;
}
}
void retdec_destroy_cact_key(int32_t value) {
    if (!value) return;
    // The layer's second list owns CActTimeLine, not a key with a layout.
    if (field<int32_t>(value)==address(kinoko_act_timeline_vtable()))
        kinoko_native_buffer_destroy(value+12);
    else clear_key(value);
    std::free(pointer<void>(value));
}
extern "C" int32_t __fastcall kinoko_method_destroy_layout(int32_t layout,void*) {
    clear_layout(layout);
    std::free(pointer<void>(layout));
    return layout;
}

void retdec_destroy_cact_list(int32_t *list_slot)
{
    int32_t sentinel;
    int32_t node;

    if (list_slot == nullptr || *list_slot == 0)
        return;
    sentinel = *list_slot;
    node = field<int32_t>(sentinel);
    while (node != 0 && node != sentinel) {
        int32_t next = field<int32_t>(node);
        retdec_destroy_cact_key(field<int32_t>(node + 8));
        node = next;
    }
    kinoko_act_list_drop_storage(sentinel);
    *list_slot = 0;
}


static void clear_resource(int32_t resource)
{
    if (resource == 0)
        return;
    if(field<const void*>(resource)==kinoko::mesh::resource_methods()) {
        kinoko::mesh::clear_resource(pointer<kinoko::mesh::Resource>(resource));return;
    }
    // Every resource owns the base name, including long names in native clones.
    kinoko_string_destroy(pointer<void>(resource + 8));
    field<int32_t>(resource + 8) = 0;
    field<uint32_t>(resource + 24) = 0;
    field<uint32_t>(resource + 28) = 15;
    if (field<int32_t>(resource) ==
            address(kinoko_act_host_symbols()->chip_resource_vtable)) {
        if (kinoko_act_release_chip_data(resource))
            retdec_mcd_free(pointer<retdec_mcd_data>(field<int32_t>(resource + 64)));
        field<int32_t>(resource + 64) = 0;
        kinoko_string_destroy(pointer<void>(resource + 36));
        field<int32_t>(resource + 36) = 0;
        field<int32_t>(resource + 52) = 0;
        field<int32_t>(resource + 56) = 15;
        kinoko_string_destroy(pointer<void>(resource + 72));
        field<int32_t>(resource + 72) = 0;
        field<int32_t>(resource + 88) = 0;
        field<int32_t>(resource + 92) = 15;
        return;
    }
    if (!kinoko_act_release_cloned_texture(resource))
        kinoko_texture_release(field<int32_t>(resource + 68));
    field<int32_t>(resource + 68) = 0;
    kinoko_string_destroy(pointer<void>(resource + 40));
    field<int32_t>(resource + 40) = 0;
    field<int32_t>(resource + 56) = 0;
    field<int32_t>(resource + 60) = 15;
}

void retdec_destroy_cact_resource(int32_t resource) {
    clear_resource(resource);
    std::free(pointer<void>(resource));
}

void retdec_destroy_cact_object(int32_t object_ptr)
{
    if (!object_ptr) return;
    const kinoko::act::DocumentView document(pointer<void>(object_ptr));
    document.set(&kinoko::act::DocumentRecord::vtable,
        kinoko_act_host_symbols()->act_vtable);

    // The document owns every layer and resource. Keep the original forward
    // deletion order and tolerate the loader's partially formed vectors.
    const auto layers = document.get(&kinoko::act::DocumentRecord::layers);
    const int32_t layer_begin = address(layers.begin);
    const int32_t layer_end = address(layers.end);
    for (int32_t slot = layer_begin;
         layer_begin != 0 && layer_end >= layer_begin && slot < layer_end;
         slot += sizeof(KinokoActLayer *)) {
        const auto layer = field<int32_t>(slot);
        if (layer) {
            retdec_destroy_cact_layer(layer);
            std::free(pointer<void>(layer));
        }
    }
    kinoko_act_array_destroy(address(document.bytes(&kinoko::act::DocumentRecord::layers)));
    document.set(&kinoko::act::DocumentRecord::layers,
        kinoko::act::DocumentPointerSpan<KinokoActLayer>{});

    const auto resources = document.get(&kinoko::act::DocumentRecord::resources);
    const int32_t resource_begin = address(resources.begin);
    const int32_t resource_end = address(resources.end);
    for (int32_t slot = resource_begin;
         resource_begin != 0 && resource_end >= resource_begin && slot < resource_end;
         slot += sizeof(KinokoActResource *))
        retdec_destroy_cact_resource(field<int32_t>(slot));
    kinoko_act_array_destroy(address(document.bytes(&kinoko::act::DocumentRecord::resources)));
    document.set(&kinoko::act::DocumentRecord::resources,
        kinoko::act::DocumentPointerSpan<kinoko::act::ResourceRecord>{});

    retdec_destroy_cact_script(address(document.bytes(&kinoko::act::DocumentRecord::script)));
    auto clear_string = [](unsigned char* storage) {
        kinoko_string_destroy(storage);
        const kinoko::native::RecordView<kinoko::legacy::StringRecord> record(storage);
        std::memset(record.bytes(&kinoko::legacy::StringRecord::characters), 0, sizeof(int32_t));
        record.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
        record.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    };
    clear_string(document.bytes(&kinoko::act::DocumentRecord::resource_path));
    clear_string(document.bytes(&kinoko::act::DocumentRecord::name));
}

int32_t retdec_destroy_cact_with_flags(int32_t object_ptr,
                                               unsigned char flags)
{
    if (object_ptr == 0)
        return 0;
    if ((flags & 2) != 0) {
        uint32_t count = field<uint32_t>(object_ptr - 4);
        uint32_t index;
        for (index = 0; index < count; ++index)
            retdec_destroy_cact_object(object_ptr + (int32_t)index * 240);
        if ((flags & 1) != 0)
            std::free(pointer<void>(object_ptr - 4));
        return object_ptr - 4;
    }
    retdec_destroy_cact_object(object_ptr);
    if ((flags & 1) != 0)
        std::free(pointer<void>(object_ptr));
    return object_ptr;
}

namespace {
template<void (*Clear)(int32_t)>
int32_t delete_with_flags(int32_t object,uint32_t size,unsigned char flags) {
    if (!object) return 0;
    if (flags&2) {
        const auto count=field<uint32_t>(object-4);
        for (auto i=count;i>0;--i) Clear(object+(i-1)*size);
        if (flags&1) std::free(pointer<void>(object-4));
        return object-4;
    }
    Clear(object);
    if (flags&1) std::free(pointer<void>(object));
    return object;
}
}
// Original deleting-destructor sizes: 420830=36, 4209B0=348,
// 429720/429780/429840=100. Bit two destroys arrays in reverse order;
// bit one releases the allocation, including its four-byte array cookie.
extern "C" int32_t __fastcall kinoko_method_delete_act_key(int32_t object,void*,unsigned char flags) {
    return delete_with_flags<clear_key>(object,36,flags);
}
extern "C" int32_t __fastcall kinoko_method_delete_act_layer(int32_t object,void*,unsigned char flags) {
    return delete_with_flags<retdec_destroy_cact_layer>(object,sizeof(kinoko::act::LayerStorageRecord),flags);
}
extern "C" int32_t __fastcall kinoko_method_delete_act_resource(int32_t object,void*,unsigned char flags) {
    return delete_with_flags<clear_resource>(object,100,flags);
}


extern "C" int32_t __fastcall kinoko_delete_layout_sprite(int32_t sprite,void*,int32_t flags) {
    const int32_t layout=sprite-4;
    const auto destroy=[](int32_t object) {
        field<int32_t>(object)=address(kinoko_act_host_symbols()->layout_vtable);
        field<int32_t>(object+4)=address(kinoko_act_host_symbols()->color_vtable);
    };
    // Original 42E6E0 -> 42D1C0: secondary this adjustment and array cookie.
    // C2DLayout has no owned nested buffers; its texture handle is borrowed.
    if(flags&2) {
        const int32_t allocation=layout-4;
        const uint32_t count=field<uint32_t>(allocation);
        for(uint32_t i=count;i>0;--i) destroy(layout+316*(i-1));
        if(flags&1) std::free(pointer<void>(allocation));
        return allocation;
    }
    destroy(layout);
    if(flags&1) std::free(pointer<void>(layout));
    return layout;
}
