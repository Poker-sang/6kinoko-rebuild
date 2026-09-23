#include "kinoko/act_document_association.hpp"
#include "kinoko/act_script_payload.hpp"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/act_map_records.hpp"
#include "kinoko/act_key_records.hpp"
#include "kinoko/act_mesh.hpp"
#include "kinoko/act_layout3d_io.h"
#include "kinoko/act_layout2d_io.h"
#include "kinoko/act_resource_io.h"
#include "kinoko/act_load_properties.h"
#include "kinoko/legacy_string.h"
#include "kinoko/native_buffer.h"
#include "kinoko/act_array.h"
#include "kinoko/act_list.h"
#include "kinoko/string_layout.h"
#include "kinoko/squirrel_api_types.h"
// Native C++ continuation of the recovered ACT path. Original function names
// remain C ABI ports until the surrounding decompiled host is migrated.
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/boost_hash.h"
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
#include "kinoko/legacy_abi.h"
#include <string>
#include <memory>
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::field;

// Scalar archive reads shared by the native schema loaders.
int32_t retdec_act_read_u8(int32_t reader_ptr, uint8_t *value)
{
    return retdec_reader_read_exact(reader_ptr, value, 1);
}

int32_t retdec_act_read_u32(int32_t reader_ptr, uint32_t *value)
{
    return retdec_reader_read_exact(reader_ptr, value, sizeof(*value));
}

int32_t retdec_act_load_script(int32_t object_ptr, int32_t reader_ptr)
{
    if (!kinoko_act_read_script_properties(object_ptr, reader_ptr)) {
        retdec_trace("act:script-properties-failed");
        return 0;
    }
    uint32_t raw_size = 0;
    if (!retdec_act_read_u32(reader_ptr, &raw_size) || raw_size > 0x1000000u) {
        retdec_trace("act:script-size-failed");
        return 0;
    }
    auto raw_data = std::unique_ptr<unsigned char, decltype(&std::free)>(
        static_cast<unsigned char *>(std::malloc(raw_size ? raw_size : 1u)), &std::free);
    if (!raw_data) {
        retdec_trace("act:script-alloc-failed");
        return 0;
    }
    if (raw_size && !retdec_reader_read_exact(reader_ptr, raw_data.get(), raw_size)) {
        retdec_trace("act:script-data-failed");
        return 0;
    }
    kinoko::act::ScriptPayloadView script(pointer<void>(object_ptr));
    std::free(script.get(&kinoko::act::ScriptPayloadRecord::bytes));
    script.set(&kinoko::act::ScriptPayloadRecord::bytes, static_cast<void *>(raw_data.release()));
    script.set(&kinoko::act::ScriptPayloadRecord::size, raw_size);
    script.set(&kinoko::act::ScriptPayloadRecord::loaded, uint8_t{1});
    return 1;
}

int32_t retdec_construct_cact_layer(int32_t layer, int32_t vm) {
    if (!layer) return 0;
    std::memset(pointer<void>(layer), 0, 348);
    field<int32_t>(layer) = address(kinoko_act_host_symbols()->layer_vtable);
    field<int32_t>(layer + 96) = -1;
    field<int32_t>(layer + 104) = -1;
    field<int32_t>(layer + 108) = -1;
    field<int32_t>(layer + 132) = 15;
    kinoko_string_assign_cstr(pointer<int32_t>(layer + 112), "Layer_");
    field<uint16_t>(layer + 140) = 1;
    field<uint8_t>(layer + 92) = 1;
    if (!retdec_act_make_list(pointer<int32_t>(layer + 180)) ||
        !retdec_act_make_list(pointer<int32_t>(layer + 192))) {
        kinoko_act_list_drop_storage(field<int32_t>(layer+180));
        kinoko_act_list_drop_storage(field<int32_t>(layer+192));
        field<int32_t>(layer + 180) = field<int32_t>(layer + 192) = 0;
        kinoko_string_destroy((void*)(intptr_t)(layer+112));
        return 0;
    }
    retdec_construct_cact_script(layer + 204);
    for (int32_t offset : {208, 228, 248}) field<int32_t>(layer + offset) = vm;
    field<int32_t>(layer + 308) = address(kinoko_act_host_symbols()->layer_ref_vtable);
    field<int32_t>(layer + 312) = vm;
    field<uint8_t>(layer + 324) = 1;
    sq_resetobject(reinterpret_cast<HSQOBJECT*>(pointer<void>(layer + 316)));
    field<int32_t>(layer + 328) = address(kinoko_act_host_symbols()->layer_layout_vtable);
    field<int32_t>(layer + 332) = vm;
    field<uint8_t>(layer + 344) = 1;
    sq_resetobject(reinterpret_cast<HSQOBJECT*>(pointer<void>(layer + 336)));
    if (vm && !kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), pointer<int32_t>(layer + 316))) {
        retdec_destroy_cact_layer(layer);
        return 0;
    }
    return layer;
}

int32_t retdec_act_make_layer(void) {
    const int32_t layer = address(std::calloc(1u, 348u));
    // The archive parser can run before VM creation; publication creates its
    // script table once a VM is available. Original native callers pass g664.
    if (!retdec_construct_cact_layer(layer, 0)) {
        std::free(pointer<void>(layer));
        return 0;
    }
    return layer;
}

int32_t retdec_construct_c2dlayout(int32_t layout) {
    if (!layout) return 0;
    // 42B4A0 initializes the layout and its embedded CSpriteEx view.
    kinoko::native::RecordView<kinoko::act::Layout2DRecord> record(pointer<void>(layout));
    record.clear();
    record.set(&kinoko::act::Layout2DRecord::methods,
        static_cast<const void *>(kinoko_act_host_symbols()->layout_vtable));
    auto quad = record.view(&kinoko::act::Layout2DRecord::quad);
    quad.set(&kinoko::render::QuadRecord::vtable,
        static_cast<uint32_t>(address(kinoko_act_host_symbols()->layout_sprite_vtable)));
    record.set(&kinoko::act::Layout2DRecord::scale,
        kinoko::render::Position3{1.0f, 1.0f, 1.0f});
    record.set(&kinoko::act::Layout2DRecord::alpha, 1.0f);
    record.set(&kinoko::act::Layout2DRecord::blend, int32_t{1});
    record.set(&kinoko::act::Layout2DRecord::red, int32_t{255});
    record.set(&kinoko::act::Layout2DRecord::green, int32_t{255});
    record.set(&kinoko::act::Layout2DRecord::blue, int32_t{255});
    return layout;
}

int32_t retdec_act_make_layout(int32_t reader_ptr)
{
    auto layout = std::unique_ptr<KinokoActLayout, decltype(&std::free)>(
        static_cast<KinokoActLayout *>(std::calloc(1u, sizeof(kinoko::act::Layout2DRecord))),
        &std::free);
    if (!layout || !retdec_construct_c2dlayout(address(layout.get()))) return 0;
    // 42C030's holder is still an integer ABI slot; the layout is borrowed.
    if (!kinoko_act_read_layout2d_properties(layout.get(), &reader_ptr, 1)) return 0;
    return address(layout.release());
}

int32_t retdec_act_make_map_layout(int32_t reader_ptr)
{
    auto layout = std::unique_ptr<KinokoActLayout, decltype(&std::free)>(
        static_cast<KinokoActLayout *>(std::calloc(1u, sizeof(kinoko::act::MapLayoutRecord))),
        &std::free);
    if (!layout) return 0;
    kinoko::act::MapLayoutView record(layout.get());
    record.set(&kinoko::act::MapLayoutRecord::methods,
        kinoko_act_host_symbols()->map_layout_vtable);
    record.set(&kinoko::act::MapLayoutRecord::view_methods,
        kinoko_act_host_symbols()->map_view_vtable);
    record.set(&kinoko::act::MapLayoutRecord::x_limit, INT32_MAX);
    record.set(&kinoko::act::MapLayoutRecord::y_limit, INT32_MAX);
    record.set(&kinoko::act::MapLayoutRecord::alpha, 1.0f);
    record.set(&kinoko::act::MapLayoutRecord::secondary_alpha, 1.0f);
    record.set(&kinoko::act::MapLayoutRecord::blend, int32_t{1});
    record.set(&kinoko::act::MapLayoutRecord::resource_id, int32_t{-1});
    if (!kinoko_act_read_map_properties_typed(layout.get(),
            pointer<KinokoArchiveReader>(reader_ptr))) return 0;
    return address(layout.release());
}

void retdec_act_free_map_records(int32_t layout)
{
    if (layout) {
        kinoko::act::MapLayoutView record(pointer<void>(layout));
        kinoko_native_buffer_destroy(address(record.bytes(&kinoko::act::MapLayoutRecord::records_begin)));
    }
}

int32_t retdec_act_read_map_records(int32_t layout,
                                           int32_t reader_ptr)
{
    uint32_t count;
    uint32_t serialized_size;
    uint32_t read_size;
    uint32_t index;
    unsigned char *records;

    if (layout == 0 ||
        !retdec_act_read_u32(reader_ptr, &count) ||
        !retdec_act_read_u32(reader_ptr, &serialized_size) ||
        count > 0x10000u || serialized_size > 0x20u)
        return 0;
    kinoko::act::MapLayoutView map(pointer<void>(layout));
    const auto records_slot = address(map.bytes(&kinoko::act::MapLayoutRecord::records_begin));
    kinoko_native_buffer_destroy(records_slot);
    if (count == 0)
        return 1;

    if (serialized_size > 0x20u ||
        count > UINT32_MAX / 0x20u)
        return 0;
    if(!kinoko_native_buffer_resize(records_slot,count*sizeof(kinoko::act::MapCellRecord))) return 0;
    records=static_cast<unsigned char *>(map.get(&kinoko::act::MapLayoutRecord::records_begin) ?
        static_cast<void *>(map.get(&kinoko::act::MapLayoutRecord::records_begin)) : nullptr);
    read_size = serialized_size;
    for (index = 0; index < count; ++index) {
        unsigned char *record = records + (size_t)index * 0x20u;
        if (read_size != 0 &&
            !retdec_reader_read_exact(reader_ptr, record, read_size)) {
            kinoko_native_buffer_destroy(records_slot);
            return 0;
        }
        kinoko::act::MapCellView cell(record);
        cell.set(&kinoko::act::MapCellRecord::index, index);
        cell.set(&kinoko::act::MapCellRecord::enabled, uint8_t{1});
        cell.set(&kinoko::act::MapCellRecord::opacity, 1.0f);
    }
    map.set(&kinoko::act::MapLayoutRecord::records_begin,
        reinterpret_cast<kinoko::act::MapCellRecord *>(records));
    map.set(&kinoko::act::MapLayoutRecord::records_end,
        reinterpret_cast<kinoko::act::MapCellRecord *>(records + size_t{count} * sizeof(kinoko::act::MapCellRecord)));

    retdec_trace_i32("act:map-record-count", (int32_t)count);
    retdec_trace_i32("act:map-record-size", (int32_t)serialized_size);
    return 1;
}

int32_t retdec_act_load_key(int32_t key, int32_t reader_ptr,
                                   int32_t version)
{
    uint8_t has_layout;
    uint32_t layout_type = 0;
    int32_t layout;

    if (!key || version != 1 || !kinoko_act_read_key_properties_typed(pointer<KinokoActKey>(key),
            pointer<KinokoArchiveReader>(reader_ptr))) {
        retdec_trace("act:key-properties-failed");
        return 0;
    }
    if (!retdec_act_read_u8(reader_ptr, &has_layout)) {
        retdec_trace("act:key-layout-flag-failed");
        return 0;
    }
    if (has_layout == 0)
        return 1;
    if (!retdec_act_read_u32(reader_ptr, &layout_type) ||
        (layout_type != 0x655cd5b0u &&
         layout_type != 0xc9ca5c20u && layout_type != 0x9e695d47u &&
         layout_type != kinoko::mesh::layout_type())) {
        retdec_trace_i32("act:unsupported-layout", (int32_t)layout_type);
        return 0;
    }
    if(layout_type==kinoko::mesh::layout_type()) {
        layout=address(kinoko::mesh::create_layout());
        if(layout && !kinoko_act_read_layout3d_properties(
                pointer<KinokoActLayout>(layout),&reader_ptr,version)) {
            std::free(pointer<void>(layout));layout=0;
        }
    } else if(layout_type==0x9e695d47u) {
        // Original Boost hash of .?AVCStringLayout@@; use the genuine native
        // reader through its recovered holder/version ABI.
        layout=address(std::calloc(1,260));
        if(layout) {
            kinoko_construct_string_layout(layout);
            if(!kinoko_string_read_properties(pointer<KinokoStringLayout>(layout),
                    &reader_ptr,version)) {
                kinoko_clear_string_layout(layout);std::free(pointer<void>(layout));layout=0;
            }
        }
    } else if (layout_type == 0xc9ca5c20u) {
        layout = retdec_act_make_map_layout(reader_ptr);
        if (layout != 0 && !retdec_act_read_map_records(layout, reader_ptr)) {
            retdec_act_free_map_records(layout);
            std::free(pointer<void>(layout));
            layout = 0;
        }
    } else {
        layout = retdec_act_make_layout(reader_ptr);
    }
    if (layout == 0)
        return 0;
    field<int32_t>(key + 4) = layout;
    return 1;
}

int32_t retdec_act_make_key(int32_t reader_ptr, int32_t version)
{
    auto destroy = [](KinokoActKey *key) { retdec_destroy_cact_key(address(key)); };
    std::unique_ptr<KinokoActKey, decltype(destroy)> key(
        static_cast<KinokoActKey *>(std::calloc(1u, sizeof(kinoko::act::KeyRecord))), destroy);
    if (!key) return 0;
    kinoko::act::KeyView record(key.get());
    record.set(&kinoko::act::KeyRecord::methods, kinoko_act_host_symbols()->key_vtable);
    auto name = record.view(&kinoko::act::KeyRecord::script_name);
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    if (!retdec_act_load_key(address(key.get()), reader_ptr, version)) return 0;
    return address(key.release());
}

int32_t retdec_act_load_layer(int32_t layer, int32_t reader_ptr,
                                     int32_t version)
{
    uint32_t count;
    uint32_t index;
    uint32_t type;
    int32_t key;

    if (!layer || version != 1 || !kinoko_act_read_layer_properties_typed(pointer<KinokoActLayer>(layer),
            pointer<KinokoArchiveReader>(reader_ptr))) {
        retdec_trace("act:layer-properties-failed");
        return 0;
    }
    retdec_trace_squirrel_name("act:layer-name",
                               address(kinoko_string_data((const void*)(intptr_t)(layer + 112))));
    if (!retdec_act_read_u32(reader_ptr, &count) || count > 0x10000u) {
        retdec_trace("act:layer-key-count-failed");
        return 0;
    }
    for (index = 0; index < count; ++index) {
        if (!retdec_act_read_u32(reader_ptr, &type) ||
            type != 0xd933304du) {
            retdec_trace_i32("act:unsupported-key", (int32_t)type);
            return 0;
        }
        key = retdec_act_make_key(reader_ptr, version);
        if (key == 0 || !retdec_act_append_list(layer + 0xb4, key)) {
            retdec_destroy_cact_key(key);
            retdec_trace("act:layer-key-load-failed");
            return 0;
        }
        // 41F8B9 binds every newly read layout to its containing layer before
        // the next key. Resource association may happen later during ACT load.
        const auto layout = field<int32_t>(key + 4);
        if (layout) retdec_call_thiscall1_result(pointer<void>(layout),
            field<void*>(field<int32_t>(layout) + 24), layer);
        retdec_trace_squirrel_name(
            "act:key-script", address(kinoko_string_data((const void*)(intptr_t)(key + 8))));
        retdec_trace_i32("act:key-layout", field<int32_t>(key + 4));
        if (field<int32_t>(key + 4) != 0 &&
            field<int32_t>(field<int32_t>(key + 4)) ==
                address(kinoko_act_host_symbols()->map_layout_vtable)) {
            int32_t key_layout = field<int32_t>(key + 4);
            int32_t key_begin = field<int32_t>(key_layout + 264);
            int32_t key_end = field<int32_t>(key_layout + 268);
            retdec_trace_i32("act:key-map-record-count",
                             key_begin != 0 && key_end >= key_begin
                                 ? (int32_t)((key_end - key_begin) / 0x20)
                                 : 0);
        }
        ++field<int32_t>(layer + 0xb8);
    }
    if (!retdec_act_read_u32(reader_ptr, &count) || count > 0x10000u) {
        retdec_trace("act:layer-extra-count-failed");
        return 0;
    }
    // 41F800's second factory loop loads CActTimeLine, whose raw RTTI name
    // .?AVCActTimeLine@@ hashes to 9902F2C0 with the original Boost algorithm.
    for (index = 0; index < count; ++index) {
        if (!retdec_act_read_u32(reader_ptr, &type) || type != 0x9902f2c0u) {
            retdec_trace_i32("act:unsupported-timeline", (int32_t)type);
            return 0;
        }
        const auto timeline = kinoko_act_new_timeline();
        if (!timeline || !kinoko_act_load_timeline(timeline, reader_ptr, version) ||
            !retdec_act_append_list(layer + 192, timeline)) {
            retdec_destroy_cact_key(timeline);
            return 0;
        }
        ++field<int32_t>(layer + 196);
    }
    return retdec_act_load_script(layer + 0xcc, reader_ptr);
}

uint32_t retdec_mcd_u32(const unsigned char *bytes)
{
    uint32_t value;

    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

int16_t retdec_mcd_i16(const unsigned char *bytes)
{
    int16_t value;

    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

struct retdec_mcd_chip *retdec_mcd_find_chip(
    struct retdec_mcd_data *data, uint32_t chip_id)
{
    uint32_t index;

    if (data == nullptr)
        return nullptr;
    for (index = 0; index < data->chip_count; ++index) {
        if (data->chips[index].chip_id == chip_id)
            return &data->chips[index];
    }
    return nullptr;
}

struct retdec_mcd_texture *retdec_mcd_find_texture(
    struct retdec_mcd_data *data, uint32_t texture_id)
{
    uint32_t index;

    if (data == nullptr)
        return nullptr;
    for (index = 0; index < data->texture_count; ++index) {
        if (data->textures[index].texture_id == texture_id)
            return &data->textures[index];
    }
    return nullptr;
}

void retdec_mcd_free(struct retdec_mcd_data *data)
{
    if (data == nullptr)
        return;
    if (data->textures != nullptr) {
        for (uint32_t index = 0; index < data->texture_count; ++index)
            kinoko_texture_release(data->textures[index].handle);
    }
    std::free(data->chips);
    std::free(data->textures);
    std::free(data);
}

int32_t retdec_act_load_mcd(int32_t resource,
                                   const char *file_name)
{
    KinokoArchiveReader *reader_slot = nullptr;
    struct retdec_mcd_data *data = nullptr;
    uint32_t magic;
    uint32_t version;
    uint32_t payload_offset;
    uint32_t chip_count;
    uint32_t record_size;
    uint32_t texture_count;
    uint32_t index;
    uint32_t loaded_texture_count = 0;

    if (resource == 0 || file_name == nullptr || *file_name == 0)
        return 0;
    if (!kinoko_reader_open(&reader_slot, file_name)) {
        retdec_trace_squirrel_name("mcd:open-failed",
                                   address(file_name));
        return 0;
    }

    if (!kinoko_reader_read_exact(reader_slot, &magic, sizeof(magic)) ||
        magic != 0x434d4432u ||
        !kinoko_reader_read_exact(reader_slot, &version, sizeof(version)) ||
        version > 1u ||
        !kinoko_reader_read_exact(reader_slot, &payload_offset,
                                  sizeof(payload_offset)) ||
        (payload_offset != 0 &&
         !kinoko_reader_seek_relative(reader_slot, payload_offset)) ||
        !kinoko_reader_read_exact(reader_slot, &chip_count,
                                  sizeof(chip_count)) ||
        !kinoko_reader_read_exact(reader_slot, &record_size,
                                  sizeof(record_size)) ||
        chip_count > 0x10000u || record_size > 48u) {
        retdec_trace("mcd:header-failed");
        kinoko_reader_close(reader_slot);
        return 0;
    }

    data = (struct retdec_mcd_data *)std::calloc(1u, sizeof(*data));
    if (data == nullptr) {
        kinoko_reader_close(reader_slot);
        return 0;
    }
    data->chip_count = chip_count;
    if (chip_count != 0) {
        data->chips = (struct retdec_mcd_chip *)std::calloc(
            (size_t)chip_count, sizeof(*data->chips));
        if (data->chips == nullptr)
            goto load_failed;
    }

    for (index = 0; index < chip_count; ++index) {
        unsigned char record[48] = { 0 };
        uint32_t next_record;

        if ((record_size != 0 &&
             !kinoko_reader_read_exact(reader_slot, record, record_size)) ||
            !kinoko_reader_read_exact(reader_slot, &next_record,
                                      sizeof(next_record)))
            goto load_failed;
        data->chips[index].chip_id = retdec_mcd_u32(record);
        std::memcpy(data->chips[index].bytes, record, sizeof(record));
        if (data->chips[index].chip_id >= 0x8d0u &&
            data->chips[index].chip_id <= 0x920u) {
            retdec_trace_i32("mcd:chip-index", (int32_t)index);
            retdec_trace_i32("mcd:chip-id",
                             (int32_t)data->chips[index].chip_id);
        }
    }

    if (!kinoko_reader_read_exact(reader_slot, &texture_count,
                                  sizeof(texture_count)) ||
        texture_count > 0x10000u)
        goto load_failed;
    data->texture_count = texture_count;
    if (texture_count != 0) {
        data->textures = (struct retdec_mcd_texture *)std::calloc(
            (size_t)texture_count, sizeof(*data->textures));
        if (data->textures == nullptr)
            goto load_failed;
    }

    for (index = 0; index < texture_count; ++index) {
        uint32_t texture_id;
        uint32_t name_length;
        char *name;
        int32_t handle;

        if (!kinoko_reader_read_exact(reader_slot, &texture_id,
                                      sizeof(texture_id)) ||
            !kinoko_reader_read_exact(reader_slot, &name_length,
                                      sizeof(name_length)) ||
            name_length > 0x100000u)
            goto load_failed;
        name = (char *)std::malloc((size_t)name_length + 1u);
        if (name == nullptr)
            goto load_failed;
        if (name_length != 0 &&
            !kinoko_reader_read_exact(reader_slot, name, name_length)) {
            std::free(name);
            goto load_failed;
        }
        name[name_length] = 0;
        handle = retdec_load_act_texture(name);
        retdec_trace_squirrel_name("mcd:texture-name",
                                   address(name));
        retdec_trace_i32("mcd:texture-id", (int32_t)texture_id);
        retdec_trace_i32("mcd:texture-handle", handle);
        if (handle > 0 && (uint32_t)handle < KINOKO_TEXTURE_CAPACITY) {
            retdec_trace_i32("mcd:texture-width",
                             (int32_t)kinoko_texture_slots[
                                 (uint32_t)handle].width);
            retdec_trace_i32("mcd:texture-height",
                             (int32_t)kinoko_texture_slots[
                                 (uint32_t)handle].height);
        }
        std::free(name);
        data->textures[index].texture_id = texture_id;
        data->textures[index].handle = handle;
        if (handle != 0)
            ++loaded_texture_count;
    }

    kinoko_reader_close(reader_slot);
    field<int32_t>(resource + 64) =
        address(data);
    kinoko_string_assign_cstr(
        pointer<int32_t>(resource + 72), file_name);
    retdec_trace_i32("mcd:chip-count", (int32_t)chip_count);
    retdec_trace_i32("mcd:texture-count", (int32_t)texture_count);
    retdec_trace_i32("mcd:texture-loaded", (int32_t)loaded_texture_count);
    return 1;

load_failed:
    kinoko_reader_close(reader_slot);
    retdec_mcd_free(data);
    retdec_trace("mcd:load-failed");
    return 0;
}

extern "C" int32_t __fastcall kinoko_method_unload_resource_texture(int32_t resource, void *) {
    if (!resource) return 0;
    const int32_t handle = field<int32_t>(resource + 68);
    if (!kinoko_act_release_cloned_texture(resource) && !field<uint8_t>(resource + 36) && handle)
        kinoko_texture_release(handle);
    field<int32_t>(resource + 68) = 0;
    return 1;
}

extern "C" int32_t __fastcall kinoko_method_load_chip_resource(
    int32_t resource, void*, const char* prefix) {
    if (!resource) return 0;
    const char* name = kinoko_string_data((const void*)(intptr_t)(resource+36));
    if (!name || !*name) return 0;
    try {
        // 42FB4E uses an empty default prefix. Append '/' only to a nonempty
        // prefix that lacks either accepted separator (42FC5A..42FC71).
        std::string base(prefix ? prefix : "");
        if (!base.empty() && base.back()!='/' && base.back()!='\\') base += '/';
        const std::string path = base + name;
        auto destroy = [](int32_t* value) { retdec_destroy_cact_resource(address(value)); };
        std::unique_ptr<int32_t, decltype(destroy)> temporary(
            static_cast<int32_t*>(std::calloc(1,100)), destroy);
        if (!temporary) return 0;
        temporary.get()[0] = address(kinoko_act_host_symbols()->chip_resource_vtable);
        temporary.get()[7] = temporary.get()[14] = temporary.get()[23] = 15;
        // Original loads into a temporary owner and only replaces on success.
        if (!retdec_act_load_mcd(address(temporary.get()), path.c_str())) return 0;
        kinoko_string_assign_cstr(pointer<int32_t>(resource+72), base.c_str());
        if (kinoko_act_release_chip_data(resource))
            retdec_mcd_free(pointer<retdec_mcd_data>(field<int32_t>(resource+64)));
        field<int32_t>(resource+64) = temporary.get()[16];
        temporary.get()[16] = 0;
        return 1;
    } catch (...) { return 0; }
}

extern "C" int32_t __fastcall kinoko_method_load_resource_texture(
    int32_t resource, void *, const char *prefix) {
    if (!resource) return 0;
    const char *name = kinoko_string_data((const void*)(intptr_t)(resource + 40));
    // 446C36 leaves the existing handle untouched for an empty texture name.
    if (!name || !*name) return 0;
    try {
        std::string path(prefix && *prefix ? prefix : "./");
        if (path.back() != '/' && path.back() != '\\') path += '/';
        retdec_call_thiscall0(pointer<void>(resource),
            field<void *>(field<int32_t>(resource) + 44));
        field<uint8_t>(resource + 36) = 0;
        path += name;
        // 431D80 concatenates prefix/name; 40E540 appends each suffix. The
        // texture reader, not this resource, maps DDS/BMP/PNG requests to CV2.
        for (const char *suffix : {".dds", ".bmp", ".png"}) {
            const auto candidate = path + suffix;
            const int32_t handle = kinoko_texture_acquire(candidate.c_str());
            field<int32_t>(resource + 68) = handle;
            if (!handle) continue;
            const auto &slot = kinoko_texture_slots[handle];
            field<int32_t>(resource + 72) = slot.width;
            field<int32_t>(resource + 76) = slot.height;
            if (field<uint8_t>(resource + 96)) {
                field<float>(resource + 80) = 0;
                field<float>(resource + 84) = 0;
                field<float>(resource + 88) = static_cast<float>(slot.width);
                field<float>(resource + 92) = static_cast<float>(slot.height);
            }
            return 1;
        }
    } catch (...) {
        return 0;
    }
    return 0;
}

int32_t retdec_act_make_resource(int32_t reader_ptr, uint32_t type)
{
    int32_t resource;
    if(type==kinoko::mesh::resource_type()) {
        auto *mesh=kinoko::mesh::create_resource();
        if(mesh && !kinoko::mesh::read_resource_properties(mesh,&reader_ptr,1)) {
            kinoko::mesh::clear_resource(mesh);std::free(mesh);return 0;
        }
        return address(mesh);
    }
    // Original 449C50 registers the raw RTTI name in the same Boost-hashed
    // factory used by 428150. This type owns an independent property schema.
    static const char target_name[] = ".?AVCActRenderTarget@@";
    static const auto target_type = static_cast<uint32_t>(kinoko_boost_hash_range(
        address(target_name), address(target_name + sizeof(target_name) - 1)));
    const bool render_target = type == target_type;
    const bool texture = type == 0xc6fdb98au || render_target;

    if (!texture && type != 0xfbaaf527u) {
        retdec_trace_i32("act:unsupported-resource", (int32_t)type);
        return 0;
    }
    resource = address(std::calloc(1u, 100u));
    if (resource == 0)
        return 0;
    field<int32_t>(resource) = render_target
        ? address(kinoko_act_host_symbols()->render_target_vtable)
        : type == 0xfbaaf527u
        ? address(kinoko_act_host_symbols()->chip_resource_vtable)
        : address(kinoko_act_host_symbols()->texture_resource_vtable);
    field<int32_t>(resource + 4) = -1;
    field<int32_t>(resource + 24) = 0;
    field<int32_t>(resource + 28) = 15;
    field<int32_t>(resource + 56) = 0;
    field<int32_t>(resource + 60) = 15;
    field<uint8_t>(resource + 8) = 0;
    field<uint8_t>(resource + 40) = 0;
    if (texture) {
        field<int32_t>(resource + 72) = 256;
        field<int32_t>(resource + 76) = 256;
        field<uint8_t>(resource + 96) = 1;
    } else {
        field<int32_t>(resource + 52) = 0;
        field<int32_t>(resource + 56) = 15;
        field<uint8_t>(resource + 36) = 0;
        field<int32_t>(resource + 64) = 0;
        field<int32_t>(resource + 68) = 0;
        field<int32_t>(resource + 88) = 0;
        field<int32_t>(resource + 92) = 15;
        field<uint8_t>(resource + 72) = 0;
    }
    auto* borrowed_resource=pointer<KinokoActResource>(resource);
    const auto loaded = render_target
        ? kinoko_act_read_render_target_properties(borrowed_resource, &reader_ptr, 1)
        : type == 0xfbaaf527u
        ? kinoko_act_read_chip_properties(borrowed_resource, &reader_ptr, 1)
        : kinoko_act_read_texture_properties(borrowed_resource, &reader_ptr, 1);
    if (!loaded) {
        retdec_trace("act:resource-properties-failed");
        retdec_destroy_cact_resource(resource);
        return 0;
    }
    if (render_target) {
        // 428150 only constructs/deserializes the target here. D3DX creation
        // belongs to the separate virtual Create(width,height) entry; do not
        // load stTextureName as a file or replace the serialized crop rectangle.
        return resource;
    }
    if (type == 0xc6fdb98au) {
        // Original 446A84 clears auto-size after deserializing, including an
        // absent property block. Serialized atlas regions must survive LoadTexture.
        field<uint8_t>(resource + 96) = 0;
    }
    // 428150 deserializes and publishes resources. Loading is the later
    // 4289C0 virtual pass, after every resource has been parsed and bound.
    return resource;
}

int32_t retdec_c2dmaplayout_set_layer_impl(int32_t layout,
                                                   int32_t layer)
{
    int32_t resource;

    if (layout == 0 || layer == 0)
        return -0x7fffbffb;
    resource = field<int32_t>(layer + 0x64);
    field<int32_t>(layout + 312) = layer;
    field<int32_t>(layout + 316) = resource;
    /* 4341F0 exposes alpha/blend through CActLayer's pointer properties. */
    field<int32_t>(layer + 52) = layout + 320;
    field<int32_t>(layer + 56) = layout + 328;
    kinoko_native_buffer_destroy(layout+332);
    field<int32_t>(layout + 380) = 0;
    retdec_trace_i32("map-layout:bind-layer", layer);
    retdec_trace_i32("map-layout:bind-resource", resource);
    retdec_trace_i32("map-layout:mcd", resource != 0
                     ? field<int32_t>(resource + 64) : 0);
    return 0;
}

int32_t retdec_act_prepare_vector(int32_t object_ptr,
                                         uint32_t begin_offset,
                                         uint32_t end_offset,
                                         uint32_t capacity_offset,
                                         uint32_t count)
{
    if (end_offset!=begin_offset+4 || capacity_offset!=begin_offset+8) return 0;
    return kinoko_act_array_prepare(object_ptr+begin_offset,count);
}

int32_t retdec_act_load(int32_t this_ptr, int32_t reader_ptr,
                               int32_t version)
{
    uint32_t layer_count;
    uint32_t resource_count;
    uint32_t index;
    uint32_t type;
    int32_t *layers;
    int32_t *resources;
    int32_t layer;
    int32_t resource;

    if (this_ptr == 0 || reader_ptr == 0 || version != 1)
        return 0;
    if (!kinoko_act_read_document_properties_typed(pointer<KinokoActDocument>(this_ptr),
            pointer<KinokoArchiveReader>(reader_ptr))) {
        retdec_trace("act:cact-properties-failed");
        return 0;
    }
    if (!retdec_act_load_script(this_ptr + 100, reader_ptr)) {
        retdec_trace("act:cact-script-failed");
        return 0;
    }
    if (!retdec_act_read_u32(reader_ptr, &layer_count) ||
        !retdec_act_prepare_vector(this_ptr, 208, 212, 216, layer_count)) {
        retdec_trace("act:layer-vector-failed");
        return 0;
    }
    kinoko::act::DocumentLoadAssociations associations;
    auto *document = pointer<KinokoActDocument>(this_ptr);
    layers = pointer<int32_t>(field<int32_t>(this_ptr + 208));
    for (index = 0; index < layer_count; ++index) {
        if (!retdec_act_read_u32(reader_ptr, &type) ||
            type != 0x2618cf18u) {
            retdec_trace_i32("act:unsupported-layer", (int32_t)type);
            return 0;
        }
        layer = retdec_act_make_layer();
        if (layer == 0 || !retdec_act_load_layer(layer, reader_ptr, version)) {
            retdec_destroy_cact_layer(layer);
            std::free(pointer<void>(layer));
            retdec_trace("act:layer-load-failed");
            return 0;
        }
        layers[index] = layer;
        field<int32_t>(this_ptr + 212) += 4;
        associations.add_layer(pointer<KinokoActLayer>(layer));
        if (index < 8) {
            retdec_trace_i32("act:layer-id",
                             field<int32_t>(layer + 0x68));
            retdec_trace_i32("act:layer-resource",
                             field<int32_t>(layer + 0x60));
        }
    }
    // 428310..428346: resolve parents before even reading resource_count.
    associations.bind_loaded_parents(document, layer_count);
    if (!retdec_act_read_u32(reader_ptr, &resource_count) ||
        !retdec_act_prepare_vector(this_ptr, 224, 228, 232,
                                   resource_count)) {
        retdec_trace("act:resource-vector-failed");
        return 0;
    }
    associations.begin_resources();
    resources = pointer<int32_t>(field<int32_t>(this_ptr + 224));
    for (index = 0; index < resource_count; ++index) {
        if (!retdec_act_read_u32(reader_ptr, &type)) {
            retdec_trace("act:resource-type-failed");
            return 0;
        }
        resource = retdec_act_make_resource(reader_ptr, type);
        if (resource == 0)
            return 0;
        resources[index] = resource;
        field<int32_t>(this_ptr + 228) += 4;
        associations.add_resource(pointer<KinokoActResource>(resource));
        if (index < 8) {
            retdec_trace_i32("act:resource-id",
                             field<int32_t>(resource + 4));
            retdec_trace_i32("act:resource-texture",
                             field<int32_t>(resource + 40));
        }
    }
    retdec_trace_i32("act:loaded-layers", (int32_t)layer_count);
    retdec_trace_i32("act:loaded-resources", (int32_t)resource_count);
    associations.bind_resources(document, layer_count);
    return 1;
}
