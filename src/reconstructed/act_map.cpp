#include "kinoko/native_buffer.h"
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

int32_t retdec_map_sprite_init(int32_t sprite, int32_t handle,
                                      const unsigned char *chip_bytes)
{
    uint32_t texture_width;
    uint32_t texture_height;
    int16_t left;
    int16_t top;
    int16_t width;
    int16_t height;
    float u0;
    float v0;
    float u1;
    float v1;
    uint32_t color = 0xffffffffu;

    if (sprite == 0 || chip_bytes == nullptr || handle <= 0 ||
        (uint32_t)handle >= KINOKO_TEXTURE_CAPACITY)
        return 0;
    texture_width = kinoko_texture_slots[(uint32_t)handle].width;
    texture_height = kinoko_texture_slots[(uint32_t)handle].height;
    if (texture_width == 0 || texture_height == 0)
        return 0;
    left = retdec_mcd_i16(chip_bytes + 8);
    top = retdec_mcd_i16(chip_bytes + 10);
    width = retdec_mcd_i16(chip_bytes + 12);
    height = retdec_mcd_i16(chip_bytes + 14);
    if (width <= 0 || height <= 0)
        return 0;

    std::memset(pointer<void>(sprite), 0, 232u);
    field<int32_t>(sprite + 4) = handle;
    field<float>(sprite + 120) = (float)texture_width;
    field<float>(sprite + 124) = (float)texture_height;
    u0 = (float)left / (float)texture_width;
    v0 = (float)top / (float)texture_height;
    u1 = (float)(left + width) / (float)texture_width;
    v1 = (float)(top + height) / (float)texture_height;
    field<float>(sprite + 28) = u0;
    field<float>(sprite + 32) = v0;
    field<float>(sprite + 56) = u1;
    field<float>(sprite + 60) = v0;
    field<float>(sprite + 84) = u0;
    field<float>(sprite + 88) = v1;
    field<float>(sprite + 112) = u1;
    field<float>(sprite + 116) = v1;

    field<float>(sprite + 140) = (float)width;
    field<float>(sprite + 156) = (float)height;
    field<float>(sprite + 164) = (float)width;
    field<float>(sprite + 168) = (float)height;
    std::memcpy(pointer<void>(sprite + 176),
           pointer<const void>(sprite + 128), 48u);
    field<uint32_t>(sprite + 24) = color;
    field<uint32_t>(sprite + 52) = color;
    field<uint32_t>(sprite + 80) = color;
    field<uint32_t>(sprite + 108) = color;
    return 1;
}

int32_t kinoko_map_update(
    int32_t layout, int32_t view_left, int32_t view_top,
    int32_t view_right, int32_t view_bottom)
{
    int32_t layer;
    int32_t resource;
    struct retdec_mcd_data *data;
    int32_t begin;
    int32_t end;
    uint32_t map_count;
    uint32_t index;
    uint32_t output_count = 0;
    int32_t render_block;
    float layer_x;
    float layer_y;
    float layer_z;
    int32_t position_layer;
    uint32_t position_guard;
    float scale;
    static volatile LONG trace_count;
    static volatile LONG sample_layer_count;
    static volatile LONG entry_sample_count;
    LONG trace_index;
    LONG sample_layer_index;
    LONG entry_sample_index;
    (void)view_right;
    (void)view_bottom;

    if (layout == 0)
        return -0x7fffbffb;
    trace_index = InterlockedIncrement(&trace_count);
    layer = field<int32_t>(layout + 312);
    resource = field<int32_t>(layout + 316);
    entry_sample_index = InterlockedIncrement(&entry_sample_count);
    if (entry_sample_index <= 8) {
        retdec_trace_i32("map:entry-layout", layout);
        retdec_trace_i32("map:entry-layer", layer);
        retdec_trace_i32("map:entry-resource", resource);
        if (layer != 0) {
            retdec_trace_squirrel_name(
                "map:entry-layer-name", address(retdec_std_string_data(layer + 112)));
            retdec_trace_i32("map:entry-visible",
                             field<int32_t>(layer + 0x8c));
        }
        retdec_trace_i32("map:entry-record-begin",
                         layout != 0 ? field<int32_t>(layout + 264) : 0);
        retdec_trace_i32("map:entry-record-end",
                         layout != 0 ? field<int32_t>(layout + 268) : 0);
        retdec_trace_i32("map:entry-record-count",
                         layout != 0 &&
                         field<int32_t>(layout + 268) >=
                         field<int32_t>(layout + 264)
                             ? (int32_t)((field<int32_t>(layout + 268) -
                                          field<int32_t>(layout + 264)) / 0x20)
                             : 0);
    }
    if (layer == 0 || resource == 0)
        return -0x7fffbffb;
    if (field<uint8_t>(layer + 0x8c) == 0)
        return 0;
    data = pointer<retdec_mcd_data>(field<int32_t>(resource + 64));
    if (data == nullptr)
        return -0x7fffbffb;

    kinoko_native_buffer_destroy(layout+332);
    field<int32_t>(layout + 380) = 0;

    begin = field<int32_t>(layout + 264);
    end = field<int32_t>(layout + 268);
    if (begin == 0 || end < begin)
        return 0;
    map_count = (uint32_t)((end - begin) / 0x20);
    if (map_count == 0 || map_count > UINT32_MAX / 232u)
        return 0;
    if(!kinoko_native_buffer_resize(layout+332,map_count*232u)) return -0x7fffbffb;
    render_block=field<int32_t>(layout+332);

    layer_x = 0.0f;
    layer_y = 0.0f;
    layer_z = 0.0f;
    position_layer = field<int32_t>(layout + 312);
    position_guard = 0;
    while (position_layer != 0 && position_guard++ < 64u) {
        layer_x += field<float>(position_layer + 0x90);
        layer_y += field<float>(position_layer + 0x94);
        layer_z += field<float>(position_layer + 0x98);
        position_layer = field<int32_t>(position_layer + 0x58);
    }
    scale = field<float>(layout + 324);
    if (scale == 0.0f)
        scale = 1.0f;
    sample_layer_index = InterlockedIncrement(&sample_layer_count);

    if (sample_layer_index <= 4) {
        int32_t layer_x_bits;
        int32_t layer_y_bits;
        int32_t layer_z_bits;
        int32_t scale_bits;
        uint32_t sample_index;

        std::memcpy(&layer_x_bits, &layer_x, sizeof(layer_x_bits));
        std::memcpy(&layer_y_bits, &layer_y, sizeof(layer_y_bits));
        std::memcpy(&layer_z_bits, &layer_z, sizeof(layer_z_bits));
        std::memcpy(&scale_bits, &scale, sizeof(scale_bits));
        retdec_trace_squirrel_name(
            "map:sample-layer", address(retdec_std_string_data(
                    field<int32_t>(layout + 312) + 112)));
        retdec_trace_i32("map:sample-layer-x", layer_x_bits);
        retdec_trace_i32("map:sample-layer-y", layer_y_bits);
        retdec_trace_i32("map:sample-layer-z", layer_z_bits);
        retdec_trace_i32("map:sample-scale", scale_bits);
        for (sample_index = 0;
             sample_index < map_count;
             ++sample_index) {
            int32_t sample_record = begin + (int32_t)sample_index * 0x20;
            struct retdec_mcd_chip *sample_chip = nullptr;
            struct retdec_mcd_texture *sample_texture = nullptr;

            retdec_trace_i32("map:sample-index", (int32_t)sample_index);
            if (sample_record == 0) {
                retdec_trace("map:sample-null-record");
                continue;
            }
            retdec_trace_i32("map:sample-chip-id",
                             (int32_t)retdec_mcd_u32(
                                 pointer<unsigned char>(sample_record)));
            retdec_trace_i32("map:sample-x",
                             field<int32_t>(sample_record + 4));
            retdec_trace_i32("map:sample-y",
                             field<int32_t>(sample_record + 8));
            retdec_trace_i32("map:sample-active",
                             field<int32_t>(sample_record + 24));
            retdec_trace_i32("map:sample-opacity-bits",
                             field<int32_t>(sample_record + 28));
            sample_chip = retdec_mcd_find_chip(
                data, retdec_mcd_u32(pointer<unsigned char>(sample_record)));
            if (sample_chip == nullptr) {
                retdec_trace("map:sample-chip-miss");
                continue;
            }
            retdec_trace_i32("map:sample-texture-id",
                             (int32_t)retdec_mcd_u32(sample_chip->bytes + 4));
            retdec_trace_i32("map:sample-left",
                             (int32_t)retdec_mcd_i16(sample_chip->bytes + 8));
            retdec_trace_i32("map:sample-top",
                             (int32_t)retdec_mcd_i16(sample_chip->bytes + 10));
            retdec_trace_i32("map:sample-width",
                             (int32_t)retdec_mcd_i16(sample_chip->bytes + 12));
            retdec_trace_i32("map:sample-height",
                             (int32_t)retdec_mcd_i16(sample_chip->bytes + 14));
            sample_texture = retdec_mcd_find_texture(
                data, retdec_mcd_u32(sample_chip->bytes + 4));
            retdec_trace_i32("map:sample-handle",
                             sample_texture != nullptr ? sample_texture->handle : 0);
        }
    }

    for (index = 0; index < map_count; ++index) {
        int32_t record = begin + (int32_t)index * 0x20;
        struct retdec_mcd_chip *chip;
        struct retdec_mcd_texture *texture;
        int32_t sprite;
        float x;
        float y;
        float z;
        float opacity;
        int32_t alpha;
        uint32_t color;

        if (record == 0 || field<uint8_t>(record + 24) == 0)
            continue;
        chip = retdec_mcd_find_chip(data,
                                    retdec_mcd_u32(pointer<unsigned char>(record)));
        if (chip == nullptr)
            continue;
        texture = retdec_mcd_find_texture(
            data, retdec_mcd_u32(chip->bytes + 4));
        if (texture == nullptr || texture->handle == 0)
            continue;
        sprite = render_block + (int32_t)output_count * 232;
        if (!retdec_map_sprite_init(sprite, texture->handle, chip->bytes))
            continue;
        x = (float)field<int32_t>(record + 4) -
            (float)view_left + layer_x;
        y = (float)field<int32_t>(record + 8) -
            (float)view_top + layer_y;
        z = layer_z;
        field<float>(sprite + 176) = x * scale;
        field<float>(sprite + 180) = y * scale;
        field<float>(sprite + 184) = z * scale;
        field<float>(sprite + 188) =
            (x + field<float>(sprite + 140)) * scale;
        field<float>(sprite + 192) = y * scale;
        field<float>(sprite + 196) = z * scale;
        field<float>(sprite + 200) = x * scale;
        field<float>(sprite + 204) =
            (y + field<float>(sprite + 156)) * scale;
        field<float>(sprite + 208) = z * scale;
        field<float>(sprite + 212) =
            (x + field<float>(sprite + 164)) * scale;
        field<float>(sprite + 216) =
            (y + field<float>(sprite + 168)) * scale;
        field<float>(sprite + 220) = z * scale;
        opacity = field<float>(record + 28) *
            field<float>(layout + 320);
        alpha = (int32_t)(opacity * 255.0f);
        if (alpha < 0)
            alpha = 0;
        if (alpha > 255)
            alpha = 255;
        color = ((uint32_t)alpha << 24) | 0x00ffffffu;
        field<uint32_t>(sprite + 24) = color;
        field<uint32_t>(sprite + 52) = color;
        field<uint32_t>(sprite + 80) = color;
        field<uint32_t>(sprite + 108) = color;
        if (sample_layer_index <= 4 && output_count < 8u) {
            retdec_trace_i32("map:sample-output-index",
                             (int32_t)output_count);
            retdec_trace_i32("map:sample-output-record", record);
            retdec_trace_i32("map:sample-output-handle", texture->handle);
            retdec_trace_i32("map:sample-output-x", field<int32_t>(record + 4));
            retdec_trace_i32("map:sample-output-y", field<int32_t>(record + 8));
        }
        ++output_count;
    }
    if (output_count == 0) {
        kinoko_native_buffer_destroy(layout+332);
        return 0;
    }
    field<int32_t>(layout + 332) = render_block;
    field<int32_t>(layout + 336) =
        render_block + (int32_t)output_count * 232;

    field<int32_t>(layout + 380) = (int32_t)output_count;
    if (trace_index <= 48) {
        retdec_trace_squirrel_name(
            "map:update-layer", address(retdec_std_string_data(
                field<int32_t>(layout + 312) + 112)));
        retdec_trace_i32("map:update-records", (int32_t)map_count);
        retdec_trace_i32("map:update-draw-records", (int32_t)output_count);
    }
    return 0;
}

int32_t kinoko_map_draw(int32_t layout,
                                             float x, float y)
{
    IDirect3DDevice9 *device;
    DWORD old_src_blend = 0;
    DWORD old_dest_blend = 0;
    DWORD old_blend_op = 0;
    DWORD old_alpha_blend = 0;
    int states_saved = 0;
    int32_t begin;
    int32_t count;
    int32_t index;
    int32_t result = 0;

    if (layout == 0 || field<int32_t>(layout + 312) == 0)
        return -0x7fffbffb;
    if (field<uint8_t>(field<int32_t>(layout + 312) + 140) == 0)
        return 0;
#if defined(RETDEC_DIAGNOSTIC_SKIP_BG2)
    {
        const char *map_layer_name = retdec_std_string_data(
            field<int32_t>(layout + 312) + 112);
        if (map_layer_name != nullptr && std::strcmp(map_layer_name, "bg2") == 0)
            return 0;
    }
#endif
    device = pointer<IDirect3DDevice9>(g678);
    if (device != nullptr &&
        SUCCEEDED(device->GetRenderState(D3DRS_SRCBLEND, &old_src_blend)) &&
        SUCCEEDED(device->GetRenderState(D3DRS_DESTBLEND, &old_dest_blend)) &&
        SUCCEEDED(device->GetRenderState(D3DRS_BLENDOP, &old_blend_op)) &&
        SUCCEEDED(device->GetRenderState(D3DRS_ALPHABLENDENABLE, &old_alpha_blend))) {
        states_saved = 1;
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
    /* C2DMapLayout::Draw enters the 2D pass with depth testing and depth
       writes disabled, so layers compose in their linked-list order. */
    if (device != nullptr && field<void*>(address(device)) != nullptr) {
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    }
    begin = field<int32_t>(layout + 332);
    count = field<int32_t>(layout + 380);
    if (begin != 0 && count > 0) {
        for (index = 0; index < count; ++index) {
            result = retdec_layout_submit_impl(begin + index * 232, x, y);
            if (result < 0)
                break;
        }
    }
    retdec_set_texture_stage(0, 0);
    if (states_saved) {
        device->SetRenderState(D3DRS_SRCBLEND, old_src_blend);
        device->SetRenderState(D3DRS_DESTBLEND, old_dest_blend);
        device->SetRenderState(D3DRS_BLENDOP, old_blend_op);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, old_alpha_blend);
    }
    return result;
}
