#include "kinoko/act_ownership.hpp"
#include "kinoko/act_layer_lifecycle.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/act_resource_records_io.hpp"
#include "kinoko/render_target.h"
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


extern "C" int32_t __fastcall kinoko_method_delete_act_script(void* script, void *) {
    if (script) {
        kinoko_destroy_cact_script((void*)(uintptr_t)(script));
        std::free(script);
    }
    return 0;
}

namespace {
void clear_layout(KinokoActLayout* layout) {
    if (!layout) return;
    const auto methods = kinoko::legacy::load<const void*>(layout);
    if (methods == kinoko_string_layout_methods()) { kinoko_clear_string_layout(reinterpret_cast<KinokoStringLayout*>(layout)); return; }
    if (methods == kinoko_act_host_symbols()->map_layout_vtable) kinoko_clear_map_layout((KinokoActLayout*)(uintptr_t)(address(layout)));
}
void clear_key(KinokoActKey* value) {
    if (!value) return;
    using namespace kinoko::act;
    const KeyView key(value);
    key.set(&KeyRecord::methods, kinoko_act_host_symbols()->key_vtable);
    dispose_owned(key.get(&KeyRecord::layout));
    key.set(&KeyRecord::layout, static_cast<KinokoActLayout*>(nullptr));
    auto name = key.view(&KeyRecord::script_name);
    kinoko_string_destroy(name.data());
    *name.bytes(&kinoko::legacy::StringRecord::characters) = 0;
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
}
}
void kinoko_destroy_cact_key(void* value) {
    if (!value) return;
    // The layer's second list owns CActTimeLine, not a key with a layout.
    if (kinoko::legacy::load<const void*>(value)==kinoko_act_timeline_vtable())
        kinoko_native_buffer_destroy(kinoko::native::RecordView<kinoko::act::TimelineRecord>(value).bytes(&kinoko::act::TimelineRecord::pairs));
    else clear_key(static_cast<KinokoActKey*>(value));
    std::free(value);
}
extern "C" void* __fastcall kinoko_method_destroy_layout(KinokoActLayout* layout,void*) {
    clear_layout(layout);
    std::free(layout);
    return layout;
}

void kinoko_destroy_cact_list(void* list_slot) {
    if (!list_slot) return;
    auto* head = kinoko::legacy::load<void*>(list_slot);
    if (!head) return;
    kinoko_act_list_dispose_payloads(head);
    kinoko_act_list_drop_storage(head);
    kinoko::legacy::store(list_slot, static_cast<void*>(nullptr));
}

static void clear_resource(KinokoActResource* resource)
{
    if (resource == 0)
        return;
    if(kinoko::legacy::load<const void*>(resource)==kinoko::mesh::resource_methods()) {
        kinoko::mesh::clear_resource(reinterpret_cast<kinoko::mesh::Resource*>(resource));return;
    }
    using namespace kinoko::act;
    auto clear_string = [](void *storage) {
        kinoko::legacy::StringView(storage).destroy();
        const kinoko::native::RecordView<kinoko::legacy::StringRecord> text(storage);
        *text.bytes(&kinoko::legacy::StringRecord::characters) = 0;
        text.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
        text.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    };
    if (kinoko::legacy::load<const void*>(resource) == kinoko_act_host_symbols()->chip_resource_vtable) {
        const ChipResourceFields chip(resource);
        // 42F1B0: loaded path, shared MCD, source name, then base name.
        clear_string(chip.bytes(&ChipResourceRecord::loaded_path));
        if (kinoko_act_release_chip_data(resource))
            kinoko_mcd_free(chip.get(&ChipResourceRecord::data));
        chip.set(&ChipResourceRecord::data, static_cast<kinoko_mcd_data *>(nullptr));
        clear_string(chip.bytes(&ChipResourceRecord::source_name));
        clear_string(chip.bytes(&ChipResourceRecord::name));
    } else {
        const TextureResourceFields texture(resource);
        const auto handle = texture.get(&TextureResourceRecord::texture);
        const bool borrowed = texture.get(&TextureResourceRecord::borrows_texture) != 0;
        // 449360 resets the device target before releasing an owned target.
        if (!borrowed && handle && kinoko::legacy::load<const void*>(resource) == kinoko_act_host_symbols()->render_target_vtable)
            kinoko_set_render_target(0);
        // Native clones retain a store reference separately from the original
        // borrowed bit. A borrowed handle without that reference is not ours.
        if (!kinoko_act_release_cloned_texture(resource) && !borrowed && handle)
            kinoko_texture_release(handle);
        texture.set(&TextureResourceRecord::texture, int32_t{0});
        clear_string(texture.bytes(&TextureResourceRecord::texture_name));
        clear_string(texture.bytes(&TextureResourceRecord::name));
    }
}

void kinoko_destroy_cact_resource(KinokoActResource* resource) {
    clear_resource(resource);
    std::free(resource);
}

void kinoko_destroy_cact_object(KinokoActDocument* object_ptr)
{
    if (!object_ptr) return;
    const kinoko::act::DocumentView document(object_ptr);
    document.set(&kinoko::act::DocumentRecord::vtable,
        kinoko_act_host_symbols()->act_vtable);

    using namespace kinoko::act;
    // 427610 re-reads each end after virtual callbacks. Both payload passes
    // precede vector destruction; resources' vector is destroyed first.
    auto *layer = document.get(&DocumentRecord::layers).begin;
    while (layer != document.get(&DocumentRecord::layers).end) {
        dispose_owned(kinoko::legacy::load<KinokoActLayer *>(layer));
        ++layer;
    }
    auto *resource = document.get(&DocumentRecord::resources).begin;
    while (resource != document.get(&DocumentRecord::resources).end) {
        dispose_owned(kinoko::legacy::load<KinokoActResource *>(resource));
        ++resource;
    }
    kinoko_act_array_destroy((void*)(document.bytes(&DocumentRecord::resources)));
    kinoko_act_array_destroy((void*)(document.bytes(&DocumentRecord::layers)));

    kinoko_destroy_cact_script((void*)(document.bytes(&kinoko::act::DocumentRecord::script)));
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

namespace {
template<class T, void (*Clear)(T*)>
void* delete_with_flags(T* object, size_t size, unsigned char flags) {
    if (!object) return nullptr;
    auto* bytes = reinterpret_cast<unsigned char*>(object);
    if (flags & 2) {
        auto* allocation = bytes - sizeof(uint32_t);
        const auto count = kinoko::legacy::load<uint32_t>(allocation);
        for (auto i = count; i > 0; --i) Clear(reinterpret_cast<T*>(bytes + (i - 1) * size));
        if (flags & 1) std::free(allocation);
        return allocation;
    }
    Clear(object);
    if (flags & 1) std::free(object);
    return object;
}
}
void* kinoko_destroy_cact_with_flags(KinokoActDocument* object, unsigned char flags) {
    return delete_with_flags<KinokoActDocument, kinoko_destroy_cact_object>(object, sizeof(kinoko::act::DocumentRecord), flags);
}
// Original deleting-destructor sizes: 420830=36, 4209B0=348,
// 429720/429780/429840=100. Bit two destroys arrays in reverse order;
// bit one releases the allocation, including its four-byte array cookie.
extern "C" void* __fastcall kinoko_method_delete_act_key(KinokoActKey* object,void*,unsigned char flags) {
    return delete_with_flags<KinokoActKey, clear_key>(object,36,flags);
}
extern "C" void* __fastcall kinoko_method_delete_act_layer(KinokoActLayer* object,void*,unsigned char flags) {
    return delete_with_flags<KinokoActLayer, kinoko_act_layer_clear>(object,sizeof(kinoko::act::LayerStorageRecord),flags);
}
extern "C" void* __fastcall kinoko_method_delete_act_resource(KinokoActResource* object,void*,unsigned char flags) {
    return delete_with_flags<KinokoActResource, clear_resource>(object,100,flags);
}


namespace {
void clear_layout_identity(KinokoActLayout* layout) {
    using namespace kinoko::act;
    const kinoko::native::RecordView<Layout2DRecord> record(layout);
    record.set(&Layout2DRecord::methods, kinoko_act_host_symbols()->layout_vtable);
    record.view(&Layout2DRecord::quad).set(&kinoko::render::QuadRecord::vtable, kinoko_act_host_symbols()->color_vtable);
}
}
extern "C" void* __fastcall kinoko_delete_layout_sprite(void* sprite, void*, int32_t flags) {
    // Original secondary this adjustment; array cookie precedes the full layout.
    auto* layout = reinterpret_cast<KinokoActLayout*>(static_cast<unsigned char*>(sprite) - offsetof(kinoko::act::Layout2DRecord, quad));
    return delete_with_flags<KinokoActLayout, clear_layout_identity>(layout, sizeof(kinoko::act::Layout2DRecord), static_cast<unsigned char>(flags));
}
