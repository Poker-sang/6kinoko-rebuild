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

int32_t retdec_c2dlayout_set_layer_impl(int32_t layout,
                                                int32_t layer)
{
    int32_t resource;
    int32_t handle;

    if (layout == 0 || layer == 0)
        return -0x7fffbffb;
    field<int32_t>(layout + 0x130) = layer;
    field<uint8_t>(layout + 0x138) = 0;
    /* CActLayerLayout is a pointer view over the active C2DLayout.  The
       original 42BA50 writes these aliases before layer properties are
       accessed through CActLayer's _get/_set tables. */
    field<int32_t>(layer + 4) = layout + 236;
    field<int32_t>(layer + 8) = layout + 240;
    field<int32_t>(layer + 12) = layout + 244;
    field<int32_t>(layer + 16) = layout + 248;
    field<int32_t>(layer + 20) = layout + 252;
    field<int32_t>(layer + 24) = layout + 256;
    field<int32_t>(layer + 28) = layout + 260;
    field<int32_t>(layer + 32) = layout + 264;
    field<int32_t>(layer + 36) = layout + 268;
    field<int32_t>(layer + 40) = layout + 272;
    field<int32_t>(layer + 44) = layout + 276;
    field<int32_t>(layer + 48) = layout + 280;
    field<int32_t>(layer + 52) = layout + 284;
    field<int32_t>(layer + 56) = layout + 288;
    field<int32_t>(layer + 60) = layout + 292;
    field<int32_t>(layer + 64) = layout + 296;
    field<int32_t>(layer + 68) = layout + 300;
    resource = field<int32_t>(layer + 0x64);
    handle = resource == 0 ? 0 :
        field<int32_t>(resource + 0x44);
    field<int32_t>(layout + 0x134) = handle;
    if (resource != 0 && handle != 0 &&
        field<uint8_t>(layout + 312) == 0) {
        field<float>(layout + 248) =
            field<float>(resource + 88) * 0.5f;
        field<float>(layout + 252) =
            field<float>(resource + 92) * 0.5f;
        /* cos_x/cos_y are serialized layout properties.  The original
           binding path does not replace them when a resource is already
           resolved; Fader relies on its zero origin for screen coverage. */
        field<uint8_t>(layout + 312) = 1;
    }
    retdec_trace_i32("layout:bind-layer", layer);
    retdec_trace_i32("layout:bind-resource", resource);
    retdec_trace_i32("layout:bind-texture", handle);
    return 0;
}

void retdec_c2dlayout_world_position(int32_t layer,
                                             float *x,
                                             float *y,
                                             float *z)
{
    uint32_t guard = 0;

    if (x == nullptr || y == nullptr || z == nullptr)
        return;
    *x = 0.0f;
    *y = 0.0f;
    *z = 0.0f;
    while (layer != 0 && guard++ < 64u) {
        *x += field<float>(layer + 0x90);
        *y += field<float>(layer + 0x94);
        *z += field<float>(layer + 0x98);
        layer = field<int32_t>(layer + 0x58);
    }
}

int32_t retdec_c2dlayout_update_impl(int32_t layout)
{
    int32_t layer;
    int32_t resource;
    int32_t handle;
    uint32_t texture_width = 0;
    uint32_t texture_height = 0;
    float source_x;
    float source_y;
    float source_width;
    float source_height;
    float scale_x;
    float scale_y;
    float world_x;
    float world_y;
    float world_z;
    float left;
    float top;
    float right;
    float bottom;
    float angle;
    uint32_t color;
    int32_t sprite;
    float *vertex;
    unsigned int alpha;
    int32_t red;
    int32_t green;
    int32_t blue;

    if (layout == 0)
        return -0x7fffbffb;
    layer = field<int32_t>(layout + 0x130);
    if (layer == 0)
        return -0x7fffbffb;
    if (field<uint8_t>(layer + 0x8c) == 0)
        return 0;
    resource = field<int32_t>(layer + 0x64);
    if (resource == 0)
        return -0x7fffbffb;
    handle = field<int32_t>(resource + 0x44);
    if (handle == 0) {
        const char *texture_name = retdec_std_string_data(resource + 40);
        if (texture_name != nullptr && *texture_name != 0) {
            handle = retdec_load_act_texture(texture_name);
            field<int32_t>(resource + 0x44) = handle;
            field<int32_t>(layout + 0x134) = handle;
        }
    }
    if (handle == 0)
        return -0x7fffbffb;

    if ((uint32_t)handle < KINOKO_TEXTURE_CAPACITY) {
        texture_width = kinoko_texture_slots[(uint32_t)handle].width;
        texture_height = kinoko_texture_slots[(uint32_t)handle].height;
    }
    if (texture_width == 0)
        texture_width = (uint32_t)field<int32_t>(resource + 72);
    if (texture_height == 0)
        texture_height = (uint32_t)field<int32_t>(resource + 76);
    if (texture_width == 0 || texture_height == 0)
        return -0x7fffbffb;

    source_x = field<float>(resource + 80);
    source_y = field<float>(resource + 84);
    source_width = field<float>(resource + 88);
    source_height = field<float>(resource + 92);
    if (source_width <= 0.0f)
        source_width = (float)field<int32_t>(resource + 72);
    if (source_height <= 0.0f)
        source_height = (float)field<int32_t>(resource + 76);
    if (source_width <= 0.0f || source_height <= 0.0f)
        return -0x7fffbffb;

    sprite = layout + 4;
    field<int32_t>(sprite + 4) = handle;
    field<float>(sprite + 120) = (float)texture_width;
    field<float>(sprite + 124) = (float)texture_height;
    field<float>(sprite + 224) =
        source_width / (float)texture_width;
    field<float>(sprite + 228) =
        source_height / (float)texture_height;
    field<float>(sprite + 28) =
        source_x / (float)texture_width;
    field<float>(sprite + 32) =
        source_y / (float)texture_height;
    field<float>(sprite + 56) =
        (source_x + source_width) / (float)texture_width;
    field<float>(sprite + 60) =
        source_y / (float)texture_height;
    field<float>(sprite + 84) =
        field<float>(sprite + 28);
    field<float>(sprite + 88) =
        (source_y + source_height) / (float)texture_height;
    field<float>(sprite + 112) =
        field<float>(sprite + 56);
    field<float>(sprite + 116) =
        field<float>(sprite + 88);

    /* Keep the source rectangle in the CSpriteEx fields used by 405800's
       original callers, then write the transformed positions in the fields
       consumed by the userpurge draw routine. */
    field<float>(sprite + 128) = 0.0f;
    field<float>(sprite + 132) = 0.0f;
    field<float>(sprite + 136) = 0.0f;
    field<float>(sprite + 140) = source_width;
    field<float>(sprite + 144) = 0.0f;
    field<float>(sprite + 148) = 0.0f;
    field<float>(sprite + 152) = 0.0f;
    field<float>(sprite + 156) = source_height;
    field<float>(sprite + 160) = 0.0f;
    field<float>(sprite + 164) = source_width;
    field<float>(sprite + 168) = source_height;
    field<float>(sprite + 172) = 0.0f;

    retdec_c2dlayout_world_position(layer, &world_x, &world_y, &world_z);
    scale_x = field<float>(layout + 260);
    scale_y = field<float>(layout + 264);
    if (scale_x == 0.0f)
        scale_x = 1.0f;
    if (scale_y == 0.0f)
        scale_y = 1.0f;
    right = world_x + source_width * scale_x;
    bottom = world_y + source_height * scale_y;
    left = world_x;
    top = world_y;

    angle = field<float>(layout + 244);
    if (angle != 0.0f) {
        float pivot_x = world_x +
            field<float>(layout + 248) * scale_x;
        float pivot_y = world_y +
            field<float>(layout + 252) * scale_y;
        float cosine = cosf(angle);
        float sine = sinf(angle);
        float points[4][2] = {
            { left, top }, { right, top },
            { left, bottom }, { right, bottom }
        };
        unsigned int index;
        for (index = 0; index < 4; ++index) {
            float dx = points[index][0] - pivot_x;
            float dy = points[index][1] - pivot_y;
            points[index][0] = pivot_x + dx * cosine - dy * sine;
            points[index][1] = pivot_y + dx * sine + dy * cosine;
        }
        field<float>(layout + 180) = points[0][0];
        field<float>(layout + 184) = points[0][1];
        field<float>(layout + 192) = points[1][0];
        field<float>(layout + 196) = points[1][1];
        field<float>(layout + 204) = points[2][0];
        field<float>(layout + 208) = points[2][1];
        field<float>(layout + 216) = points[3][0];
        field<float>(layout + 220) = points[3][1];
    } else {
        field<float>(layout + 180) = left;
        field<float>(layout + 184) = top;
        field<float>(layout + 192) = right;
        field<float>(layout + 196) = top;
        field<float>(layout + 204) = left;
        field<float>(layout + 208) = bottom;
        field<float>(layout + 216) = right;
        field<float>(layout + 220) = bottom;
    }
    field<float>(layout + 188) = world_z;
    field<float>(layout + 200) = world_z;
    field<float>(layout + 212) = world_z;
    field<float>(layout + 224) = world_z;

    alpha = (unsigned int)(field<float>(layout + 284) * 255.0f);
    if (alpha > 255u)
        alpha = 255u;
    red = field<int32_t>(layout + 292);
    green = field<int32_t>(layout + 296);
    blue = field<int32_t>(layout + 300);
    if (red < 0) red = 0;
    if (green < 0) green = 0;
    if (blue < 0) blue = 0;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    color = (alpha << 24) | ((uint32_t)red << 16) |
            ((uint32_t)green << 8) | (uint32_t)blue;
    vertex = pointer<float>(sprite + 8);
    field<uint32_t>(sprite + 24) = color;
    field<uint32_t>(sprite + 52) = color;
    field<uint32_t>(sprite + 80) = color;
    field<uint32_t>(sprite + 108) = color;
    (void)vertex;
    return 0;
}

float retdec_sprite_scale_about(float value,
                                           float pivot,
                                           float scale)
{
    return (value - pivot) * scale + pivot;
}

void retdec_sprite_scale_faithful(int32_t sprite,
                                          float scale_x,
                                          float pivot_x,
                                          float scale_y,
                                          float pivot_y,
                                          float scale_z,
                                          float pivot_z)
{
    static const uint32_t x_offsets[4] = { 176, 188, 200, 212 };
    static const uint32_t y_offsets[4] = { 180, 192, 204, 216 };
    static const uint32_t z_offsets[4] = { 184, 196, 208, 220 };
    uint32_t index;

    for (index = 0; index < 4; ++index) {
        float *x = pointer<float>(sprite + x_offsets[index]);
        float *y = pointer<float>(sprite + y_offsets[index]);
        float *z = pointer<float>(sprite + z_offsets[index]);
        *x = retdec_sprite_scale_about(*x, pivot_x, scale_x);
        *y = retdec_sprite_scale_about(*y, pivot_y, scale_y);
        *z = retdec_sprite_scale_about(*z, pivot_z, scale_z);
    }
}

void retdec_sprite_rotate_xy(float *x, float *y,
                                    float pivot_x, float pivot_y,
                                    float angle)
{
    float cosine;
    float sine;
    float old_x;
    float old_y;
    float dx;
    float dy;

    if (angle == 0.0f || x == nullptr || y == nullptr)
        return;
    cosine = function_404130((long double)angle);
    sine = function_4040d0((long double)angle);
    old_x = *x;
    old_y = *y;
    dx = old_x - pivot_x;
    dy = old_y - pivot_y;
    *x = dx * cosine + pivot_x - dy * sine;
    *y = dx * sine + pivot_y + dy * cosine;
}

void retdec_sprite_rotate_faithful(int32_t sprite,
                                           float angle_x,
                                           float angle_y,
                                           float angle_z,
                                           float pivot_x,
                                           float pivot_y,
                                           float pivot_z)
{
    static const uint32_t x_offsets[4] = { 176, 188, 200, 212 };
    static const uint32_t y_offsets[4] = { 180, 192, 204, 216 };
    static const uint32_t z_offsets[4] = { 184, 196, 208, 220 };
    float cosine;
    float sine;
    uint32_t index;

    /* 405320 applies Z, then Y, then X rotation around the supplied pivot. */
    if (angle_z != 0.0f) {
        for (index = 0; index < 4; ++index) {
            retdec_sprite_rotate_xy(
                pointer<float>(sprite + x_offsets[index]),
                pointer<float>(sprite + y_offsets[index]),
                pivot_x, pivot_y, angle_z);
        }
    }
    if (angle_y != 0.0f) {
        cosine = function_404130((long double)angle_y);
        sine = function_4040d0((long double)angle_y);
        for (index = 0; index < 4; ++index) {
            float *x = pointer<float>(sprite + x_offsets[index]);
            float *z = pointer<float>(sprite + z_offsets[index]);
            float old_x = *x;
            float old_z = *z;
            float dx = old_x - pivot_x;
            float dz = old_z - pivot_z;
            *x = dx * cosine + pivot_x + dz * sine;
            *z = dz * cosine + pivot_z - dx * sine;
        }
    }
    if (angle_x != 0.0f) {
        cosine = function_404130((long double)angle_x);
        sine = function_4040d0((long double)angle_x);
        for (index = 0; index < 4; ++index) {
            float *y = pointer<float>(sprite + y_offsets[index]);
            float *z = pointer<float>(sprite + z_offsets[index]);
            float old_y = *y;
            float old_z = *z;
            float dy = old_y - pivot_y;
            float dz = old_z - pivot_z;
            *y = dy * cosine + pivot_y + dz * sine;
            *z = dz * cosine + pivot_z - dy * sine;
        }
    }
}

void retdec_sprite_translate_faithful(int32_t sprite,
                                              float x,
                                              float y,
                                              float z)
{
    static const uint32_t x_offsets[4] = { 176, 188, 200, 212 };
    static const uint32_t y_offsets[4] = { 180, 192, 204, 216 };
    static const uint32_t z_offsets[4] = { 184, 196, 208, 220 };
    uint32_t index;

    for (index = 0; index < 4; ++index) {
        field<float>(sprite + x_offsets[index]) += x;
        field<float>(sprite + y_offsets[index]) += y;
        field<float>(sprite + z_offsets[index]) += z;
    }
}

int32_t retdec_c2dlayout_update_faithful_impl(int32_t layout)
{
    int32_t layer;
    int32_t resource;
    int32_t handle;
    uint32_t texture_width = 0;
    uint32_t texture_height = 0;
    float source_x;
    float source_y;
    float source_width;
    float source_height;
    float scale_x;
    float scale_y;
    float scale_z;
    float world_x = 0.0f;
    float world_y = 0.0f;
    float world_z = 0.0f;
    int32_t source_left;
    int32_t source_top;
    int32_t source_width_i;
    int32_t source_height_i;
    int32_t sprite;
    unsigned int alpha;
    int32_t alpha_value;
    int32_t red;
    int32_t green;
    int32_t blue;
    uint32_t color;
    uint32_t index;
    static volatile LONG diagnostic_count;
    LONG diagnostic_index;

    if (layout == 0)
        return -0x7fffbffb;
    diagnostic_index = InterlockedIncrement(&diagnostic_count);
    layer = field<int32_t>(layout + 0x130);
    if (layer == 0)
        return -0x7fffbffb;
    if (field<uint8_t>(layer + 0x8c) == 0)
        return 0;
    resource = field<int32_t>(layer + 0x64);
    if (resource == 0)
        return -0x7fffbffb;

    handle = field<int32_t>(resource + 0x44);
    if (handle == 0) {
        const char *texture_name = retdec_std_string_data(resource + 40);
        if (texture_name != nullptr && *texture_name != 0) {
            handle = retdec_load_act_texture(texture_name);
            field<int32_t>(resource + 0x44) = handle;
        }
    }
    if (handle == 0)
        return -0x7fffbffb;
    field<int32_t>(layout + 0x134) = handle;

    if ((uint32_t)handle < KINOKO_TEXTURE_CAPACITY) {
        texture_width = kinoko_texture_slots[(uint32_t)handle].width;
        texture_height = kinoko_texture_slots[(uint32_t)handle].height;
    }
    if (texture_width == 0)
        texture_width = (uint32_t)field<int32_t>(resource + 72);
    if (texture_height == 0)
        texture_height = (uint32_t)field<int32_t>(resource + 76);
    if (texture_width == 0 || texture_height == 0)
        return -0x7fffbffb;

    source_x = field<float>(resource + 80);
    source_y = field<float>(resource + 84);
    source_width = field<float>(resource + 88);
    source_height = field<float>(resource + 92);
    if (source_width <= 0.0f)
        source_width = (float)field<int32_t>(resource + 72);
    if (source_height <= 0.0f)
        source_height = (float)field<int32_t>(resource + 76);
    if (source_width <= 0.0f || source_height <= 0.0f)
        return -0x7fffbffb;

    if (diagnostic_index <= 8) {
        retdec_trace_i32("c2d:diag-layout", layout);
        retdec_trace_i32("c2d:diag-layer", layer);
        retdec_trace_i32("c2d:diag-dst-x",
                         field<int32_t>(layer + 0x90));
        retdec_trace_i32("c2d:diag-dst-y",
                         field<int32_t>(layer + 0x94));
        retdec_trace_i32("c2d:diag-dst-z",
                         field<int32_t>(layer + 0x98));
        retdec_trace_i32("c2d:diag-scale-x",
                         field<int32_t>(layout + 260));
        retdec_trace_i32("c2d:diag-scale-y",
                         field<int32_t>(layout + 264));
        retdec_trace_i32("c2d:diag-scale-z",
                         field<int32_t>(layout + 268));
        retdec_trace_i32("c2d:diag-pivot-x",
                         field<int32_t>(layout + 272));
        retdec_trace_i32("c2d:diag-pivot-y",
                         field<int32_t>(layout + 276));
        retdec_trace_i32("c2d:diag-pivot-z",
                         field<int32_t>(layout + 280));
        retdec_trace_i32("c2d:diag-angle-x",
                         field<int32_t>(layout + 236));
        retdec_trace_i32("c2d:diag-angle-y",
                         field<int32_t>(layout + 240));
        retdec_trace_i32("c2d:diag-angle-z",
                         field<int32_t>(layout + 244));
    }

    /* 404EE0 receives the ACT rectangle as four truncated integers. */
    source_left = (int32_t)source_x;
    source_top = (int32_t)source_y;
    source_width_i = (int32_t)source_width;
    source_height_i = (int32_t)source_height;
    sprite = layout + 4;
    field<int32_t>(sprite + 4) = handle;
    field<float>(sprite + 120) = (float)texture_width;
    field<float>(sprite + 124) = (float)texture_height;
    field<float>(sprite + 224) =
        (float)source_width_i / (float)texture_width;
    field<float>(sprite + 228) =
        (float)source_height_i / (float)texture_height;
    field<float>(sprite + 28) =
        (float)source_left / (float)texture_width;
    field<float>(sprite + 32) =
        (float)source_top / (float)texture_height;
    field<float>(sprite + 56) =
        (float)(source_left + source_width_i) /
        (float)texture_width;
    field<float>(sprite + 60) =
        (float)source_top / (float)texture_height;
    field<float>(sprite + 84) =
        field<float>(sprite + 28);
    field<float>(sprite + 88) =
        (float)(source_top + source_height_i) /
        (float)texture_height;
    field<float>(sprite + 112) =
        field<float>(sprite + 56);
    field<float>(sprite + 116) =
        field<float>(sprite + 88);

    field<float>(sprite + 128) = 0.0f;
    field<float>(sprite + 132) = 0.0f;
    field<float>(sprite + 136) = 0.0f;
    field<float>(sprite + 140) = (float)source_width_i;
    field<float>(sprite + 144) = 0.0f;
    field<float>(sprite + 148) = 0.0f;
    field<float>(sprite + 152) = 0.0f;
    field<float>(sprite + 156) = (float)source_height_i;
    field<float>(sprite + 160) = 0.0f;
    field<float>(sprite + 164) = (float)source_width_i;
    field<float>(sprite + 168) = (float)source_height_i;
    field<float>(sprite + 172) = 0.0f;

    /* 42C100 copies the untransformed sprite geometry before applying the
       scale, Euler rotations, and layer translation in that order. */
    std::memcpy(pointer<void>(layout + 180),
           pointer<const void>(layout + 132), 12u * sizeof(float));
    scale_x = field<float>(layout + 260);
    scale_y = field<float>(layout + 264);
    scale_z = field<float>(layout + 268);
    if (scale_x == 0.0f) scale_x = 1.0f;
    if (scale_y == 0.0f) scale_y = 1.0f;
    if (scale_z == 0.0f) scale_z = 1.0f;
    retdec_sprite_scale_faithful(
        sprite, scale_x, field<float>(layout + 272),
        scale_y, field<float>(layout + 276),
        scale_z, field<float>(layout + 280));
    retdec_sprite_rotate_faithful(
        sprite,
        field<float>(layout + 236),
        field<float>(layout + 240),
        field<float>(layout + 244),
        field<float>(layout + 248),
        field<float>(layout + 252),
        field<float>(layout + 256));

    /* function_41ef50 is the layer +1C virtual method in the original.  Its
       reconstructed body still loses ECX, so use the same +90/+94/+98 and
       +58 walk explicitly until that method has its own bridge. */
    retdec_c2dlayout_world_position(layer, &world_x, &world_y, &world_z);
    retdec_sprite_translate_faithful(sprite, world_x, world_y, world_z);

    alpha_value = (int32_t)(field<float>(layout + 284) * 255.0f);
    if (alpha_value < 0) alpha_value = 0;
    if (alpha_value > 255) alpha_value = 255;
    alpha = (unsigned int)alpha_value;
    red = field<int32_t>(layout + 292);
    green = field<int32_t>(layout + 296);
    blue = field<int32_t>(layout + 300);
    if (red < 0) red = 0;
    if (green < 0) green = 0;
    if (blue < 0) blue = 0;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    color = (alpha << 24) | ((uint32_t)red << 16) |
            ((uint32_t)green << 8) | (uint32_t)blue;
    for (index = 0; index < 4; ++index)
        field<uint32_t>(sprite + 24u + index * 28u) = color;
    if (diagnostic_index <= 8) {
        retdec_trace_i32("c2d:diag-v0-x",
                         field<int32_t>(layout + 180));
        retdec_trace_i32("c2d:diag-v0-y",
                         field<int32_t>(layout + 184));
        retdec_trace_i32("c2d:diag-v1-x",
                         field<int32_t>(layout + 192));
        retdec_trace_i32("c2d:diag-v1-y",
                         field<int32_t>(layout + 196));
        retdec_trace_i32("c2d:diag-v2-x",
                         field<int32_t>(layout + 204));
        retdec_trace_i32("c2d:diag-v2-y",
                         field<int32_t>(layout + 208));
        retdec_trace_i32("c2d:diag-v3-x",
                         field<int32_t>(layout + 216));
        retdec_trace_i32("c2d:diag-v3-y",
                         field<int32_t>(layout + 220));
    }
    return 0;
}

int32_t retdec_c2dlayout_draw_impl(int32_t layout,
                                           float x, float y)
{
    int32_t result;
    int32_t layer;
    int32_t visibility_layer;
    uint32_t guard = 0;
    int32_t *vtable;
    IDirect3DDevice9 *device;
    DWORD old_src_blend = 0;
    DWORD old_dest_blend = 0;
    DWORD old_blend_op = 0;
    DWORD old_alpha_blend = 0;
    int states_saved = 0;
    static volatile LONG trace_count;
    LONG trace_index;

    if (layout == 0)
        return -0x7fffbffb;
    layer = field<int32_t>(layout + 0x130);
    if (layer == 0)
        return -0x7fffbffb;
    trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 16) {
        retdec_trace_i32("c2d:layout", layout);
        retdec_trace_i32("c2d:layer", layer);
        retdec_trace_i32("c2d:layer-id",
                         field<int32_t>(layer + 0x68));
        retdec_trace_i32("c2d:parent-id",
                         field<int32_t>(layer + 0x6c));
        retdec_trace_i32("c2d:visible",
                         field<int32_t>(layer + 0x8c));
        retdec_trace_i32("c2d:resource",
                         field<int32_t>(layer + 0x64));
        retdec_trace_i32("c2d:handle",
                         field<int32_t>(layout + 0x134));
        retdec_trace_i32("c2d:blend",
                         field<int32_t>(layout + 0x120));
        retdec_trace_squirrel_name(
            "c2d:layer-name",
            address(retdec_std_string_data(layer + 0x70)));
        if (field<int32_t>(layer + 0x64) != 0) {
            int32_t bound_resource =
                field<int32_t>(layer + 0x64);
            retdec_trace_squirrel_name(
                "c2d:texture-name",
                address(retdec_std_string_data(
                    bound_resource + 40)));
        }
    }
    visibility_layer = layer;
    while (visibility_layer != 0 && guard++ < 64u) {
        if (field<uint8_t>(visibility_layer + 0x8c) == 0) {
            if (trace_index <= 16)
                retdec_trace("c2d:skip-invisible");
            return 0;
        }
        visibility_layer =
            field<int32_t>(visibility_layer + 0x58);
    }

    /* 42C300 surrounds every sprite with the renderer's alpha-blend state.
       Without this, the A8R8G8B8 ACT textures are submitted but their
       transparent pixels become opaque black/white rectangles. */
    device = pointer<IDirect3DDevice9>(g678);
    if (device != nullptr &&
        SUCCEEDED(device->GetRenderState(D3DRS_SRCBLEND, &old_src_blend)) &&
        SUCCEEDED(device->GetRenderState(D3DRS_DESTBLEND, &old_dest_blend)) &&
        SUCCEEDED(device->GetRenderState(D3DRS_BLENDOP, &old_blend_op)) &&
        SUCCEEDED(device->GetRenderState(D3DRS_ALPHABLENDENABLE, &old_alpha_blend))) {
        states_saved = 1;
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        /* ACT layout blend=1 is the normal source-alpha composition mode. */
        if (field<int32_t>(layout + 0x120) == 1) {
            device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }
    }
    result = retdec_c2dlayout_update_faithful_impl(layout);
    if (trace_index <= 16)
        retdec_trace_i32("c2d:update-result", result);
    if (result < 0)
        goto restore_render_state;
    vtable = field<int32_t *>(layout + 0);
    if (vtable == nullptr) {
        result = -0x7fffbffb;
        goto restore_render_state;
    }
    if (trace_index <= 16)
        retdec_trace("c2d:submit");
    result = retdec_layout_submit_impl(layout + 4, x, y);

restore_render_state:
    retdec_set_texture_stage(0, 0);
    if (states_saved) {
        device->SetRenderState(D3DRS_SRCBLEND, old_src_blend);
        device->SetRenderState(D3DRS_DESTBLEND, old_dest_blend);
        device->SetRenderState(D3DRS_BLENDOP, old_blend_op);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, old_alpha_blend);
    }
    return result;
}
