#include "kinoko/act_ownership.hpp"
#include "kinoko/act_document_association.hpp"
#include "kinoko/act_script_payload.hpp"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/act_map_records.hpp"
#include "kinoko/act_key_records.hpp"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_mesh.hpp"
#include "kinoko/act_layout3d_io.h"
#include "kinoko/act_layout2d_io.h"
#include "kinoko/act_resource_io.h"
#include "kinoko/act_resource_records_io.hpp"
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
int32_t kinoko_act_read_u8(int32_t reader_ptr, uint8_t *value)
{
    return kinoko_reader_read_exact_abi(reader_ptr, value, 1);
}

int32_t kinoko_act_read_u32(int32_t reader_ptr, uint32_t *value)
{
    return kinoko_reader_read_exact_abi(reader_ptr, value, sizeof(*value));
}

int32_t kinoko_act_load_script(int32_t object_ptr, int32_t reader_ptr)
{
    if (!kinoko_act_read_script_properties(object_ptr, reader_ptr)) {
        kinoko_trace("act:script-properties-failed");
        return 0;
    }
    uint32_t raw_size = 0;
    if (!kinoko_act_read_u32(reader_ptr, &raw_size) || raw_size > 0x1000000u) {
        kinoko_trace("act:script-size-failed");
        return 0;
    }
    auto raw_data = std::unique_ptr<unsigned char, decltype(&std::free)>(
        static_cast<unsigned char *>(std::malloc(raw_size ? raw_size : 1u)), &std::free);
    if (!raw_data) {
        kinoko_trace("act:script-alloc-failed");
        return 0;
    }
    if (raw_size && !kinoko_reader_read_exact_abi(reader_ptr, raw_data.get(), raw_size)) {
        kinoko_trace("act:script-data-failed");
        return 0;
    }
    kinoko::act::ScriptPayloadView script(pointer<void>(object_ptr));
    std::free(script.get(&kinoko::act::ScriptPayloadRecord::bytes));
    script.set(&kinoko::act::ScriptPayloadRecord::bytes, static_cast<void *>(raw_data.release()));
    script.set(&kinoko::act::ScriptPayloadRecord::size, raw_size);
    script.set(&kinoko::act::ScriptPayloadRecord::loaded, uint8_t{1});
    return 1;
}


int32_t kinoko_construct_c2dlayout(int32_t layout) {
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

int32_t kinoko_act_make_layout(int32_t reader_ptr)
{
    auto layout = std::unique_ptr<KinokoActLayout, decltype(&std::free)>(
        static_cast<KinokoActLayout *>(std::calloc(1u, sizeof(kinoko::act::Layout2DRecord))),
        &std::free);
    if (!layout || !kinoko_construct_c2dlayout(address(layout.get()))) return 0;
    // 42C030's holder is still an integer ABI slot; the layout is borrowed.
    if (!kinoko_act_read_layout2d_properties(layout.get(), &reader_ptr, 1)) return 0;
    return address(layout.release());
}

int32_t kinoko_act_make_map_layout(int32_t reader_ptr)
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

void kinoko_act_free_map_records(int32_t layout)
{
    if (layout) {
        kinoko::act::MapLayoutView record(pointer<void>(layout));
        kinoko_native_buffer_destroy(address(record.bytes(&kinoko::act::MapLayoutRecord::records_begin)));
    }
}

int32_t kinoko_act_read_map_records(int32_t layout,
                                           int32_t reader_ptr)
{
    uint32_t count;
    uint32_t serialized_size;
    uint32_t read_size;
    uint32_t index;
    unsigned char *records;

    if (layout == 0 ||
        !kinoko_act_read_u32(reader_ptr, &count) ||
        !kinoko_act_read_u32(reader_ptr, &serialized_size) ||
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
            !kinoko_reader_read_exact_abi(reader_ptr, record, read_size)) {
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

    kinoko_trace_i32("act:map-record-count", (int32_t)count);
    kinoko_trace_i32("act:map-record-size", (int32_t)serialized_size);
    return 1;
}

int32_t kinoko_act_load_key(int32_t key, int32_t reader_ptr,
                                   int32_t version)
{
    uint8_t has_layout;
    uint32_t layout_type = 0;
    KinokoActLayout *layout = nullptr;

    if (!key || version != 1 || !kinoko_act_read_key_properties_typed(pointer<KinokoActKey>(key),
            pointer<KinokoArchiveReader>(reader_ptr))) {
        kinoko_trace("act:key-properties-failed");
        return 0;
    }
    if (!kinoko_act_read_u8(reader_ptr, &has_layout)) {
        kinoko_trace("act:key-layout-flag-failed");
        return 0;
    }
    if (has_layout == 0)
        return 1;
    if (!kinoko_act_read_u32(reader_ptr, &layout_type) ||
        (layout_type != 0x655cd5b0u &&
         layout_type != 0xc9ca5c20u && layout_type != 0x9e695d47u &&
         layout_type != kinoko::mesh::layout_type())) {
        kinoko_trace_i32("act:unsupported-layout", (int32_t)layout_type);
        return 0;
    }
    if(layout_type==kinoko::mesh::layout_type()) {
        layout = reinterpret_cast<KinokoActLayout *>(kinoko::mesh::create_layout());
        if (layout && !kinoko_act_read_layout3d_properties(layout, &reader_ptr, version)) {
            std::free(layout);
            layout = nullptr;
        }
    } else if(layout_type==0x9e695d47u) {
        // Original Boost hash of .?AVCStringLayout@@; use the genuine native
        // reader through its recovered holder/version ABI.
        layout = static_cast<KinokoActLayout *>(std::calloc(1, sizeof(kinoko::act::StringLayoutRecord)));
        if (layout) {
            kinoko_construct_string_layout(address(layout));
            if (!kinoko_string_read_properties(reinterpret_cast<KinokoStringLayout *>(layout),
                    &reader_ptr, version)) {
                kinoko_clear_string_layout(address(layout));
                std::free(layout);
                layout = nullptr;
            }
        }
    } else if (layout_type == 0xc9ca5c20u) {
        layout = pointer<KinokoActLayout>(kinoko_act_make_map_layout(reader_ptr));
        if (layout && !kinoko_act_read_map_records(address(layout), reader_ptr)) {
            kinoko_act_free_map_records(address(layout));
            std::free(layout);
            layout = nullptr;
        }
    } else {
        layout = pointer<KinokoActLayout>(kinoko_act_make_layout(reader_ptr));
    }
    if (!layout) return 0;
    kinoko::act::KeyView(pointer<void>(key)).set(&kinoko::act::KeyRecord::layout, layout);
    return 1;
}

int32_t kinoko_act_make_key(int32_t reader_ptr, int32_t version)
{
    auto destroy = [](KinokoActKey *key) { kinoko_destroy_cact_key(address(key)); };
    std::unique_ptr<KinokoActKey, decltype(destroy)> key(
        static_cast<KinokoActKey *>(std::calloc(1u, sizeof(kinoko::act::KeyRecord))), destroy);
    if (!key) return 0;
    kinoko::act::KeyView record(key.get());
    record.set(&kinoko::act::KeyRecord::methods, kinoko_act_host_symbols()->key_vtable);
    auto name = record.view(&kinoko::act::KeyRecord::script_name);
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    if (!kinoko_act_load_key(address(key.get()), reader_ptr, version)) return 0;
    return address(key.release());
}

int32_t kinoko_act_load_layer(int32_t layer, int32_t reader_ptr,
                                     int32_t version)
{
    uint32_t count;
    uint32_t index;
    uint32_t type;
    kinoko::native::RecordView<kinoko::act::LayerKeys> layer_record(pointer<void>(layer));

    if (!layer || version != 1 || !kinoko_act_read_layer_properties_typed(pointer<KinokoActLayer>(layer),
            pointer<KinokoArchiveReader>(reader_ptr))) {
        kinoko_trace("act:layer-properties-failed");
        return 0;
    }
    kinoko_trace_squirrel_name("act:layer-name",
                               address(kinoko_string_data(layer_record.bytes(&kinoko::act::LayerKeys::name))));
    if (!kinoko_act_read_u32(reader_ptr, &count) || count > 0x10000u) {
        kinoko_trace("act:layer-key-count-failed");
        return 0;
    }
    for (index = 0; index < count; ++index) {
        if (!kinoko_act_read_u32(reader_ptr, &type) ||
            type != 0xd933304du) {
            kinoko_trace_i32("act:unsupported-key", (int32_t)type);
            return 0;
        }
        auto *key = pointer<KinokoActKey>(kinoko_act_make_key(reader_ptr, version));
        if (!key || !kinoko_act_append_list(layer + 0xb4, address(key))) {
            kinoko_destroy_cact_key(address(key));
            kinoko_trace("act:layer-key-load-failed");
            return 0;
        }
        // 41F8B9 binds every newly read layout to its containing layer before
        // the next key. Resource association may happen later during ACT load.
        auto *layout = kinoko::act::KeyView(key).get(&kinoko::act::KeyRecord::layout);
        if (layout) kinoko_call_thiscall1_result(layout,
            field<void*>(field<int32_t>(address(layout)) + 24), layer);
        kinoko_trace_squirrel_name(
            "act:key-script", address(kinoko_string_data(kinoko::act::KeyView(key).bytes(&kinoko::act::KeyRecord::script_name))));
        kinoko_trace_i32("act:key-layout", address(layout));
        if (layout && field<int32_t>(address(layout)) ==
                address(kinoko_act_host_symbols()->map_layout_vtable)) {
            const int32_t key_layout = address(layout);
            int32_t key_begin = field<int32_t>(key_layout + 264);
            int32_t key_end = field<int32_t>(key_layout + 268);
            kinoko_trace_i32("act:key-map-record-count",
                             key_begin != 0 && key_end >= key_begin
                                 ? (int32_t)((key_end - key_begin) / 0x20)
                                 : 0);
        }
        layer_record.set(&kinoko::act::LayerKeys::key_count,
            layer_record.get(&kinoko::act::LayerKeys::key_count) + 1);
    }
    if (!kinoko_act_read_u32(reader_ptr, &count) || count > 0x10000u) {
        kinoko_trace("act:layer-extra-count-failed");
        return 0;
    }
    // 41F800's second factory loop loads CActTimeLine, whose raw RTTI name
    // .?AVCActTimeLine@@ hashes to 9902F2C0 with the original Boost algorithm.
    for (index = 0; index < count; ++index) {
        if (!kinoko_act_read_u32(reader_ptr, &type) || type != 0x9902f2c0u) {
            kinoko_trace_i32("act:unsupported-timeline", (int32_t)type);
            return 0;
        }
        const auto timeline = kinoko_act_new_timeline();
        if (!timeline || !kinoko_act_load_timeline(timeline, reader_ptr, version) ||
            !kinoko_act_append_list(
                address(layer_record.bytes(&kinoko::act::LayerKeys::timeline_head)), timeline)) {
            kinoko_destroy_cact_key(timeline);
            return 0;
        }
        layer_record.set(&kinoko::act::LayerKeys::extra_count,
            layer_record.get(&kinoko::act::LayerKeys::extra_count) + 1);
    }
    return kinoko_act_load_script(layer + 0xcc, reader_ptr);
}

uint32_t kinoko_mcd_u32(const unsigned char *bytes)
{
    uint32_t value;

    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

int16_t kinoko_mcd_i16(const unsigned char *bytes)
{
    int16_t value;

    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

struct kinoko_mcd_chip *kinoko_mcd_find_chip(
    struct kinoko_mcd_data *data, uint32_t chip_id)
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

struct kinoko_mcd_texture *kinoko_mcd_find_texture(
    struct kinoko_mcd_data *data, uint32_t texture_id)
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

void kinoko_mcd_free(struct kinoko_mcd_data *data)
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

namespace {
// MCD buffers and acquired texture handles belong to this object until
// publication into a chip resource. A failure releases both together.
using MCDOwner = std::unique_ptr<kinoko_mcd_data, decltype(&kinoko_mcd_free)>;
using ArchiveOwner = std::unique_ptr<KinokoArchiveReader, decltype(&kinoko_reader_close)>;
int32_t load_chip_archive(KinokoActResource *resource, const char *file_name) {
    if (!resource || !file_name || !*file_name) return 0;
    KinokoArchiveReader *opened = nullptr;
    if (!kinoko_reader_open(&opened, file_name)) {
        kinoko_trace_squirrel_name("mcd:open-failed", address(file_name));
        return 0;
    }
    ArchiveOwner reader(opened, &kinoko_reader_close);
    uint32_t magic = 0, version = 0, payload_offset = 0;
    uint32_t chip_count = 0, record_size = 0, texture_count = 0;
    auto read_u32 = [&reader](uint32_t &value) {
        return kinoko_reader_read_exact(reader.get(), &value, sizeof(value)) != 0;
    };
    if (!read_u32(magic) || magic != 0x434d4432u ||
        !read_u32(version) || version > 1u ||
        !read_u32(payload_offset) ||
        (payload_offset && !kinoko_reader_seek_relative(reader.get(), payload_offset)) ||
        !read_u32(chip_count) || !read_u32(record_size) ||
        chip_count > 0x10000u || record_size > 48u) {
        kinoko_trace("mcd:header-failed");
        return 0;
    }
    MCDOwner data(static_cast<kinoko_mcd_data *>(std::calloc(1u, sizeof(kinoko_mcd_data))),
                  &kinoko_mcd_free);
    if (!data) return 0;
    data->chip_count = chip_count;
    if (chip_count) {
        data->chips = static_cast<kinoko_mcd_chip *>(
            std::calloc(chip_count, sizeof(kinoko_mcd_chip)));
        if (!data->chips) { kinoko_trace("mcd:load-failed"); return 0; }
    }
    for (uint32_t index = 0; index < chip_count; ++index) {
        unsigned char record[48]{};
        uint32_t next_record = 0;
        if ((record_size && !kinoko_reader_read_exact(reader.get(), record, record_size)) ||
            !read_u32(next_record)) {
            kinoko_trace("mcd:load-failed"); return 0;
        }
        auto &chip = data->chips[index];
        chip.chip_id = kinoko_mcd_u32(record);
        std::memcpy(chip.bytes, record, sizeof(record));
        if (chip.chip_id >= 0x8d0u && chip.chip_id <= 0x920u) {
            kinoko_trace_i32("mcd:chip-index", static_cast<int32_t>(index));
            kinoko_trace_i32("mcd:chip-id", static_cast<int32_t>(chip.chip_id));
        }
    }
    if (!read_u32(texture_count) || texture_count > 0x10000u) {
        kinoko_trace("mcd:load-failed"); return 0;
    }
    data->texture_count = texture_count;
    if (texture_count) {
        data->textures = static_cast<kinoko_mcd_texture *>(
            std::calloc(texture_count, sizeof(kinoko_mcd_texture)));
        if (!data->textures) { kinoko_trace("mcd:load-failed"); return 0; }
    }
    uint32_t loaded_texture_count = 0;
    for (uint32_t index = 0; index < texture_count; ++index) {
        uint32_t texture_id = 0, name_length = 0;
        if (!read_u32(texture_id) || !read_u32(name_length) || name_length > 0x100000u) {
            kinoko_trace("mcd:load-failed"); return 0;
        }
        auto name = std::unique_ptr<char, decltype(&std::free)>(
            static_cast<char *>(std::malloc(static_cast<size_t>(name_length) + 1u)), &std::free);
        if (!name || (name_length &&
            !kinoko_reader_read_exact(reader.get(), name.get(), name_length))) {
            kinoko_trace("mcd:load-failed"); return 0;
        }
        name.get()[name_length] = 0;
        const int32_t handle = kinoko_load_act_texture(name.get());
        kinoko_trace_squirrel_name("mcd:texture-name", address(name.get()));
        kinoko_trace_i32("mcd:texture-id", static_cast<int32_t>(texture_id));
        kinoko_trace_i32("mcd:texture-handle", handle);
        if (handle > 0 && static_cast<uint32_t>(handle) < KINOKO_TEXTURE_CAPACITY) {
            kinoko_trace_i32("mcd:texture-width", kinoko_texture_slots[handle].width);
            kinoko_trace_i32("mcd:texture-height", kinoko_texture_slots[handle].height);
        }
        data->textures[index] = {texture_id, handle};
        if (handle) ++loaded_texture_count;
    }
    reader.reset(); // Original closes the stream before publishing data.
    kinoko::act::ChipResourceFields fields(resource);
    fields.set(&kinoko::act::ChipResourceRecord::data, data.release());
    kinoko_string_assign_cstr(reinterpret_cast<int32_t *>(
        fields.bytes(&kinoko::act::ChipResourceRecord::loaded_path)), file_name);
    kinoko_trace_i32("mcd:chip-count", static_cast<int32_t>(chip_count));
    kinoko_trace_i32("mcd:texture-count", static_cast<int32_t>(texture_count));
    kinoko_trace_i32("mcd:texture-loaded", static_cast<int32_t>(loaded_texture_count));
    return 1;
}
} // namespace
int32_t kinoko_act_load_mcd(int32_t resource, const char *file_name) {
    return load_chip_archive(pointer<KinokoActResource>(resource), file_name);
}

extern "C" int32_t __fastcall kinoko_method_unload_resource_texture(int32_t receiver, void *) {
    auto *resource = pointer<KinokoActResource>(receiver);
    if (!resource) return 0;
    kinoko::act::TextureResourceFields fields(resource);
    const auto handle = fields.get(&kinoko::act::TextureResourceRecord::texture);
    if (!kinoko_act_release_cloned_texture(receiver) &&
        !fields.get(&kinoko::act::TextureResourceRecord::borrows_texture) && handle)
        kinoko_texture_release(handle);
    fields.set(&kinoko::act::TextureResourceRecord::texture, int32_t{0});
    return 1;
}

extern "C" int32_t __fastcall kinoko_method_load_chip_resource(
    int32_t receiver, void *, const char *prefix) {
    auto *resource = pointer<KinokoActResource>(receiver);
    if (!resource) return 0;
    kinoko::act::ChipResourceFields fields(resource);
    const char *name = kinoko_string_data(
        fields.bytes(&kinoko::act::ChipResourceRecord::source_name));
    if (!name || !*name) return 0;
    try {
        // 42FB4E starts with an empty prefix; add a separator only when needed.
        std::string base(prefix ? prefix : "");
        if (!base.empty() && base.back() != '/' && base.back() != '\\') base += '/';
        const std::string path = base + name;
        auto destroy = [](KinokoActResource *value) {
            kinoko_destroy_cact_resource(address(value));
        };
        std::unique_ptr<KinokoActResource, decltype(destroy)> temporary(
            static_cast<KinokoActResource *>(std::calloc(1, sizeof(kinoko::act::ChipResourceRecord))),
            destroy);
        if (!temporary) return 0;
        kinoko::act::ChipResourceFields scratch(temporary.get());
        scratch.set(&kinoko::act::ChipResourceRecord::methods,
            kinoko_act_host_symbols()->chip_resource_vtable);
        auto base_name = scratch.view(&kinoko::act::ChipResourceRecord::name);
        auto source_name = scratch.view(&kinoko::act::ChipResourceRecord::source_name);
        auto loaded_path = scratch.view(&kinoko::act::ChipResourceRecord::loaded_path);
        base_name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
        source_name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
        loaded_path.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
        // Original loads into a temporary resource and replaces only on success.
        if (!load_chip_archive(temporary.get(), path.c_str())) return 0;
        kinoko_string_assign_cstr(reinterpret_cast<int32_t *>(
            fields.bytes(&kinoko::act::ChipResourceRecord::loaded_path)), base.c_str());
        if (kinoko_act_release_chip_data(receiver))
            kinoko_mcd_free(fields.get(&kinoko::act::ChipResourceRecord::data));
        fields.set(&kinoko::act::ChipResourceRecord::data,
            scratch.get(&kinoko::act::ChipResourceRecord::data));
        scratch.set(&kinoko::act::ChipResourceRecord::data,
            static_cast<kinoko_mcd_data *>(nullptr));
        return 1;
    } catch (...) { return 0; }
}

extern "C" int32_t __fastcall kinoko_method_load_resource_texture(
    int32_t receiver, void *, const char *prefix) {
    auto *resource = pointer<KinokoActResource>(receiver);
    if (!resource) return 0;
    kinoko::act::TextureResourceFields fields(resource);
    const char *name = kinoko_string_data(
        fields.bytes(&kinoko::act::TextureResourceRecord::texture_name));
    // 446C36 leaves the old handle in place for an empty texture name.
    if (!name || !*name) return 0;
    try {
        std::string path(prefix && *prefix ? prefix : "./");
        if (path.back() != '/' && path.back() != '\\') path += '/';
        kinoko_call_thiscall0(resource, field<void *>(field<int32_t>(receiver) + 44));
        fields.set(&kinoko::act::TextureResourceRecord::borrows_texture, uint8_t{0});
        path += name;
        // 431D80 joins prefix/name; 40E540 appends each suffix.
        for (const char *suffix : {".dds", ".bmp", ".png"}) {
            const auto candidate = path + suffix;
            const int32_t handle = kinoko_texture_acquire(candidate.c_str());
            fields.set(&kinoko::act::TextureResourceRecord::texture, handle);
            if (!handle) continue;
            const auto &slot = kinoko_texture_slots[handle];
            fields.set(&kinoko::act::TextureResourceRecord::width,
                       static_cast<int32_t>(slot.width));
            fields.set(&kinoko::act::TextureResourceRecord::height,
                       static_cast<int32_t>(slot.height));
            if (fields.get(&kinoko::act::TextureResourceRecord::auto_size)) {
                fields.set(&kinoko::act::TextureResourceRecord::source_x, 0.0f);
                fields.set(&kinoko::act::TextureResourceRecord::source_y, 0.0f);
                fields.set(&kinoko::act::TextureResourceRecord::source_width,
                           static_cast<float>(slot.width));
                fields.set(&kinoko::act::TextureResourceRecord::source_height,
                           static_cast<float>(slot.height));
            }
            return 1;
        }
    } catch (...) { return 0; }
    return 0;
}

int32_t kinoko_act_make_resource(int32_t reader_ptr, uint32_t type)
{
    if (type == kinoko::mesh::resource_type()) {
        auto *mesh = kinoko::mesh::create_resource();
        if (mesh && !kinoko::mesh::read_resource_properties(mesh, &reader_ptr, 1)) {
            kinoko::mesh::clear_resource(mesh);
            std::free(mesh);
            return 0;
        }
        return address(mesh);
    }
    // 449C50 registers the render target's raw RTTI name in the same factory.
    static const char target_name[] = ".?AVCActRenderTarget@@";
    static const auto target_type = static_cast<uint32_t>(kinoko_boost_hash_range(
        address(target_name), address(target_name + sizeof(target_name) - 1)));
    const bool render_target = type == target_type;
    const bool texture = type == 0xc6fdb98au || render_target;
    if (!texture && type != 0xfbaaf527u) {
        kinoko_trace_i32("act:unsupported-resource", static_cast<int32_t>(type));
        return 0;
    }
    auto destroy = [](KinokoActResource *value) {
        kinoko_destroy_cact_resource(address(value));
    };
    std::unique_ptr<KinokoActResource, decltype(destroy)> resource(
        static_cast<KinokoActResource *>(std::calloc(1, sizeof(kinoko::act::TextureResourceRecord))),
        destroy);
    if (!resource) return 0;
    auto name = kinoko::act::TextureResourceFields(resource.get()).view(
        &kinoko::act::TextureResourceRecord::name);
    name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    if (texture) {
        kinoko::act::TextureResourceFields fields(resource.get());
        fields.set(&kinoko::act::TextureResourceRecord::methods, render_target
            ? kinoko_act_host_symbols()->render_target_vtable
            : kinoko_act_host_symbols()->texture_resource_vtable);
        fields.set(&kinoko::act::TextureResourceRecord::id, int32_t{-1});
        auto image_name = fields.view(&kinoko::act::TextureResourceRecord::texture_name);
        image_name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
        fields.set(&kinoko::act::TextureResourceRecord::width, int32_t{256});
        fields.set(&kinoko::act::TextureResourceRecord::height, int32_t{256});
        fields.set(&kinoko::act::TextureResourceRecord::auto_size, uint8_t{1});
    } else {
        kinoko::act::ChipResourceFields fields(resource.get());
        fields.set(&kinoko::act::ChipResourceRecord::methods,
            kinoko_act_host_symbols()->chip_resource_vtable);
        fields.set(&kinoko::act::ChipResourceRecord::id, int32_t{-1});
        auto source = fields.view(&kinoko::act::ChipResourceRecord::source_name);
        auto loaded = fields.view(&kinoko::act::ChipResourceRecord::loaded_path);
        source.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
        loaded.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
        fields.set(&kinoko::act::ChipResourceRecord::unknown60, uint32_t{15});
    }
    const auto loaded = render_target
        ? kinoko_act_read_render_target_properties(resource.get(), &reader_ptr, 1)
        : !texture
        ? kinoko_act_read_chip_properties(resource.get(), &reader_ptr, 1)
        : kinoko_act_read_texture_properties(resource.get(), &reader_ptr, 1);
    if (!loaded) {
        kinoko_trace("act:resource-properties-failed");
        return 0;
    }
    if (type == 0xc6fdb98au) {
        // 446A84 clears auto-size even without a property block.
        kinoko::act::TextureResourceFields(resource.get()).set(
            &kinoko::act::TextureResourceRecord::auto_size, uint8_t{0});
    }
    // 428150 publishes the object now; 4289C0 loads actual resources later.
    return address(resource.release());
}

int32_t kinoko_c2dmaplayout_set_layer_impl(int32_t layout,
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
    kinoko_trace_i32("map-layout:bind-layer", layer);
    kinoko_trace_i32("map-layout:bind-resource", resource);
    kinoko_trace_i32("map-layout:mcd", resource != 0
                     ? field<int32_t>(resource + 64) : 0);
    return 0;
}

int32_t kinoko_act_prepare_vector(int32_t object_ptr,
                                         uint32_t begin_offset,
                                         uint32_t end_offset,
                                         uint32_t capacity_offset,
                                         uint32_t count)
{
    if (end_offset!=begin_offset+4 || capacity_offset!=begin_offset+8) return 0;
    return kinoko_act_array_prepare((void*)(uintptr_t)(object_ptr+begin_offset), count);
}

int32_t kinoko_act_load(int32_t this_ptr, int32_t reader_ptr,
                               int32_t version)
{
    uint32_t layer_count;
    uint32_t resource_count;
    uint32_t index;
    uint32_t type;

    if (this_ptr == 0 || reader_ptr == 0 || version != 1)
        return 0;
    if (!kinoko_act_read_document_properties_typed(pointer<KinokoActDocument>(this_ptr),
            pointer<KinokoArchiveReader>(reader_ptr))) {
        kinoko_trace("act:cact-properties-failed");
        return 0;
    }
    if (!kinoko_act_load_script(this_ptr + 100, reader_ptr)) {
        kinoko_trace("act:cact-script-failed");
        return 0;
    }
    kinoko::act::DocumentView view(pointer<void>(this_ptr));
    const auto layer_slot = address(view.bytes(&kinoko::act::DocumentRecord::layers));
    if (!kinoko_act_read_u32(reader_ptr, &layer_count) ||
        layer_count > 0x10000) {
        kinoko_trace("act:layer-vector-failed");
        return 0;
    }
    kinoko::act::DocumentLoadAssociations associations;
    auto *document = pointer<KinokoActDocument>(this_ptr);
    for (index = 0; index < layer_count; ++index) {
        if (!kinoko_act_read_u32(reader_ptr, &type) ||
            type != 0x2618cf18u) {
            kinoko_trace_i32("act:unsupported-layer", (int32_t)type);
            return 0;
        }
        std::unique_ptr<KinokoActLayer, kinoko::act::OwnedDeleter<KinokoActLayer>> pending(
            pointer<KinokoActLayer>(kinoko_act_make_layer()));
        auto *layer = pending.get();
        if (!layer || !kinoko_act_load_layer(address(layer), reader_ptr, version)) {
            kinoko_trace("act:layer-load-failed");
            return 0;
        }
        // 4295D0 appends; replacing the backing span loses previously owned
        // objects on a second load and exposes reserved slots after failure.
        kinoko_act_array_append((void*)(uintptr_t)(layer_slot), (void*)(uintptr_t)(address(layer)));
        pending.release(); // document owns it even if association insertion throws
        associations.add_layer(layer);
        if (index < 8) {
            kinoko::native::RecordView<kinoko::act::LayerAssociationRecord> loaded(layer);
            kinoko_trace_i32("act:layer-id",
                loaded.get(&kinoko::act::LayerAssociationRecord::layer_id));
            kinoko_trace_i32("act:layer-resource",
                loaded.get(&kinoko::act::LayerAssociationRecord::resource_id));
        }
    }
    // 428310..428346: resolve parents before even reading resource_count.
    associations.bind_loaded_parents(document, layer_count);
    const auto resource_slot = address(view.bytes(&kinoko::act::DocumentRecord::resources));
    if (!kinoko_act_read_u32(reader_ptr, &resource_count) ||
        resource_count > 0x10000) {
        kinoko_trace("act:resource-vector-failed");
        return 0;
    }
    associations.begin_resources();
    for (index = 0; index < resource_count; ++index) {
        if (!kinoko_act_read_u32(reader_ptr, &type)) {
            kinoko_trace("act:resource-type-failed");
            return 0;
        }
        std::unique_ptr<KinokoActResource, kinoko::act::OwnedDeleter<KinokoActResource>> pending(
            pointer<KinokoActResource>(kinoko_act_make_resource(reader_ptr, type)));
        auto *resource = pending.get();
        if (!resource) return 0;
        kinoko_act_array_append((void*)(uintptr_t)(resource_slot), (void*)(uintptr_t)(address(resource)));
        pending.release();
        associations.add_resource(resource);
        if (index < 8) {
            kinoko::native::RecordView<kinoko::act::ResourceIdentityRecord> identity(resource);
            kinoko_trace_i32("act:resource-id",
                identity.get(&kinoko::act::ResourceIdentityRecord::id));
            // The +40 word is diagnostic only; resource subclasses differ here.
            kinoko_trace_i32("act:resource-texture",
                field<int32_t>(address(resource) + 40));
        }
    }
    kinoko_trace_i32("act:loaded-layers", (int32_t)layer_count);
    kinoko_trace_i32("act:loaded-resources", (int32_t)resource_count);
    associations.bind_resources(document, layer_count);
    return 1;
}
