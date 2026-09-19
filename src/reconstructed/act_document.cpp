// Native C++ continuation of the recovered ACT path. Original function names
// remain C ABI ports until the surrounding decompiled host is migrated.
#include "kinoko/act_runtime.h"
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

// ACT property parsing/mapping lives in act_properties.cpp.

int32_t retdec_act_load_script(int32_t object_ptr, int32_t reader_ptr)
{
    struct retdec_act_property *properties = nullptr;
    uint32_t property_count = 0;
    uint32_t raw_size;
    unsigned char *raw_data;

    if (!retdec_act_read_properties(reader_ptr, &properties,
                                    &property_count)) {
        retdec_trace("act:script-properties-failed");
        return 0;
    }
    retdec_act_apply_script(object_ptr, properties, property_count);
    retdec_act_free_properties(properties, property_count);
    if (!retdec_act_read_u32(reader_ptr, &raw_size) ||
        raw_size > 0x1000000u) {
        retdec_trace("act:script-size-failed");
        return 0;
    }
    raw_data = (unsigned char *)std::malloc(raw_size == 0 ? 1u : raw_size);
    if (raw_data == nullptr) {
        retdec_trace("act:script-alloc-failed");
        return 0;
    }
    if (raw_size != 0 && !retdec_reader_read_exact(reader_ptr, raw_data,
                                                   raw_size)) {
        std::free(raw_data);
        retdec_trace("act:script-data-failed");
        return 0;
    }
    field<int32_t>(object_ptr + 92) =
        address(raw_data);
    field<uint32_t>(object_ptr + 96) = raw_size;
    field<uint8_t>(object_ptr + 100) = 1;
    return 1;
}

int32_t retdec_act_make_list(int32_t *list_slot)
{
    int32_t *sentinel;

    if (list_slot == nullptr)
        return 0;
    sentinel = (int32_t *)std::malloc(12u);
    if (sentinel == nullptr)
        return 0;
    sentinel[0] = address(sentinel);
    sentinel[1] = address(sentinel);
    sentinel[2] = 0;
    *list_slot = address(sentinel);
    return 1;
}

int32_t retdec_act_append_list(int32_t list_slot, int32_t value)
{
    int32_t sentinel;
    int32_t previous;
    int32_t *node;

    if (list_slot == 0)
        return 0;
    sentinel = field<int32_t>(list_slot);
    if (sentinel == 0)
        return 0;
    previous = field<int32_t>(sentinel + 4);
    node = (int32_t *)std::malloc(12u);
    if (node == nullptr)
        return 0;
    node[0] = sentinel;
    node[1] = previous;
    node[2] = value;
    field<int32_t>(sentinel + 4) = address(node);
    field<int32_t>(previous) = address(node);
    return 1;
}

int32_t retdec_act_make_layer(void)
{
    int32_t layer;

    layer = address(std::calloc(1u, 348u));
    if (layer == 0)
        return 0;
    field<int32_t>(layer) = address(kinoko_act_host_symbols()->layer_vtable);
    field<int32_t>(layer + 0x60) = -1;
    field<int32_t>(layer + 0x68) = -1;
    field<int32_t>(layer + 0x6c) = -1;
    field<int32_t>(layer + 0x80) = 0;
    field<int32_t>(layer + 0x84) = 15;
    field<uint8_t>(layer + 0x70) = 0;
    field<uint16_t>(layer + 0x8c) = 1;
    field<uint8_t>(layer + 0x5c) = 1;
    if (!retdec_act_make_list(pointer<int32_t>(layer + 0xb4)) ||
        !retdec_act_make_list(pointer<int32_t>(layer + 0xc0))) {
        std::free(pointer<void>(layer));
        return 0;
    }
    field<int32_t>(layer + 0xb8) = 0;
    field<int32_t>(layer + 0xc4) = 0;
    if (retdec_construct_cact_script(layer + 0xcc) == 0) {
        std::free(pointer<void>(layer));
        return 0;
    }
    return layer;
}

int32_t retdec_act_make_layout(int32_t reader_ptr)
{
    int32_t layout;
    struct retdec_act_property *properties = nullptr;
    uint32_t property_count = 0;

    layout = address(std::calloc(1u, 316u));
    if (layout == 0)
        return 0;
    field<int32_t>(layout) = address(kinoko_act_host_symbols()->layout_vtable);
    field<int32_t>(layout + 4) = address(kinoko_act_host_symbols()->layout_sprite_vtable);
    field<float>(layout + 0x104) = 1.0f;
    field<float>(layout + 0x108) = 1.0f;
    field<float>(layout + 0x10c) = 1.0f;
    field<float>(layout + 0x11c) = 1.0f;
    field<int32_t>(layout + 0x120) = 1;
    field<int32_t>(layout + 0x124) = 255;
    field<int32_t>(layout + 0x128) = 255;
    field<int32_t>(layout + 0x12c) = 255;
    if (!retdec_act_read_properties(reader_ptr, &properties,
                                    &property_count)) {
        std::free(pointer<void>(layout));
        return 0;
    }
    retdec_act_apply_layout(layout, properties, property_count);
    retdec_act_free_properties(properties, property_count);
    return layout;
}

int32_t retdec_act_make_map_layout(int32_t reader_ptr)
{
    int32_t layout;
    struct retdec_act_property *properties = nullptr;
    uint32_t property_count = 0;

    layout = address(std::calloc(1u, 464u));
    if (layout == 0)
        return 0;
    field<int32_t>(layout) = address(kinoko_act_host_symbols()->map_layout_vtable);
    field<int32_t>(layout + 4) = address(kinoko_act_host_symbols()->map_view_vtable);
    field<int32_t>(layout + 240) = 0x7fffffff;
    field<int32_t>(layout + 244) = 0x7fffffff;
    field<float>(layout + 320) = 1.0f;
    field<float>(layout + 324) = 1.0f;
    field<int32_t>(layout + 328) = 1;
    field<int32_t>(layout + 452) = -1;
    if (!retdec_act_read_properties(reader_ptr, &properties,
                                    &property_count)) {
        std::free(pointer<void>(layout));
        return 0;
    }
    retdec_act_apply_map_layout(layout, properties, property_count);
    retdec_act_free_properties(properties, property_count);
    return layout;
}

void retdec_act_free_map_records(int32_t layout)
{
    int32_t begin;
    int32_t end;

    if (layout == 0)
        return;
    begin = field<int32_t>(layout + 264);
    end = field<int32_t>(layout + 268);
    (void)end;
    /* C2DMapLayout stores CActMapChip records inline in one vector. */
    if (begin != 0)
        std::free(pointer<void>(begin));
    field<int32_t>(layout + 264) = 0;
    field<int32_t>(layout + 268) = 0;
    field<int32_t>(layout + 272) = 0;
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
    field<int32_t>(layout + 264) = 0;
    field<int32_t>(layout + 268) = 0;
    field<int32_t>(layout + 272) = 0;
    if (count == 0)
        return 1;

    if (serialized_size > 0x20u ||
        count > UINT32_MAX / 0x20u)
        return 0;
    records = (unsigned char *)std::calloc((size_t)count, 0x20u);
    if (records == nullptr)
        return 0;
    read_size = serialized_size;
    for (index = 0; index < count; ++index) {
        unsigned char *record = records + (size_t)index * 0x20u;
        if (read_size != 0 &&
            !retdec_reader_read_exact(reader_ptr, record, read_size)) {
            std::free(records);
            return 0;
        }
        *(uint32_t *)(void *)(record + 0x14) = index;
        *(uint8_t *)(void *)(record + 0x18) = 1;
        *(float *)(void *)(record + 0x1c) = 1.0f;
    }
    field<int32_t>(layout + 264) =
        address(records);
    field<int32_t>(layout + 268) =
        address((records + (size_t)count * 0x20u));
    field<int32_t>(layout + 272) =
        address((records + (size_t)count * 0x20u));
    retdec_trace_i32("act:map-record-count", (int32_t)count);
    retdec_trace_i32("act:map-record-size", (int32_t)serialized_size);
    return 1;
}

int32_t retdec_act_load_key(int32_t key, int32_t reader_ptr,
                                   int32_t version)
{
    struct retdec_act_property *properties = nullptr;
    uint32_t property_count = 0;
    uint8_t has_layout;
    uint32_t layout_type;
    int32_t layout;

    (void)version;
    if (!retdec_act_read_properties(reader_ptr, &properties,
                                    &property_count)) {
        retdec_trace("act:key-properties-failed");
        return 0;
    }
    for (uint32_t index = 0; index < property_count; ++index) {
        if (std::strcmp(properties[index].name, "scriptFunction") == 0)
            retdec_act_assign_string(key, 8, &properties[index]);
    }
    retdec_act_free_properties(properties, property_count);
    if (!retdec_act_read_u8(reader_ptr, &has_layout)) {
        retdec_trace("act:key-layout-flag-failed");
        return 0;
    }
    if (has_layout == 0)
        return 1;
    if (!retdec_act_read_u32(reader_ptr, &layout_type) ||
        (layout_type != 0x655cd5b0u &&
         layout_type != 0xc9ca5c20u)) {
        retdec_trace_i32("act:unsupported-layout", (int32_t)layout_type);
        return 0;
    }
    if (layout_type == 0xc9ca5c20u) {
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
    int32_t key;

    key = address(std::calloc(1u, 36u));
    if (key == 0)
        return 0;
    field<int32_t>(key) = address(kinoko_act_host_symbols()->key_vtable);
    field<int32_t>(key + 24) = 0;
    field<int32_t>(key + 28) = 15;
    field<uint8_t>(key + 8) = 0;
    if (!retdec_act_load_key(key, reader_ptr, version)) {
        std::free(pointer<void>(key));
        return 0;
    }
    return key;
}

int32_t retdec_act_load_layer(int32_t layer, int32_t reader_ptr,
                                     int32_t version)
{
    struct retdec_act_property *properties = nullptr;
    uint32_t property_count = 0;
    uint32_t count;
    uint32_t index;
    uint32_t type;
    int32_t key;

    if (!retdec_act_read_properties(reader_ptr, &properties,
                                    &property_count)) {
        retdec_trace("act:layer-properties-failed");
        return 0;
    }
    retdec_act_apply_layer(layer, properties, property_count);
    retdec_act_free_properties(properties, property_count);
    retdec_trace_squirrel_name("act:layer-name",
                               address(retdec_std_string_data(
                                   layer + 112)));
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
            retdec_trace("act:layer-key-load-failed");
            return 0;
        }
        retdec_trace_squirrel_name(
            "act:key-script", address(retdec_std_string_data(
                key + 8)));
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
    /* The second layer list is empty in Logo.act.  Its object types are not
       needed to load a 2D title layer, but reject rather than desynchronize. */
    if (count != 0) {
        retdec_trace_i32("act:layer-extra-count", (int32_t)count);
        return 0;
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
    int32_t reader_slot = 0;
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
    if (!function_407370(address(&reader_slot), file_name)) {
        retdec_trace_squirrel_name("mcd:open-failed",
                                   address(file_name));
        return 0;
    }

    if (!retdec_reader_read_exact(reader_slot, &magic, sizeof(magic)) ||
        magic != 0x434d4432u ||
        !retdec_reader_read_exact(reader_slot, &version, sizeof(version)) ||
        version > 1u ||
        !retdec_reader_read_exact(reader_slot, &payload_offset,
                                  sizeof(payload_offset)) ||
        (payload_offset != 0 &&
         !retdec_reader_seek_relative(reader_slot, payload_offset)) ||
        !retdec_reader_read_exact(reader_slot, &chip_count,
                                  sizeof(chip_count)) ||
        !retdec_reader_read_exact(reader_slot, &record_size,
                                  sizeof(record_size)) ||
        chip_count > 0x10000u || record_size > 48u) {
        retdec_trace("mcd:header-failed");
        retdec_destroy_reader(pointer<int32_t>(reader_slot));
        return 0;
    }

    data = (struct retdec_mcd_data *)std::calloc(1u, sizeof(*data));
    if (data == nullptr) {
        retdec_destroy_reader(pointer<int32_t>(reader_slot));
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
             !retdec_reader_read_exact(reader_slot, record, record_size)) ||
            !retdec_reader_read_exact(reader_slot, &next_record,
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

    if (!retdec_reader_read_exact(reader_slot, &texture_count,
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

        if (!retdec_reader_read_exact(reader_slot, &texture_id,
                                      sizeof(texture_id)) ||
            !retdec_reader_read_exact(reader_slot, &name_length,
                                      sizeof(name_length)) ||
            name_length > 0x100000u)
            goto load_failed;
        name = (char *)std::malloc((size_t)name_length + 1u);
        if (name == nullptr)
            goto load_failed;
        if (name_length != 0 &&
            !retdec_reader_read_exact(reader_slot, name, name_length)) {
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

    retdec_destroy_reader(pointer<int32_t>(reader_slot));
    field<int32_t>(resource + 64) =
        address(data);
    retdec_string_assign_cstr(
        pointer<int32_t>(resource + 72), file_name);
    retdec_trace_i32("mcd:chip-count", (int32_t)chip_count);
    retdec_trace_i32("mcd:texture-count", (int32_t)texture_count);
    retdec_trace_i32("mcd:texture-loaded", (int32_t)loaded_texture_count);
    return 1;

load_failed:
    retdec_destroy_reader(pointer<int32_t>(reader_slot));
    retdec_mcd_free(data);
    retdec_trace("mcd:load-failed");
    return 0;
}

int32_t retdec_act_make_resource(int32_t reader_ptr, uint32_t type)
{
    int32_t resource;
    struct retdec_act_property *properties = nullptr;
    uint32_t property_count = 0;

    if (type != 0xc6fdb98au && type != 0xfbaaf527u) {
        retdec_trace_i32("act:unsupported-resource", (int32_t)type);
        return 0;
    }
    resource = address(std::calloc(1u, 100u));
    if (resource == 0)
        return 0;
    field<int32_t>(resource) = type == 0xfbaaf527u
        ? address(kinoko_act_host_symbols()->chip_resource_vtable)
        : address(kinoko_act_host_symbols()->texture_resource_vtable);
    field<int32_t>(resource + 4) = -1;
    field<int32_t>(resource + 24) = 0;
    field<int32_t>(resource + 28) = 15;
    field<int32_t>(resource + 56) = 0;
    field<int32_t>(resource + 60) = 15;
    field<uint8_t>(resource + 8) = 0;
    field<uint8_t>(resource + 40) = 0;
    if (type == 0xc6fdb98au) {
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
    if (!retdec_act_read_properties(reader_ptr, &properties,
                                    &property_count)) {
        retdec_trace("act:resource-properties-failed");
        std::free(pointer<void>(resource));
        return 0;
    }
    if (type == 0xfbaaf527u)
        retdec_act_apply_chip_resource(resource, properties, property_count);
    else
        retdec_act_apply_resource(resource, properties, property_count);
    retdec_act_free_properties(properties, property_count);
    if (type == 0xc6fdb98au) {
        const char *texture_name = retdec_std_string_data(resource + 40);
        int32_t texture_handle = 0;
        if (texture_name != nullptr && *texture_name != 0)
            texture_handle = retdec_load_act_texture(texture_name);
        field<int32_t>(resource + 0x44) = texture_handle;
        retdec_trace_i32("act:resource-handle", texture_handle);
    } else {
        const char *chip_file = retdec_std_string_data(resource + 36);
        if (!retdec_act_load_mcd(resource, chip_file)) {
            retdec_trace("act:chip-resource-load-failed");
            retdec_destroy_cact_resource(resource);
            return 0;
        }
        retdec_trace_squirrel_name(
            "act:chip-resource-file", address(chip_file));
        retdec_trace("act:chip-resource-loaded");
    }
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
    if (field<int32_t>(layout + 332) != 0)
        std::free(pointer<void>(field<int32_t>(layout + 332)));
    field<int32_t>(layout + 332) = 0;
    field<int32_t>(layout + 336) = 0;
    field<int32_t>(layout + 340) = 0;
    field<int32_t>(layout + 380) = 0;
    retdec_trace_i32("map-layout:bind-layer", layer);
    retdec_trace_i32("map-layout:bind-resource", resource);
    retdec_trace_i32("map-layout:mcd", resource != 0
                     ? field<int32_t>(resource + 64) : 0);
    return 0;
}

int32_t retdec_act_bind_layouts(int32_t act)
{
    int32_t layer_begin;
    int32_t layer_end;
    int32_t resource_begin;
    int32_t resource_end;
    int32_t layer_count;
    int32_t resource_count;
    int32_t layer_index;
    int32_t resource_index;
    int32_t layer;
    int32_t resource;
    int32_t resource_id;
    int32_t layer_sentinel;
    int32_t node;
    uint32_t trace_count = 0;

    if (act == 0)
        return 0;
    layer_begin = field<int32_t>(act + 208);
    layer_end = field<int32_t>(act + 212);
    resource_begin = field<int32_t>(act + 224);
    resource_end = field<int32_t>(act + 228);
    if (layer_end < layer_begin || resource_end < resource_begin)
        return 0;
    layer_count = (layer_end - layer_begin) / 4;
    resource_count = (resource_end - resource_begin) / 4;

    for (layer_index = 0; layer_index < layer_count; ++layer_index) {
        layer = field<int32_t>(layer_begin + layer_index * 4);
        if (layer == 0)
            continue;
        resource_id = field<int32_t>(layer + 0x60);
        resource = 0;
        for (resource_index = 0; resource_index < resource_count;
             ++resource_index) {
            int32_t candidate = field<int32_t>(resource_begin + resource_index * 4);
            if (candidate != 0 &&
                field<int32_t>(candidate + 4) == resource_id) {
                resource = candidate;
                break;
            }
        }
        field<int32_t>(layer + 0x64) = resource;
        layer_sentinel = field<int32_t>(layer + 0xb4);
        if (layer_sentinel == 0)
            continue;
        node = field<int32_t>(layer_sentinel);
        while (node != 0 && node != layer_sentinel) {
            int32_t key = field<int32_t>(node + 8);
            int32_t layout = key == 0 ? 0 :
                field<int32_t>(key + 4);
            if (layout != 0) {
                if (field<int32_t>(layout) ==
                        address(kinoko_act_host_symbols()->map_layout_vtable))
                    retdec_c2dmaplayout_set_layer_impl(layout, layer);
                else
                    retdec_c2dlayout_set_layer_impl(layout, layer);
                if (trace_count++ < 32u) {
                    retdec_trace_i32("act:layout", layout);
                    retdec_trace_i32("act:layout-layer", layer);
                    retdec_trace_i32("act:layout-texture",
                                     field<int32_t>(layout + 0x134));
                }
            }
            node = field<int32_t>(node);
        }
    }
    return 1;
}

int32_t retdec_act_prepare_vector(int32_t object_ptr,
                                         uint32_t begin_offset,
                                         uint32_t end_offset,
                                         uint32_t capacity_offset,
                                         uint32_t count)
{
    int32_t *values;

    if (count > 0x10000u) {
        return 0;
    }
    if (count == 0) {
        field<int32_t>(object_ptr + (int32_t)begin_offset) = 0;
        field<int32_t>(object_ptr + (int32_t)end_offset) = 0;
        field<int32_t>(object_ptr + (int32_t)capacity_offset) = 0;
        return 1;
    }
    values = (int32_t *)std::calloc((size_t)count, sizeof(*values));
    if (values == nullptr)
        return 0;
    field<int32_t>(object_ptr + (int32_t)begin_offset) =
        address(values);
    field<int32_t>(object_ptr + (int32_t)end_offset) =
        address(values);
    field<int32_t>(object_ptr + (int32_t)capacity_offset) =
        address((values + count));
    return 1;
}

int32_t retdec_act_load(int32_t this_ptr, int32_t reader_ptr,
                               int32_t version)
{
    struct retdec_act_property *properties = nullptr;
    uint32_t property_count = 0;
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
    if (!retdec_act_read_properties(reader_ptr, &properties,
                                    &property_count)) {
        retdec_trace("act:cact-properties-failed");
        return 0;
    }
    retdec_act_apply_cact(this_ptr, properties, property_count);
    retdec_act_free_properties(properties, property_count);
    if (!retdec_act_load_script(this_ptr + 100, reader_ptr)) {
        retdec_trace("act:cact-script-failed");
        return 0;
    }
    if (!retdec_act_read_u32(reader_ptr, &layer_count) ||
        !retdec_act_prepare_vector(this_ptr, 208, 212, 216, layer_count)) {
        retdec_trace("act:layer-vector-failed");
        return 0;
    }
    layers = pointer<int32_t>(field<int32_t>(this_ptr + 208));
    for (index = 0; index < layer_count; ++index) {
        if (!retdec_act_read_u32(reader_ptr, &type) ||
            type != 0x2618cf18u) {
            retdec_trace_i32("act:unsupported-layer", (int32_t)type);
            return 0;
        }
        layer = retdec_act_make_layer();
        if (layer == 0 || !retdec_act_load_layer(layer, reader_ptr, version)) {
            retdec_trace("act:layer-load-failed");
            return 0;
        }
        layers[index] = layer;
        field<int32_t>(this_ptr + 212) += 4;
        if (index < 8) {
            retdec_trace_i32("act:layer-id",
                             field<int32_t>(layer + 0x68));
            retdec_trace_i32("act:layer-resource",
                             field<int32_t>(layer + 0x60));
        }
    }
    if (!retdec_act_read_u32(reader_ptr, &resource_count) ||
        !retdec_act_prepare_vector(this_ptr, 224, 228, 232,
                                   resource_count)) {
        retdec_trace("act:resource-vector-failed");
        return 0;
    }
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
        if (index < 8) {
            retdec_trace_i32("act:resource-id",
                             field<int32_t>(resource + 4));
            retdec_trace_i32("act:resource-texture",
                             field<int32_t>(resource + 40));
        }
    }
    retdec_trace_i32("act:loaded-layers", (int32_t)layer_count);
    retdec_trace_i32("act:loaded-resources", (int32_t)resource_count);
    if (!retdec_act_bind_layouts(this_ptr)) {
        retdec_trace("act:layout-bind-failed");
        return 0;
    }
    return 1;
}
