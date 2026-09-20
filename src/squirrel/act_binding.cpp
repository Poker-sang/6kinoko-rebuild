#include "kinoko/squirrel_api_types.h"
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
#include "kinoko/legacy_abi.h"
#include "kinoko/upstream_bindings.hpp"
#include <windows.h>
#include <d3d9.h>
#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::field;

namespace {
// Typed wrapper around the existing source-backed Sqrat C boundary.
int32_t get_pair(int32_t object, const char* name, int32_t* output) {
    return retdec_sqrat_get(object, name, address(output));
}
}

int32_t retdec_publish_cact_layer_property(
    int32_t vm, const char *name, int32_t offset,
    int32_t getter, int32_t setter)
{
    if (g1151 != 0x0A000020 || g1152 == 0 ||
        g1153 != 0x0A000020 || g1154 == 0)
        return 0;
    if (!retdec_sqrat_set_offset_closure(
            vm, (const int32_t *)&g1153, name, offset, getter) ||
        !retdec_sqrat_set_offset_closure(
            vm, (const int32_t *)&g1151, name, offset, setter))
        return 0;
    return 1;
}

int32_t retdec_publish_cact_layer_members(
    int32_t vm, const int32_t *class_pair)
{
    static const char *const direct_int_names[] = {
        "resourceID", "layerID", "parentID"
    };
    static const int32_t direct_int_offsets[] = { 0x60, 0x68, 0x6c };
    static const char *const direct_float_names[] = {
        "dst_x", "dst_y", "dst_z", "x", "y", "z",
        "prev_x", "prev_y", "prev_z", "xPrev", "yPrev", "zPrev",
        "ox", "oy", "oz"
    };
    static const int32_t direct_float_offsets[] = {
        0x90, 0x94, 0x98, 0x90, 0x94, 0x98,
        0xa8, 0xac, 0xb0, 0xa8, 0xac, 0xb0,
        0x9c, 0xa0, 0xa4
    };
    static const char *const pointer_float_names[] = {
        "roll_x", "roll_y", "roll_z", "cor_x", "cor_y", "cor_z",
        "scale_x", "scale_y", "scale_z", "cos_x", "cos_y", "cos_z",
        "alpha"
    };
    static const int32_t pointer_float_offsets[] = {
        4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52
    };
    static const char *const pointer_int_names[] = {
        "blend", "colorR", "colorG", "colorB"
    };
    static const int32_t pointer_int_offsets[] = { 56, 60, 64, 68 };
    static const char *const object_names[] = {
        "script", "layout", "resource"
    };
    int32_t empty_pair[2] = { g483, g484 };
    int32_t table_pair[2] = { g483, g484 };
    int32_t table_pair2[2] = { g483, g484 };
    int32_t null_pair[2] = { 0x01000001, 0 };
    size_t index;

    if (vm == 0 || class_pair == nullptr ||
        class_pair[0] != 0x08004000 || class_pair[1] == 0)
        return 0;

    if (g1151 != 0x0A000020 || g1152 == 0 ||
        g1153 != 0x0A000020 || g1154 == 0) {
        if (!retdec_sqrat_new_table(vm, table_pair) ||
            !retdec_sqrat_new_table(vm, table_pair2)) {
            retdec_sqrat_release_pair(vm, table_pair);
            retdec_sqrat_release_pair(vm, table_pair2);
            return 0;
        }
        retdec_sqrat_assign_pair(vm, &g1151, table_pair);
        retdec_sqrat_assign_pair(vm, &g1153, table_pair2);
        retdec_sqrat_release_pair(vm, table_pair);
        retdec_sqrat_release_pair(vm, table_pair2);
    }

    if (!retdec_sqrat_initialize_class(vm, class_pair,
            (const int32_t *)&g1151, (const int32_t *)&g1153,
            address(retdec_sqrat_no_constructor), address(function_41e2c0),
            address(function_41e260), address(function_431650)))
        goto failed;

    if (!retdec_publish_cact_layer_property(
            vm, "stName", 0x70,
            address(retdec_cact_layer_get_string),
            address(retdec_cact_layer_set_string)))
        goto failed;
    for (index = 0; index < sizeof(direct_int_names) /
                         sizeof(direct_int_names[0]); ++index) {
        if (!retdec_publish_cact_layer_property(
                vm, direct_int_names[index], direct_int_offsets[index],
                address(retdec_cact_layer_get_int),
                address(retdec_cact_layer_set_int)))
            goto failed;
    }
    if (!retdec_publish_cact_layer_property(
            vm, "visible", 0x8c,
            address(retdec_cact_layer_get_bool),
            address(retdec_cact_layer_set_bool)) ||
        !retdec_publish_cact_layer_property(
            vm, "debugOnly", 0x8d,
            address(retdec_cact_layer_get_bool),
            address(retdec_cact_layer_set_bool)))
        goto failed;
    for (index = 0; index < sizeof(direct_float_names) /
                         sizeof(direct_float_names[0]); ++index) {
        if (!retdec_publish_cact_layer_property(
                vm, direct_float_names[index], direct_float_offsets[index],
                address(retdec_cact_layer_get_float),
                address(retdec_cact_layer_set_float)))
            goto failed;
    }
    for (index = 0; index < sizeof(pointer_float_names) /
                         sizeof(pointer_float_names[0]); ++index) {
        if (!retdec_publish_cact_layer_property(
                vm, pointer_float_names[index], pointer_float_offsets[index],
                address(retdec_cact_layer_get_pointer_float),
                address(retdec_cact_layer_set_pointer_float)))
            goto failed;
    }
    for (index = 0; index < sizeof(pointer_int_names) /
                         sizeof(pointer_int_names[0]); ++index) {
        if (!retdec_publish_cact_layer_property(
                vm, pointer_int_names[index], pointer_int_offsets[index],
                address(retdec_cact_layer_get_pointer_int),
                address(retdec_cact_layer_set_pointer_int)))
            goto failed;
    }

    /* These are the three SQObject members installed by the original
       TypePropertyClass<string/object> helpers.  Their values are filled on
       each instance with the same raw/newslot distinction as the ACT path. */
    for (index = 0; index < sizeof(object_names) / sizeof(object_names[0]);
         ++index) {
        if (!retdec_sqrat_set_pair(vm, class_pair, object_names[index],
                                   null_pair))
            goto failed;
    }
    return 1;

failed:
    retdec_sqrat_assign_pair(vm, &g1151, empty_pair);
    retdec_sqrat_assign_pair(vm, &g1153, empty_pair);
    return 0;
}

int32_t retdec_publish_c2dlayout_properties(
    int32_t vm, const int32_t class_pair[2])
{
    static const char *const float_names[] = {
        "roll_x", "roll_y", "roll_z", "cor_x", "cor_y", "cor_z",
        "scale_x", "scale_y", "scale_z", "cos_x", "cos_y", "coS_z",
        "alpha"
    };
    static const int32_t float_offsets[] = {
        236, 240, 244, 248, 252, 256, 260, 264, 268, 272, 276, 280, 284
    };
    static const char *const color_names[] = { "colorR", "colorG", "colorB" };
    static const int32_t color_offsets[] = { 292, 296, 300 };
    int32_t empty_pair[2] = { g483, g484 };
    int32_t table_pair[2] = { g483, g484 };
    int32_t table_pair2[2] = { g483, g484 };
    size_t index;

    if (vm == 0 || class_pair == nullptr ||
        class_pair[0] != 0x08004000 || class_pair[1] == 0)
        return 0;

    if (g1141 != 0x0A000020 || g1142 == 0 ||
        g1143 != 0x0A000020 || g1144 == 0) {
        if (!retdec_sqrat_new_table(vm, table_pair) ||
            !retdec_sqrat_new_table(vm, table_pair2)) {
            retdec_sqrat_release_pair(vm, table_pair);
            retdec_sqrat_release_pair(vm, table_pair2);
            return 0;
        }
        retdec_sqrat_assign_pair(vm, &g1141, table_pair);
        retdec_sqrat_assign_pair(vm, &g1143, table_pair2);
        retdec_sqrat_release_pair(vm, table_pair);
        retdec_sqrat_release_pair(vm, table_pair2);
    }

    if (!retdec_sqrat_initialize_class(vm, class_pair,
            (const int32_t *)&g1141, (const int32_t *)&g1143,
            address(retdec_sqrat_no_constructor), address(function_41e2c0),
            address(function_41e260), address(function_431650)))
        goto failed;

    for (index = 0; index < sizeof(float_names) / sizeof(float_names[0]);
         ++index) {
        if (!retdec_sqrat_set_offset_closure(
                vm, (const int32_t *)&g1143, float_names[index],
                float_offsets[index],
                address(retdec_c2dlayout_get_float)) ||
            !retdec_sqrat_set_offset_closure(
                vm, (const int32_t *)&g1141, float_names[index],
                float_offsets[index],
                address(retdec_c2dlayout_set_float)))
            goto failed;
    }
    if (!retdec_sqrat_set_offset_closure(
            vm, (const int32_t *)&g1143, "blend", 288,
            address(retdec_c2dlayout_get_int)) ||
        !retdec_sqrat_set_offset_closure(
            vm, (const int32_t *)&g1141, "blend", 288,
            address(retdec_c2dlayout_set_int)))
        goto failed;
    for (index = 0; index < sizeof(color_names) / sizeof(color_names[0]);
         ++index) {
        if (!retdec_sqrat_set_offset_closure(
                vm, (const int32_t *)&g1143, color_names[index],
                color_offsets[index],
                address(retdec_c2dlayout_get_int)) ||
            !retdec_sqrat_set_offset_closure(
                vm, (const int32_t *)&g1141, color_names[index],
                color_offsets[index],
                address(retdec_c2dlayout_set_color)))
            goto failed;
    }
    return 1;

failed:
    retdec_sqrat_assign_pair(vm, &g1141, empty_pair);
    retdec_sqrat_assign_pair(vm, &g1143, empty_pair);
    return 0;
}

int32_t retdec_publish_c2dlayout_class(int32_t vm, int32_t root_object)
{
    int32_t existing[2] = { g483, g484 };
    int32_t class_pair[2] = { g483, g484 };
    int32_t empty_pair[2] = { g483, g484 };
    int32_t existing_result;
    int32_t base;

    if (vm == 0 || root_object == 0)
        return 0;
    existing_result = get_pair(root_object, "C2DLayout", existing);
    if (existing_result && existing[0] == 0x08004000 && existing[1] != 0) {
        retdec_sqrat_assign_pair(vm, &g1145, existing);
        class_pair[0] = existing[0];
        class_pair[1] = existing[1];
        retdec_sqrat_release_pair(vm, existing);
        if (g1141 == 0x0A000020 && g1142 != 0 &&
            g1143 == 0x0A000020 && g1144 != 0)
            return 1;
        return retdec_publish_c2dlayout_properties(vm, class_pair);
    }
    if (existing_result)
        retdec_sqrat_release_pair(vm, existing);

    base = sq_gettop(kinoko_vm(vm));
    if (!retdec_sqrat_new_class(vm, class_pair) || sq_gettop(kinoko_vm(vm)) <= base) {
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (class_pair[0] != 0x08004000 || class_pair[1] == 0) {
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }

    retdec_sqrat_assign_pair(vm, &g1145, class_pair);
    if (!retdec_publish_c2dlayout_properties(vm, class_pair))
        goto publish_failed;
    if (!retdec_sqrat_set_pair(
            vm, pointer<const int32_t>(root_object + 8),
            "C2DLayout", class_pair)) {
        retdec_sqrat_assign_pair(vm, &g1145, empty_pair);
        goto publish_failed;
    }
    retdec_trace_i32("act:c2dlayout-class", class_pair[1]);
    retdec_sqrat_release_pair(vm, class_pair);
    retdec_sqrat_trim_stack(vm, base);
    return 1;

publish_failed:
    retdec_sqrat_release_pair(vm, class_pair);
    retdec_sqrat_trim_stack(vm, base);
    return 0;
}

// Original 42B6D0 is a cdecl VM-only registration entry. The root-table
// registry used by the active binding owns the same class/property handles;
// do not create another decompiled map of Sqrat objects for this old caller.
extern "C" int32_t function_42b6d0(int32_t vm) {
    if (!vm) return static_cast<int32_t>(E_INVALIDARG);
    int32_t root[5] = {};
    if (!retdec_sqrat_root_construct(address(root), vm)) return static_cast<int32_t>(E_FAIL);
    const auto ok = retdec_publish_c2dlayout_class(vm, address(root));
    retdec_sqrat_object_release(address(root));
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

int32_t retdec_cact_associate_resource(int32_t vm)
{
    int32_t layer = 0, resource = 0;
    int32_t result = (int32_t)E_FAIL;
    if (sq_getinstanceup(kinoko_vm(vm), 1, (SQUserPointer*)(&layer), kinoko_pointer(0)) >= 0 && layer != 0 &&
        sq_getinstanceup(kinoko_vm(vm), 2, (SQUserPointer*)(&resource), kinoko_pointer(0)) >= 0 && resource != 0) {
        /* 424210 -> 4252E0 -> 41EF20: change only the resource and its ID. */
        field<int32_t>(layer + 100) = resource;
        field<int32_t>(layer + 96) = field<int32_t>(resource + 4);
        retdec_trace_squirrel_name("act:associate-layer",
            address(retdec_std_string_data(layer + 112)));
        retdec_trace_squirrel_name("act:associate-resource",
            address(retdec_std_string_data(resource + 8)));
        result = 0;
    }
    sq_pushinteger(kinoko_vm(vm), result);
    return 1;
}

int32_t retdec_resource_load_texture(int32_t vm) {
    int32_t resource = 0;
    const SQChar *prefix = nullptr;
    if (SQ_FAILED(sq_getinstanceup(kinoko_vm(vm), 1, reinterpret_cast<SQUserPointer *>(&resource), nullptr)) ||
        !resource || sq_gettop(kinoko_vm(vm)) < 2) return 0;
    if (sq_gettype(kinoko_vm(vm), 2) != OT_NULL &&
        SQ_FAILED(sq_getstring(kinoko_vm(vm), 2, &prefix))) return 0;
    // 44FD20 forwards through virtual slot +40, retaining derived behavior.
    const int32_t result = retdec_call_thiscall1_result(pointer<void>(resource),
        field<void *>(field<int32_t>(resource) + 40), address(prefix));
    sq_pushbool(kinoko_vm(vm), (result & 0xff) != 0);
    return 1;
}

int32_t retdec_publish_texture_resource_class(int32_t vm, int32_t root,
    const char *name, int32_t out[2]) {
    static const retdec_native_view_property properties[] = {
        {"resourceID", 4, 0}, {"stName", 8, 4},
        {"image_width", 72, 0}, {"image_height", 76, 0},
        {"src_x", 80, 1}, {"src_y", 84, 1},
        {"src_width", 88, 1}, {"src_height", 92, 1}
    };
    if (get_pair(root, name, out) && out[0] == 0x08004000) return 1;
    retdec_sqrat_release_pair(vm, out);
    return retdec_publish_map_view_class(vm, root, name, properties,
        sizeof(properties) / sizeof(properties[0]), 0, out) &&
        retdec_sqrat_set_native_closure(vm, out, "LoadTexture",
            address(retdec_resource_load_texture), nullptr, 0);
}

int32_t retdec_publish_cact_resource2d_class(int32_t vm, int32_t root) {
    if (!vm || !root) return 0;
    int32_t klass[2] = { g483, g484 };
    const auto ok = retdec_publish_texture_resource_class(vm, root, "CActResource2D", klass);
    if (ok) {
        retdec_sqrat_assign_pair(vm, &g1079, klass);
        g1037 = 1;
    }
    retdec_sqrat_release_pair(vm, klass);
    return ok;
}

extern "C" int32_t function_446520(int32_t vm) {
    if (!vm) return static_cast<int32_t>(E_INVALIDARG);
    int32_t root[5] = {};
    if (!retdec_sqrat_root_construct(address(root), vm)) return static_cast<int32_t>(E_FAIL);
    const auto ok = retdec_publish_cact_resource2d_class(vm, address(root));
    retdec_sqrat_object_release(address(root));
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

extern "C" int32_t function_4495a0(int32_t vm) {
    if (!vm) return static_cast<int32_t>(E_INVALIDARG);
    int32_t root[5] = {}, klass[2] = { g483, g484 };
    if (!retdec_sqrat_root_construct(address(root), vm)) return static_cast<int32_t>(E_FAIL);
    const auto ok = retdec_publish_texture_resource_class(vm, address(root), "CActRenderTarget", klass);
    retdec_sqrat_release_pair(vm, klass);
    retdec_sqrat_object_release(address(root));
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

extern "C" int32_t __fastcall kinoko_method_register_texture_resource(int32_t, void *, int32_t vm) {
    return function_446520(vm);
}
extern "C" int32_t __fastcall kinoko_method_register_render_target(int32_t, void *, int32_t vm) {
    return function_4495a0(vm);
}

int32_t retdec_publish_cact_layer_class(int32_t vm, int32_t root_object)
{
    int32_t existing[2] = { g483, g484 };
    int32_t class_pair[2] = { g483, g484 };
    int32_t class_wrapper[5] = { address(kinoko_act_host_symbols()->sq_object_vtable), 0,
                                 g483, g484, 1 };
    int32_t method_source[2] = {
        address(function_4252e0), 0
    };
    int32_t existing_result;
    int32_t base;

    if (vm == 0 || root_object == 0)
        return 0;
    existing_result = get_pair(root_object, "CActLayer", existing);
    if (existing_result) {
        int32_t already_published =
            existing[0] == 0x08004000 && existing[1] != 0;
        retdec_sqrat_release_pair(vm, existing);
        if (already_published)
            return 1;
    }

    base = sq_gettop(kinoko_vm(vm));
    if (!retdec_sqrat_new_class(vm, class_pair) || sq_gettop(kinoko_vm(vm)) <= base) {
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (class_pair[0] != 0x08004000 || class_pair[1] == 0) {
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }

    class_wrapper[1] = vm;
    class_wrapper[2] = class_pair[0];
    class_wrapper[3] = class_pair[1];
    if (function_415550_this(
            address(class_wrapper),
            (int32_t)(intptr_t)"AssociateResource",
            address(method_source), 8,
            address(retdec_cact_associate_resource), 0) < 0) {
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (!retdec_publish_cact_layer_members(vm, class_pair)) {
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (!retdec_sqrat_set_pair(
            vm, pointer<const int32_t>(root_object + 8),
            "CActLayer", class_pair)) {
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }
    retdec_trace_i32("act:cact-layer-class", class_pair[1]);
    retdec_sqrat_release_pair(vm, class_pair);
    retdec_sqrat_trim_stack(vm, base);
    return 1;
}

// Original VM-only CActLayer class registration (41EFF0). Game instance
// publication still lives in its register method; only the class setup is shared.
extern "C" int32_t function_41eff0(int32_t vm) {
    if (!vm) return static_cast<int32_t>(E_INVALIDARG);
    int32_t root[5] = {};
    if (!retdec_sqrat_root_construct(address(root), vm)) return static_cast<int32_t>(E_FAIL);
    const auto ok = retdec_publish_cact_layer_class(vm, address(root));
    retdec_sqrat_object_release(address(root));
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

int32_t retdec_publish_acting_player_properties(int32_t vm,
                                                        const int32_t class_pair[2]) {
    static const char *names[] = { "staging", "marginLeft", "marginRight",
        "marginTop", "marginBottom", "offsetX", "offsetY", "visible",
        "resolutionMs", "screenWidth", "screenHeight", "stName" };
    static const int32_t offsets[] = { 8, 108, 112, 116, 120, 124, 128,
        132, 136, 140, 144, 148 };
    int32_t get_table[2] = { g483, g484 };
    int32_t set_table[2] = { g483, g484 };
    int32_t ok = 0;
    if (!retdec_sqrat_new_table(vm, get_table) ||
        !retdec_sqrat_new_table(vm, set_table) ||
        !retdec_sqrat_initialize_class(vm, class_pair, set_table, get_table,
            0, address(function_41e2c0), address(function_41e260), 0))
        goto cleanup;
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        if (!retdec_sqrat_set_offset_closure(vm, get_table, names[i], offsets[i],
                address(retdec_acting_player_get_property)) ||
            !retdec_sqrat_set_offset_closure(vm, set_table, names[i], offsets[i],
                address(retdec_acting_player_set_property)))
            goto cleanup;
    }
    ok = 1;
cleanup:
    retdec_sqrat_release_pair(vm, get_table);
    retdec_sqrat_release_pair(vm, set_table);
    return ok;
}

int32_t retdec_publish_acting_player_class(int32_t vm,
                                                   int32_t root_object)
{
    int32_t existing[2] = { g483, g484 };
    int32_t class_pair[2] = { g483, g484 };
    int32_t class_object[4] = { 0, vm, g483, g484 };
    int32_t existing_result;
    int32_t base;

    if (vm == 0 || root_object == 0)
        return 0;
    existing_result = get_pair(root_object, "ActingPlayer", existing);
    if (existing_result) {
        int32_t already_published =
            existing[0] == 0x08004000 && existing[1] != 0;
        if (already_published) {
            g1049 = existing[0];
            g1050 = existing[1];
        }
        retdec_sqrat_release_pair(vm, existing);
        if (already_published)
            return 1;
    }

    base = sq_gettop(kinoko_vm(vm));
    if (!retdec_sqrat_new_class(vm, class_pair) || sq_gettop(kinoko_vm(vm)) <= base)
        return 0;
    if (class_pair[0] != 0x08004000 || class_pair[1] == 0) {
        retdec_sqrat_release_pair(vm, class_pair);
        return 0;
    }

    class_object[1] = vm;
    class_object[2] = class_pair[0];
    class_object[3] = class_pair[1];
    function_460e00_register_actor_method(vm, class_object + 1, "SetCurrentTime",
                                          address(kinoko_act_set_current_time),
                                          address(function_455330), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "IncrementFrame",
                                          address(kinoko_act_increment_frame),
                                          address(function_445530), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "GetCurrentTime",
                                          address(kinoko_act_get_current_time),
                                          address(function_445530), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "GetCurrentFrame",
                                          address(kinoko_act_get_current_frame),
                                          address(function_445530), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "BeginStage",
                                          address(kinoko_method_begin_stage),
                                          address(function_455330), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "EndStage",
                                          address(kinoko_act_end_stage),
                                          address(function_445530), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "CreateLayer2D",
                                          address(function_4517c0),
                                          address(function_455390), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "CreateLayerString",
                                          address(function_451b70),
                                          address(function_455390), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "GetLayerOrder",
                                          address(function_451f30),
                                          address(function_4554b0), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "SwapLayer",
                                          address(function_451f80),
                                          address(function_455520), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "BitBlt",
                                          address(kinoko_method_act_bitblt),
                                          address(function_4555a0), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "SetRenderTarget",
                                          address(function_452010),
                                          address(function_4556c0), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "FindFirstFile",
                                          address(function_452150),
                                          address(function_445650), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "FindNextFile",
                                          address(function_452220),
                                          address(function_4455e0), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "FindClose",
                                          address(function_452270),
                                          address(function_4455e0), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "GetFindFileName",
                                          address(function_4522c0),
                                          address(function_455730), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "timeGetTime",
                                          address(timeGetTime),
                                          address(function_445530), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "Sleep",
                                          address(kinoko_act_sleep),
                                          address(function_445730), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "SleepTo",
                                          address(kinoko_act_sleep_to),
                                          address(function_445730), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "Suspend",
                                          address(kinoko_act_suspend),
                                          address(function_4552e0), 0);
    function_460e00_register_actor_method(vm, class_object + 1, "Resume",
                                          address(kinoko_act_resume),
                                          address(function_4552e0), 0);

    if (!retdec_publish_acting_player_properties(vm, class_pair)) {
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }

    g1049 = class_pair[0];
    g1050 = class_pair[1];
    retdec_sqrat_set_pair(vm,
                          pointer<const int32_t>(root_object + 8),
                          "ActingPlayer", class_pair);
    retdec_sqrat_release_pair(vm, class_pair);
    retdec_sqrat_trim_stack(vm, base);
    return 1;
}

int32_t retdec_publish_acting_player(int32_t vm,
                                             const int32_t *act_pair,
                                             const char *name,
                                             int32_t player_ptr,
                                             int32_t out_pair[2])
{
    int32_t base;
    int32_t result;
    int32_t instance_slot;
    int32_t actual[2] = { g483, g484 };
    int32_t object_wrapper[5] = { address(kinoko_act_host_symbols()->sq_root_vtable), 0,
                                  g483, g484, 1 };

    if (vm == 0 || act_pair == nullptr || name == nullptr || out_pair == nullptr ||
        g1049 != 0x08004000 || g1050 == 0)
        return 0;
    retdec_trace_squirrel_name("act:pair-name", address(name));
    retdec_trace_i32("act:pair-act", act_pair[1]);
    retdec_trace_squirrel_name("act:acting-name", address(name));
    retdec_trace_i32("act:acting-resource", player_ptr);
    retdec_trace_i32("act:acting-class-type", g1049);
    retdec_trace_i32("act:acting-class-data", g1050);
    out_pair[0] = g483;
    out_pair[1] = g484;
    base = sq_gettop(kinoko_vm(vm));
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(act_pair[0], act_pair[1]));
    sq_pushstring(kinoko_vm(vm), (const SQChar*)kinoko_pointer(address(name)), -1);
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(g1049, g1050));
    if (sq_createinstance(kinoko_vm(vm), -1) < 0) {
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }
    sq_remove(kinoko_vm(vm), -2);
    instance_slot = kinoko_sq_get_up(vm, -1);
    retdec_trace_i32("act:acting-instance-slot", instance_slot);
    retdec_trace_i32("act:acting-instance-type",
                     instance_slot != 0 ? field<int32_t>(instance_slot) : 0);
    retdec_trace_i32("act:acting-instance-data",
                     instance_slot != 0 ? field<int32_t>(instance_slot + 4) : 0);
    if (instance_slot != 0 &&
        field<int32_t>(instance_slot) == 0x0A008000 &&
        field<int32_t>(instance_slot + 4) != 0) {
        int32_t instance = field<int32_t>(instance_slot + 4);
        retdec_trace_i32("act:acting-instance-class",
                         field<int32_t>(instance + 28));
        retdec_trace_i32("act:acting-instance-user-before",
                         field<int32_t>(instance + 32));
    }
    if (sq_setinstanceup(kinoko_vm(vm), -1, kinoko_pointer(player_ptr)) < 0) {
        retdec_sqrat_trim_stack(vm, base);
        return 0;
    }
    if (instance_slot != 0 &&
        field<int32_t>(instance_slot) == 0x0A008000 &&
        field<int32_t>(instance_slot + 4) != 0) {
        int32_t instance = field<int32_t>(instance_slot + 4);
        retdec_trace_i32("act:acting-instance-class-after",
                         field<int32_t>(instance + 28));
        retdec_trace_i32("act:acting-instance-user-after",
                         field<int32_t>(instance + 32));
    }
    sq_getstackobj(kinoko_vm(vm), -1, (HSQOBJECT*)(out_pair));
    function_48a400(vm, address(out_pair));
    result = sq_newslot(kinoko_vm(vm), -3, ((0) != 0));
    if (std::strcmp(name, "pl") == 0 || std::strcmp(name, "player") == 0) {
        retdec_trace_squirrel_name("act:acting-read-name",
                                   address(name));
        object_wrapper[1] = vm;
        object_wrapper[2] = act_pair[0];
        object_wrapper[3] = act_pair[1];
        if (get_pair(address(object_wrapper),
                             name, actual)) {
            retdec_trace_i32("act:acting-read-type", actual[0]);
            retdec_trace_i32("act:acting-read-data", actual[1]);
            if (actual[0] == 0x0A008000 && actual[1] != 0) {
                retdec_trace_i32("act:acting-read-class",
                                 field<int32_t>(actual[1] + 28));
                retdec_trace_i32("act:acting-read-user",
                                 field<int32_t>(actual[1] + 32));
            }
            retdec_sqrat_release_pair(vm, actual);
        } else {
            retdec_trace("act:acting-read-failed");
        }
    }
    retdec_trace_i32("act:pair-act-after", act_pair[1]);
    retdec_sqrat_trim_stack(vm, base);
    return result >= 0 && out_pair[0] == 0x0A008000;
}

int32_t retdec_execute_act_source_script(
    int32_t vm, int32_t script_ptr, const int32_t *environment_pair)
{
    unsigned char *bytecode = nullptr;
    int32_t bytecode_size = 0;
    int32_t compiled_script[26] = { 0 };
    const char *source;
    const char *name;
    int32_t size;
    int32_t result;

    if (vm == 0 || script_ptr == 0 || environment_pair == nullptr)
        return 0;
    source = field<const char *>(script_ptr + 92);
    size = field<int32_t>(script_ptr + 96);
    name = retdec_std_string_data(script_ptr + 64);
    if (source == nullptr || size <= 0 || size > 0x1000000)
        return 0;
    if (!retdec_squirrel_compile_source(source, size,
            name != nullptr && *name != 0 ? name : "ACT inline",
            &bytecode, &bytecode_size))
        return 0;
    compiled_script[23] = address(bytecode);
    compiled_script[24] = bytecode_size;
    result = retdec_execute_embedded_act_script(
        vm, address(compiled_script), environment_pair);
    std::free(bytecode);
    return result;
}

int32_t retdec_execute_act_callback(int32_t script_ptr,
                                            int32_t offset,
                                            const char *trace_label)
{
    int32_t slot;
    int32_t vm;
    int32_t closure_type;
    int32_t closure_data;
    int32_t result;

    if (script_ptr == 0)
        return 0;
    slot = script_ptr + offset;
    vm = field<int32_t>(slot);
    closure_type = field<int32_t>(slot + 12);
    closure_data = field<int32_t>(slot + 16);
    retdec_trace_i32("act:callback-offset", offset);
    retdec_trace_i32("act:callback-vm", vm);
    retdec_trace_i32("act:callback-type", closure_type);
    retdec_trace_i32("act:callback-data", closure_data);
    if (vm == 0 || closure_type == g483 || closure_data == 0)
        return 0;
    if (trace_label != nullptr)
        retdec_trace(trace_label);
    result = function_415810_this(slot);
    retdec_trace_i32("act:callback-result", result);
    return result;
}

int32_t retdec_publish_act_script_constants(int32_t vm, const int32_t *environment)
{
    static const char *const names[] = {
        "BLEND_NORMAL", "BLEND_ALPHA", "BLEND_ADD", "BLEND_SUB",
        "BLEND_MULTI", "BLEND_INVERT"
    };
    int32_t object[5] = {address(kinoko_act_host_symbols()->sq_object_vtable), vm, environment[0], environment[1], 0};
    int32_t user[2] = {g483, g484};
    int32_t have_user = get_pair(address(object), "u", user);
    int32_t needs_user = !have_user || user[0] == g483;
    retdec_sqrat_release_pair(vm, user);
    /* 416056..41605F passes the same Sqrat object as source and destination. */
    if (needs_user && !retdec_sqrat_set_pair(vm, environment, "u", environment))
        return 0;
    /* 415FD0:4160DC installs these in the script environment before loading. */
    for (int32_t value = 0; value < 6; ++value) {
        if (!retdec_sqrat_set_int(vm, environment, names[value], value))
            return 0;
    }
    return 1;
}

int32_t retdec_prepare_cact_layer_objects(int32_t vm, int32_t layer,
                                                 int32_t script_pair[2])
{
    int32_t table_pair[2] = { g483, g484 };
    int32_t old_vm;

    if (vm == 0 || layer == 0 || script_pair == nullptr)
        return 0;

    /* CActLayer::CActLayer initializes a Sqrat::Table at +0x134 and a
       Sqrat::Instance at +0x148.  The parser allocates the native layer
       directly, so establish those two wrappers before publishing it. */
    old_vm = field<int32_t>(layer + 312);
    if (field<uint8_t>(layer + 324) != 0 && old_vm != 0) {
        function_48a430(old_vm, layer + 316);
    }
    field<int32_t>(layer + 308) = address(kinoko_act_host_symbols()->layer_ref_vtable);
    field<int32_t>(layer + 312) = vm;
    field<uint8_t>(layer + 324) = 1;
    sq_resetobject((HSQOBJECT*)kinoko_pointer(layer + 316));
    if (!retdec_sqrat_new_table(vm, table_pair))
        return 0;
    field<int32_t>(layer + 316) = table_pair[0];
    field<int32_t>(layer + 320) = table_pair[1];
    function_48a400(vm, layer + 316);
    retdec_sqrat_release_pair(vm, table_pair);
    script_pair[0] = field<int32_t>(layer + 316);
    script_pair[1] = field<int32_t>(layer + 320);
    if (!retdec_publish_act_script_constants(vm, script_pair))
        return 0;

    old_vm = field<int32_t>(layer + 332);
    if (field<uint8_t>(layer + 344) != 0 && old_vm != 0) {
        function_48a430(old_vm, layer + 336);
    }
    field<int32_t>(layer + 328) = address(kinoko_act_host_symbols()->layer_layout_vtable);
    field<int32_t>(layer + 332) = vm;
    field<uint8_t>(layer + 344) = 1;
    sq_resetobject((HSQOBJECT*)kinoko_pointer(layer + 336));
    return 1;
}

void retdec_publish_c2dlayout_values(
    int32_t vm, const int32_t layout_pair[2], int32_t layout)
{
    /* C2DLayout properties are native descriptor slots.  Writing mirrored
       values into the instance would shadow _get/_set and leave animation
       changes detached from the C2DLayout allocation. */
    (void)vm;
    (void)layout_pair;
    (void)layout;
}

int32_t retdec_map_chip_count(int32_t vm) {
    int32_t layout = 0;
    if (sq_getinstanceup(kinoko_vm(vm), 1, (SQUserPointer*)(&layout), kinoko_pointer(0)) < 0 || layout == 0)
        return 0;
    sq_pushinteger(kinoko_vm(vm), (field<int32_t>(layout + 268) -
                         field<int32_t>(layout + 264)) / 32);
    return 1;
}

int32_t retdec_map_get_chip_layout(int32_t vm) {
    int32_t layout = 0;
    int32_t index = -1;
    int32_t begin;
    int32_t count;
    int32_t root[5];
    int32_t chip_class[2] = { g483, g484 };
    int32_t instance[2] = { g483, g484 };
    if (sq_getinstanceup(kinoko_vm(vm), 1, (SQUserPointer*)(&layout), kinoko_pointer(0)) < 0 || layout == 0 ||
        sq_getinteger(kinoko_vm(vm), 2, (SQInteger*)(&index)) < 0)
        return 0;
    begin = field<int32_t>(layout + 264);
    count = (field<int32_t>(layout + 268) - begin) / 32;
    if (index < 0 || index >= count) {
        sq_pushnull(kinoko_vm(vm));
        return 1;
    }
    if (!retdec_sqrat_root_construct(address(root), vm))
        return 0;
    if (get_pair(address(root), "ChipLayout", chip_class) &&
        retdec_create_unbound_instance(vm, chip_class, begin + 32 * index, instance))
        sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(instance[0], instance[1]));
    else
        sq_pushnull(kinoko_vm(vm));
    retdec_sqrat_release_pair(vm, instance);
    retdec_sqrat_release_pair(vm, chip_class);
    retdec_sqrat_object_release(address(root));
    return 1;
}

int32_t retdec_map_layout_argument(int32_t vm, int32_t *index) {
    int32_t layout = 0;
    if (sq_getinstanceup(kinoko_vm(vm), 1, (SQUserPointer*)(&layout), kinoko_pointer(0)) < 0 || layout == 0 ||
        (index != nullptr && sq_getinteger(kinoko_vm(vm), 2, (SQInteger*)(index)) < 0))
        return 0;
    return layout;
}

int32_t retdec_map_record_at(int32_t layout, int32_t index) {
    int32_t begin = layout ? field<int32_t>(layout + 264) : 0;
    int32_t end = layout ? field<int32_t>(layout + 268) : 0;
    return index >= 0 && index < (end - begin) / 32 ? begin + index * 32 : 0;
}

struct retdec_mcd_data *retdec_map_chip_data(int32_t layout) {
    int32_t resource = layout ? field<int32_t>(layout + 316) : 0;
    return resource ? field<retdec_mcd_data *>(resource + 64) : nullptr;
}

int32_t retdec_map_get_chip_by_position(int32_t vm) {
    int32_t x = 0, y = 0;
    int32_t layout = retdec_map_layout_argument(vm, &x);
    struct retdec_mcd_data *data = retdec_map_chip_data(layout);
    int32_t found = -1;
    if (layout != 0 && data != nullptr && sq_getinteger(kinoko_vm(vm), 3, (SQInteger*)(&y)) >= 0) {
        int32_t layer = field<int32_t>(layout + 312);
        for (int32_t index = 0, record; (record = retdec_map_record_at(layout, index)) != 0; ++index) {
            struct retdec_mcd_chip *chip = retdec_mcd_find_chip(data, field<uint32_t>(record));
            int32_t left = field<int32_t>(record + 4);
            int32_t top = field<int32_t>(record + 8);
            field<float>(record + 12) = (float)left +
                (layer ? field<float>(layer + 144) : 0.0f);
            field<float>(record + 16) = (float)top +
                (layer ? field<float>(layer + 148) : 0.0f);
            if (chip != nullptr && left <= x && top <= y &&
                (int64_t)left + retdec_mcd_i16(chip->bytes + 12) > x &&
                (int64_t)top + retdec_mcd_i16(chip->bytes + 14) > y) {
                found = index;
                break;
            }
        }
    }
    sq_pushinteger(kinoko_vm(vm), found);
    return 1;
}

int32_t retdec_map_set_chip_rect(int32_t vm) {
    int32_t id = 0, rectangle[4];
    int32_t layout = retdec_map_layout_argument(vm, &id);
    struct retdec_mcd_chip *chip = retdec_mcd_find_chip(retdec_map_chip_data(layout), (uint32_t)id);
    if (chip != nullptr) {
        for (int32_t i = 0; i < 4; ++i) {
            if (sq_getinteger(kinoko_vm(vm), i + 3, (SQInteger*)(rectangle + i)) < 0) {
                sq_pushbool(kinoko_vm(vm), ((0) != 0));
                return 1;
            }
        }
        for (int32_t i = 0; i < 4; ++i) {
            int16_t component = (int16_t)rectangle[i];
            std::memcpy(chip->bytes + 8 + i * 2, &component, sizeof(component));
        }
    }
    sq_pushbool(kinoko_vm(vm), ((chip != nullptr) != 0));
    return 1;
}

int32_t retdec_map_set_chip_layout(int32_t vm) {
    int32_t index = -1, left = 0, top = 0;
    int32_t layout = retdec_map_layout_argument(vm, &index);
    int32_t record = retdec_map_record_at(layout, index);
    int32_t ok = record != 0 && sq_getinteger(kinoko_vm(vm), 3, (SQInteger*)(&left)) >= 0 &&
                 sq_getinteger(kinoko_vm(vm), 4, (SQInteger*)(&top)) >= 0;
    if (ok) {
        field<int32_t>(record + 4) = left;
        field<int32_t>(record + 8) = top;
    }
    sq_pushbool(kinoko_vm(vm), ((ok) != 0));
    return 1;
}

int32_t retdec_map_set_chip_id(int32_t vm) {
    int32_t index = -1, id = 0;
    int32_t layout = retdec_map_layout_argument(vm, &index);
    int32_t record = retdec_map_record_at(layout, index);
    int32_t ok = record != 0 && sq_getinteger(kinoko_vm(vm), 3, (SQInteger*)(&id)) >= 0;
    if (ok)
        field<int32_t>(record) = id;
    sq_pushbool(kinoko_vm(vm), ((ok) != 0));
    return 1;
}

int32_t retdec_map_get_chip_id(int32_t vm) {
    int32_t index = -1;
    int32_t layout = retdec_map_layout_argument(vm, &index);
    int32_t record = retdec_map_record_at(layout, index);
    sq_pushinteger(kinoko_vm(vm), record ? field<int32_t>(record) : -1);
    return 1;
}

int retdec_map_compare_records(const void *a, const void *b) {
    const int32_t *left = (const int32_t *)a;
    const int32_t *right = (const int32_t *)b;
    if (left[1] != right[1])
        return left[1] < right[1] ? -1 : 1;
    return left[2] < right[2] ? -1 : left[2] != right[2];
}

int32_t retdec_map_prearrangement(int32_t vm) {
    int32_t layout = retdec_map_layout_argument(vm, nullptr);
    struct retdec_mcd_data *data = retdec_map_chip_data(layout);
    int32_t begin, count;
    if (layout == 0 || data == nullptr) {
        sq_pushinteger(kinoko_vm(vm), (int32_t)E_FAIL);
        return 1;
    }
    begin = field<int32_t>(layout + 264);
    count = (field<int32_t>(layout + 268) - begin) / 32;
    if (count > 1)
        std::qsort(pointer<void>(begin), (size_t)count, 32, retdec_map_compare_records);
    field<int32_t>(layout + 240) = INT32_MIN;
    field<int32_t>(layout + 244) = INT32_MIN;
    std::memset(pointer<void>(layout + 248), 0, 16);
    if (count != 0) {
        field<int32_t>(layout + 248) = field<int32_t>(begin + 4);
        field<int32_t>(layout + 252) = field<int32_t>(begin + 8);
        field<int32_t>(layout + 256) = field<int32_t>(begin + 32 * (count - 1) + 4);
        field<int32_t>(layout + 260) = field<int32_t>(layout + 256);
    }
    for (int32_t i = 0; i < count; ++i) {
        int32_t *record = pointer<int32_t>(begin + i * 32);
        struct retdec_mcd_chip *chip = retdec_mcd_find_chip(data, (uint32_t)record[0]);
        if (chip != nullptr) {
            int32_t width = retdec_mcd_i16(chip->bytes + 12);
            int32_t height = retdec_mcd_i16(chip->bytes + 14);
            if (field<int32_t>(layout + 240) < width)
                field<int32_t>(layout + 240) = width;
            if (field<int32_t>(layout + 244) < height)
                field<int32_t>(layout + 244) = height;
            if (field<int32_t>(layout + 256) < record[1] + width)
                field<int32_t>(layout + 256) = record[1] + width;
            if (field<int32_t>(layout + 260) < record[2] + height)
                field<int32_t>(layout + 260) = record[2] + height;
        }
        if (field<int32_t>(layout + 248) > record[1])
            field<int32_t>(layout + 248) = record[1];
        if (field<int32_t>(layout + 252) > record[2])
            field<int32_t>(layout + 252) = record[2];
    }
    sq_pushinteger(kinoko_vm(vm), 0);
    return 1;
}

// Original 433740 stores the fractional position and truncates it into left.
// 433770 intentionally updates only f_top; preserve that asymmetry.
int32_t retdec_chip_set_fractional_left(int32_t vm) {
    const int32_t chip = retdec_map_layout_argument(vm, nullptr);
    SQFloat value = 0;
    if (!chip || !kinoko::script::upstream::sqrat_float_argument(kinoko_vm(vm), 2, value)) return 0;
    field<float>(chip + 12) = value;
    // __ftol2_sse produces a signed 64-bit integer; the caller keeps EAX.
    const int64_t truncated = std::isfinite(value) &&
        static_cast<double>(value) >= -9223372036854775808.0 &&
        static_cast<double>(value) < 9223372036854775808.0
        ? static_cast<int64_t>(value) : INT64_MIN;
    field<int32_t>(chip + 4) = static_cast<int32_t>(truncated);
    return 0;
}

int32_t retdec_map_get_left(int32_t vm) {
    const int32_t layout = retdec_map_layout_argument(vm, nullptr);
    const int32_t first = retdec_map_record_at(layout, 0);
    sq_pushinteger(kinoko_vm(vm), first ? field<int32_t>(first + 4) : 0);
    return 1;
}

// Original 435F00 scans backwards only within maxChipWidth of the last left.
int32_t retdec_map_get_right(int32_t vm) {
    const int32_t layout = retdec_map_layout_argument(vm, nullptr);
    const int32_t begin = layout ? field<int32_t>(layout + 264) : 0;
    const int32_t end = layout ? field<int32_t>(layout + 268) : 0;
    int32_t right = end != begin ? field<int32_t>(end - 28) : 0;
    auto *data = retdec_map_chip_data(layout);
    if (end != begin && data) {
        const int32_t minimum = static_cast<int32_t>(
            static_cast<uint32_t>(right) - field<uint32_t>(layout + 240));
        for (int32_t record = end - 32; record >= begin; record -= 32) {
            const int32_t left = field<int32_t>(record + 4);
            if (left < minimum) break;
            auto *chip = retdec_mcd_find_chip(data, field<uint32_t>(record));
            if (chip) {
                const int32_t edge = static_cast<int32_t>(static_cast<uint32_t>(left) +
                    retdec_mcd_i16(chip->bytes + 12));
                if (edge >= right) right = edge;
            }
        }
    }
    sq_pushinteger(kinoko_vm(vm), right);
    return 1;
}

int32_t retdec_publish_map_view_class(int32_t vm, int32_t root,
    const char *name, const struct retdec_native_view_property *properties,
    int32_t property_count, int32_t is_map, int32_t out[2]) {
    int32_t get_table[2] = { g483, g484 };
    int32_t set_table[2] = { g483, g484 };
    int32_t base = sq_gettop(kinoko_vm(vm));
    int32_t ok = 0;
    if (get_pair(root, name, out) && out[0] == 0x08004000)
        return 1;
    retdec_sqrat_release_pair(vm, out);
    if (!retdec_sqrat_new_class(vm, out))
        goto cleanup;
    if (!retdec_sqrat_new_table(vm, get_table) ||
        !retdec_sqrat_new_table(vm, set_table) ||
        !retdec_sqrat_initialize_class(vm, out, set_table, get_table,
            address(retdec_sqrat_no_constructor), address(function_41e2c0), address(function_41e260), address(function_431650)))
        goto cleanup;
    for (int32_t i = 0; i < property_count; ++i) {
        const struct retdec_native_view_property *p = properties + i;
        int32_t getter = p->kind == 1 ? address(retdec_c2dlayout_get_float) :
            p->kind == 2 ? address(retdec_cact_layer_get_bool) :
            p->kind == 3 ? address(retdec_native_view_get_short) :
            p->kind == 4 ? address(retdec_cact_layer_get_string) :
                          address(retdec_c2dlayout_get_int);
        int32_t setter = p->kind == 1 ? address(retdec_c2dlayout_set_float) :
            p->kind == 2 ? address(retdec_cact_layer_set_bool) :
            p->kind == 3 ? address(retdec_native_view_set_short) :
            p->kind == 4 ? address(retdec_cact_layer_set_string) :
                          address(retdec_c2dlayout_set_int);
        if (!retdec_sqrat_set_offset_closure(vm, get_table, p->name, p->offset, getter) ||
            !retdec_sqrat_set_offset_closure(vm, set_table, p->name, p->offset, setter))
            goto cleanup;
    }
    if (!is_map && std::strcmp(name, "ChipLayout") == 0 &&
        !retdec_sqrat_set_native_closure(vm, set_table, "f_left",
            address(retdec_chip_set_fractional_left), nullptr, 0))
        goto cleanup;
    if (is_map &&
        (!retdec_sqrat_set_native_closure(vm, get_table, "left",
            address(retdec_map_get_left), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, get_table, "right",
            address(retdec_map_get_right), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, out, "PreArrangement",
            address(retdec_map_prearrangement), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, get_table, "chipCount",
            address(retdec_map_chip_count), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, out, "GetChipLayout",
            address(retdec_map_get_chip_layout), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, out, "GetChipByPosition",
            address(retdec_map_get_chip_by_position), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, out, "SetChipRect",
            address(retdec_map_set_chip_rect), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, out, "SetChipLayout",
            address(retdec_map_set_chip_layout), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, out, "SetChipID",
            address(retdec_map_set_chip_id), nullptr, 0) ||
         !retdec_sqrat_set_native_closure(vm, out, "GetChipID",
            address(retdec_map_get_chip_id), nullptr, 0)))
        goto cleanup;
    ok = retdec_sqrat_set_pair(vm, pointer<const int32_t>(root + 8), name, out);
cleanup:
    retdec_sqrat_release_pair(vm, get_table);
    retdec_sqrat_release_pair(vm, set_table);
    retdec_sqrat_trim_stack(vm, base);
    return ok;
}

int32_t retdec_publish_c2dmaplayout_class(int32_t vm, int32_t root,
                                                int32_t out[2]) {
    static const struct retdec_native_view_property map_properties[] = {
        { "layerType", 236, 0 }, { "maxChipWidth", 240, 0 },
        { "maxChipHeight", 244, 0 }, { "mapChipLeft", 248, 0 },
        { "mapChipRight", 256, 0 }, { "mapChipTop", 252, 0 },
        { "mapChipBottom", 260, 0 }, { "alpha", 320, 1 },
        { "scale", 324, 1 }, { "blend", 328, 0 }
    };
    static const struct retdec_native_view_property chip_properties[] = {
        { "chipID", 0, 0 }, { "left", 4, 0 }, { "top", 8, 0 },
        { "f_left", 12, 1 }, { "f_top", 16, 1 }, { "layoutID", 20, 0 },
        { "visible", 24, 2 }, { "alpha", 28, 1 }
    };
    int32_t chip_class[2] = { g483, g484 };
    int32_t ok = retdec_publish_map_view_class(vm, root, "ChipLayout",
        chip_properties, sizeof(chip_properties) / sizeof(chip_properties[0]), 0, chip_class);
    retdec_sqrat_release_pair(vm, chip_class);
    return ok && retdec_publish_map_view_class(vm, root, "C2DMapLayout",
        map_properties, sizeof(map_properties) / sizeof(map_properties[0]), 1, out);
}

// 433C90: cdecl, one VM argument, HRESULT result (IDA 4341ED: retn).
extern "C" int32_t function_433c90(int32_t vm) {
    if (!vm) return static_cast<int32_t>(E_INVALIDARG);
    int32_t root[5] = {}, klass[2] = { g483, g484 };
    if (!retdec_sqrat_root_construct(address(root), vm)) return static_cast<int32_t>(E_FAIL);
    const auto ok = retdec_publish_c2dmaplayout_class(vm, address(root), klass);
    retdec_sqrat_release_pair(vm, klass);
    retdec_sqrat_object_release(address(root));
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

int32_t retdec_resource_get_chip_info(int32_t vm) {
    int32_t resource = 0, id = 0;
    int32_t root[5];
    int32_t klass[2] = { g483, g484 }, instance[2] = { g483, g484 };
    struct retdec_mcd_chip *chip;
    if (sq_getinstanceup(kinoko_vm(vm), 1, (SQUserPointer*)(&resource), kinoko_pointer(0)) < 0 || resource == 0 ||
        sq_getinteger(kinoko_vm(vm), 2, (SQInteger*)(&id)) < 0)
        return 0;
    chip = retdec_mcd_find_chip(field<retdec_mcd_data *>(resource + 64), (uint32_t)id);
    if (chip == nullptr || !retdec_sqrat_root_construct(address(root), vm)) {
        sq_pushnull(kinoko_vm(vm));
        return 1;
    }
    if (get_pair(address(root), "ChipInfo", klass) &&
        retdec_create_unbound_instance(vm, klass, address(chip->bytes), instance))
        sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(instance[0], instance[1]));
    else
        sq_pushnull(kinoko_vm(vm));
    retdec_sqrat_release_pair(vm, klass);
    retdec_sqrat_release_pair(vm, instance);
    retdec_sqrat_object_release(address(root));
    return 1;
}

int32_t retdec_resource_set_chip_flag(int32_t vm) {
    const int32_t resource = retdec_map_layout_argument(vm, nullptr);
    SQInteger id = 0, flag = -1;
    // Original 42FEF8/42FEFD accepts only bit zero, despite the 64-bit storage.
    auto *data = resource ? field<retdec_mcd_data *>(resource + 64) : nullptr;
    auto *chip = data && kinoko::script::upstream::sqrat_integer_argument(kinoko_vm(vm), 2, id) &&
        kinoko::script::upstream::sqrat_integer_argument(kinoko_vm(vm), 3, flag) && flag == 0
        ? retdec_mcd_find_chip(data, static_cast<uint32_t>(id)) : nullptr;
    if (chip) {
        uint32_t bits;
        std::memcpy(&bits, chip->bytes + 16, sizeof(bits));
        bits = kinoko::script::upstream::sqrat_bool_argument(kinoko_vm(vm), 4) ? bits | 1u : bits & ~1u;
        std::memcpy(chip->bytes + 16, &bits, sizeof(bits));
    }
    sq_pushbool(kinoko_vm(vm), chip != nullptr);
    return 1;
}

int32_t retdec_publish_chip_resource_class(int32_t vm, int32_t root, int32_t out[2]) {

    static const struct retdec_native_view_property info_properties[] = {
        { "chipID", 0, 0 }, { "textureID", 4, 0 },
        { "left", 8, 3 }, { "top", 10, 3 }, { "width", 12, 3 }, { "height", 14, 3 },
        { "flag0", 16, 0 }, { "flag1", 20, 0 }, { "visible", 32, 2 },
        { "boundType", 34, 3 }, { "boundLeft", 36, 3 }, { "boundTop", 38, 3 },
        { "boundWidth", 40, 3 }, { "boundHeight", 42, 3 }
    };
    static const struct retdec_native_view_property resource_properties[] = {
        { "resourceID", 4, 0 }, { "stName", 8, 4 }
    };
    if (get_pair(root, "CActResourceChip", out) && out[0] == 0x08004000) return 1;
    retdec_sqrat_release_pair(vm, out);
    int32_t info[2] = { g483, g484 };
    int32_t ok;
    ok = retdec_publish_map_view_class(vm, root, "ChipInfo",
        info_properties, sizeof(info_properties) / sizeof(info_properties[0]), 0, info) &&
        retdec_publish_map_view_class(vm, root, "CActResourceChip",
            resource_properties, sizeof(resource_properties) / sizeof(resource_properties[0]), 0, out) &&
        retdec_sqrat_set_native_closure(vm, out, "GetChipInfo",
            address(retdec_resource_get_chip_info), nullptr, 0) &&
        retdec_sqrat_set_native_closure(vm, out, "SetChipFlag",
            address(retdec_resource_set_chip_flag), nullptr, 0);
    retdec_sqrat_release_pair(vm, info);
    return ok;
}

// Reconstructed C callers use this cdecl port; the virtual slot has its own
// ECX/stack-cleanup adapter matching original 42F350 (retn 4).
extern "C" int32_t function_42f350(int32_t vm) {
    if (!vm) return static_cast<int32_t>(E_INVALIDARG);
    int32_t root[5] = {}, klass[2] = { g483, g484 };
    if (!retdec_sqrat_root_construct(address(root), vm)) return static_cast<int32_t>(E_FAIL);
    const auto ok = retdec_publish_chip_resource_class(vm, address(root), klass);
    retdec_sqrat_release_pair(vm, klass);
    retdec_sqrat_object_release(address(root));
    return ok ? 0 : static_cast<int32_t>(E_FAIL);
}

extern "C" int32_t __fastcall kinoko_method_register_chip_resource(
    int32_t receiver, void *, int32_t vm) {
    return function_42f350(vm);
}

int32_t retdec_get_act_resource_class(int32_t vm, int32_t resource, int32_t out[2]) {
    if (field<int32_t>(resource) != address(kinoko_act_host_symbols()->chip_resource_vtable)) {
        out[0] = g1079;
        out[1] = g1080;
        function_48a400(vm, address(out));
        return out[0] == 0x08004000;
    }
    int32_t root[5] = {};
    if (!retdec_sqrat_root_construct(address(root), vm)) return 0;
    const auto ok = retdec_publish_chip_resource_class(vm, address(root), out);
    retdec_sqrat_object_release(address(root));
    return ok;
}

int32_t retdec_publish_act_resource_pairs(
    int32_t vm, const int32_t layer_pair[2],
    const int32_t script_pair[2], int32_t resource)
{
    int32_t outer_pair[2] = { g483, g484 };
    int32_t script_resource_pair[2] = { g483, g484 };
    int32_t resource_class_pair[2] = { g483, g484 };
    int32_t null_pair[2] = { 0x01000001, 0 };

    if (vm == 0 || layer_pair == nullptr || script_pair == nullptr)
        return 0;
    if (resource == 0) {
        return retdec_sqrat_raw_set_pair(
                   vm, layer_pair, "resource", null_pair) &&
               retdec_sqrat_raw_set_pair(
                   vm, script_pair, "resource", null_pair);
    }
    if (!retdec_get_act_resource_class(vm, resource, resource_class_pair))
        return 0;

    /* 4467E0 -> 448FB0 uses sq_newslot on the outer layer object. */
    if (!retdec_create_bound_instance(
            vm, layer_pair, "resource", resource_class_pair,
            resource, outer_pair)) {
        retdec_sqrat_release_pair(vm, outer_pair);
        retdec_sqrat_release_pair(vm, resource_class_pair);
        return 0;
    }
    retdec_sqrat_release_pair(vm, outer_pair);

    /* 446920 -> 448910 uses sq_rawset on the script table. */
    if (!retdec_create_unbound_instance(
            vm, resource_class_pair, resource,
            script_resource_pair) ||
        !retdec_sqrat_raw_set_pair(
            vm, script_pair, "resource", script_resource_pair)) {
        retdec_sqrat_release_pair(vm, script_resource_pair);
        retdec_sqrat_release_pair(vm, resource_class_pair);
        return 0;
    }
    retdec_sqrat_release_pair(vm, script_resource_pair);
    retdec_sqrat_release_pair(vm, resource_class_pair);
    return 1;
}

int32_t retdec_publish_act_layers(int32_t vm, int32_t act,
                                          int32_t resource_ptr,
                                          int32_t *active_count)
{
    int32_t root_object[5] = { 0, 0, g483, g484, 1 };
    int32_t act_root_object[5] = { address(kinoko_act_host_symbols()->sq_root_vtable), 0,
                                   g483, g484, 1 };
    int32_t class_pair[2] = { g483, g484 };
    int32_t layout_class_pair[2] = { g483, g484 };
    int32_t parent_pair[2] = { g483, g484 };
    const char *act_name;
    int32_t layer_begin;
    int32_t layer_end;
    int32_t layer_count;

    if (active_count != nullptr)
        *active_count = 0;
    if (vm == 0 || act == 0 ||
        !retdec_sqrat_root_construct(address(root_object), vm))
        return 0;
    if (!get_pair(address(root_object), "CActLayer",
                          class_pair) ||
        class_pair[0] != 0x08004000 || class_pair[1] == 0) {
        retdec_trace("act:layer-class-missing");
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_object_release(address(root_object));
        return 0;
    }
    retdec_trace_i32("act:layer-class", class_pair[1]);

    /* The parent passed to sq_newslot is the ACT's published Squirrel table,
       not the native CAct allocation.  450E30 stored the root table pair on
       the resource, so resolve the ACT by its native stName here. */
    act_name = retdec_std_string_data(act + 16);
    if (resource_ptr == 0 || act_name == nullptr || *act_name == 0 ||
        field<int32_t>(resource_ptr + 156) != 0x0A000020 ||
         field<int32_t>(resource_ptr + 160) == 0) {
        retdec_trace("act:parent-root-invalid");
        retdec_sqrat_release_pair(vm, parent_pair);
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_object_release(address(root_object));
        return 0;
    }
    act_root_object[1] = vm;
    act_root_object[2] = field<int32_t>(resource_ptr + 156);
    act_root_object[3] = field<int32_t>(resource_ptr + 160);
    if (!get_pair(address(act_root_object),
                          act_name, parent_pair) ||
                          parent_pair[0] != 0x0A000020 || parent_pair[1] == 0) {
        retdec_trace("act:parent-table-missing");
        retdec_sqrat_release_pair(vm, parent_pair);
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_object_release(address(root_object));
        return 0;
    }
    retdec_trace_i32("act:parent-table", parent_pair[1]);
    if (!retdec_publish_c2dlayout_class(
            vm, address(root_object))) {
        retdec_trace("act:c2dlayout-class-missing");
    }
    (void)get_pair(address(root_object), "C2DLayout",
                           layout_class_pair);
    retdec_trace_i32("act:layout-class-type", layout_class_pair[0]);
    retdec_trace_i32("act:layout-class-data", layout_class_pair[1]);

    /* 450950:450BB0 publishes every resource before registering layers,
       including font atlases that have no visible layer of their own. */
    {
        int32_t parent_object[5] = {
            address(kinoko_act_host_symbols()->sq_object_vtable), vm, parent_pair[0], parent_pair[1], 0
        };
        int32_t resources[2] = { g483, g484 };
        int32_t resource_class[2];
        int32_t begin = field<int32_t>(act + 224);
        int32_t end = field<int32_t>(act + 228);
        if (!retdec_publish_cact_resource2d_class(vm, address(root_object)) ||
            !get_pair(address(parent_object), "resource", resources)) {
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }
        for (int32_t slot = begin; slot != 0 && slot < end; slot += 4) {
            int32_t resource = field<int32_t>(slot);
            int32_t value[2] = { g483, g484 };
            const char *name = retdec_std_string_data(resource + 8);
            resource_class[0] = g483;
            resource_class[1] = g484;
            if (name != nullptr && *name != 0 &&
                retdec_get_act_resource_class(vm, resource, resource_class) &&
                retdec_create_bound_instance(vm, resources, name, resource_class,
                                               resource, value)) {
                retdec_trace_squirrel_name("act:resource-published", address(name));
            }
            retdec_sqrat_release_pair(vm, value);
            retdec_sqrat_release_pair(vm, resource_class);
        }
        retdec_sqrat_release_pair(vm, resources);
    }

    layer_begin = field<int32_t>(act + 208);
    layer_end = field<int32_t>(act + 212);
    if (layer_begin == 0 || layer_end < layer_begin ||
        ((layer_end - layer_begin) & 3) != 0) {
        retdec_trace("act:layer-vector-invalid");
        retdec_sqrat_release_pair(vm, parent_pair);
        retdec_sqrat_release_pair(vm, layout_class_pair);
        retdec_sqrat_release_pair(vm, class_pair);
        retdec_sqrat_object_release(address(root_object));
        return 0;
    }
    layer_count = (layer_end - layer_begin) / 4;
    for (int32_t index = 0; index < layer_count; ++index) {
        int32_t layer = field<int32_t>(layer_begin + index * 4);
        int32_t layer_pair[2] = { g483, g484 };
        int32_t script_pair[2] = { g483, g484 };
        int32_t layout_pair[2] = { g483, g484 };
        int32_t script_layout_pair[2] = { g483, g484 };
        int32_t inner_layer_pair[2] = { g483, g484 };
        int32_t script_wrapper[5] = { 0, 0, g483, g484, 1 };
        int32_t script_ptr;
        int32_t node;
        int32_t sentinel;
        const char *layer_name;
        const char *script_path;
        int32_t layout = 0;
        int32_t have_layout = 0;

        if (layer == 0)
            continue;
        /* 41F580 saves the loaded position before publishing the layer.
           Scripts use this origin when moving a cursor or restoring a logo. */
        std::memcpy(pointer<void>(layer + 156),
               pointer<const void>(layer + 144), 3 * sizeof(float));
        layer_name = retdec_std_string_data(layer + 112);
        if (layer_name == nullptr || *layer_name == 0 ||
            !retdec_create_bound_instance(vm, parent_pair, layer_name,
                                           class_pair, layer, layer_pair) ||
            !retdec_prepare_cact_layer_objects(vm, layer, script_pair)) {
            retdec_trace_i32("act:layer-publish-failed", index);
            retdec_sqrat_release_pair(vm, layer_pair);
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }
        field<int32_t>(layer + 336) = layer_pair[0];
        field<int32_t>(layer + 340) = layer_pair[1];
        function_48a400(vm, layer + 336);

        /* Scalar and visual fields are class descriptors in the original
           CActLayer binding.  They must not be materialized as instance raw
           slots: doing so shadows _get/_set and leaves animation detached
           from the native layer. */
        if (!retdec_sqrat_raw_set_pair(vm, layer_pair, "script",
                                       script_pair)) {
            retdec_trace_i32("act:layer-fields-failed", index);
            retdec_sqrat_release_pair(vm, layer_pair);
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }

        script_ptr = layer + 204;
        script_path = retdec_std_string_data(script_ptr + 64);
        if (script_path != nullptr && *script_path != 0 &&
            !retdec_sqrat_raw_set_string(vm, script_pair, "filePath",
                                         script_path)) {
            retdec_trace_i32("act:layer-script-fields-failed", index);
            retdec_sqrat_release_pair(vm, layer_pair);
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }
        if (!retdec_sqrat_raw_set_bool(
                vm, script_pair, "compiled",
                field<uint8_t>(script_ptr + 101))) {
            retdec_trace_i32("act:layer-script-compiled-failed", index);
            retdec_sqrat_release_pair(vm, layer_pair);
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }

        /* CActLayer::Register stores thisAct on the script table before
           CActScript::Register captures the callback environment. */
        if (!retdec_sqrat_set_pair(vm, script_pair, "thisAct",
                                   parent_pair)) {
            retdec_trace_i32("act:layer-script-thisact-failed", index);
            retdec_sqrat_release_pair(vm, layer_pair);
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }

        script_wrapper[0] = address(kinoko_act_host_symbols()->sq_object_vtable);
        script_wrapper[1] = vm;
        script_wrapper[2] = script_pair[0];
        script_wrapper[3] = script_pair[1];
        if (field<uint8_t>(script_ptr + 100) != 0) {
            int32_t script_result;
            if (field<uint8_t>(script_ptr + 101) != 0 &&
                field<int32_t>(script_ptr + 92) != 0 &&
                field<int32_t>(script_ptr + 96) >= 2 &&
                field<uint16_t>(field<int32_t>(script_ptr + 92)) ==
                    0xFAFAu) {
                int32_t environment_pair[2] = {
                    script_pair[0], script_pair[1]
                };
                script_result = retdec_execute_embedded_act_script(
                    vm, script_ptr, environment_pair);
            } else {
                int32_t environment_pair[2] = {
                    script_pair[0], script_pair[1]
                };
                script_result = retdec_execute_act_source_script(
                    vm, script_ptr, environment_pair);
            }
            retdec_trace_i32("act:layer-script-result", script_result);
        }
        retdec_copy_act_callback(vm, script_ptr, 4,
                                 address(script_wrapper), "Init");
        retdec_copy_act_callback(vm, script_ptr, 24,
                                 address(script_wrapper), "Update");
        retdec_copy_act_callback(vm, script_ptr, 44,
                                 address(script_wrapper), "OnCreate");
        if (index < 96) {
            int32_t raw_data = field<int32_t>(script_ptr + 92);
            retdec_trace_i32("act:layer-native", layer);
            retdec_trace_squirrel_name("act:layer-name",
                                       address(layer_name));
            if (script_path != nullptr)
                retdec_trace_squirrel_name("act:layer-script-path",
                                           address(script_path));
            retdec_trace_i32("act:layer-script-compiled",
                             field<uint8_t>(script_ptr + 101));
            retdec_trace_i32("act:layer-script-size",
                             field<int32_t>(script_ptr + 96));
            retdec_trace_i32("act:layer-script-first-word",
                             raw_data != 0 ? field<int32_t>(raw_data) : 0);
            retdec_trace_i32("act:layer-init-vm",
                             field<int32_t>(layer + 208));
            retdec_trace_i32("act:layer-init-env-type",
                             field<int32_t>(layer + 212));
            retdec_trace_i32("act:layer-init-env-data",
                             field<int32_t>(layer + 216));
            retdec_trace_i32("act:layer-init-type",
                             field<int32_t>(layer + 220));
            retdec_trace_i32("act:layer-init-data",
                             field<int32_t>(layer + 224));
            retdec_trace_i32("act:layer-update-vm",
                             field<int32_t>(layer + 228));
            retdec_trace_i32("act:layer-update-env-type",
                             field<int32_t>(layer + 232));
            retdec_trace_i32("act:layer-update-env-data",
                             field<int32_t>(layer + 236));
            retdec_trace_i32("act:layer-update-type",
                             field<int32_t>(layer + 240));
            retdec_trace_i32("act:layer-update-data",
                             field<int32_t>(layer + 244));
            retdec_trace_i32("act:layer-oncreate-vm",
                             field<int32_t>(layer + 248));
            retdec_trace_i32("act:layer-oncreate-env-type",
                             field<int32_t>(layer + 252));
            retdec_trace_i32("act:layer-oncreate-env-data",
                             field<int32_t>(layer + 256));
            retdec_trace_i32("act:layer-oncreate-type",
                             field<int32_t>(layer + 260));
            retdec_trace_i32("act:layer-oncreate-data",
                             field<int32_t>(layer + 264));
        }

        sentinel = field<int32_t>(layer + 0xb4);
        node = sentinel != 0 ? field<int32_t>(sentinel) : 0;
        if (index < 16) {
            retdec_trace_i32("act:publish-layer-list-sentinel", sentinel);
            retdec_trace_i32("act:publish-layer-list-first", node);
            retdec_trace_i32("act:publish-layer-list-count",
                             field<int32_t>(layer + 0xb8));
        }
        while (node != 0 && node != sentinel) {
            int32_t key = field<int32_t>(node + 8);
            int32_t candidate = key != 0
                ? field<int32_t>(key + 4) : 0;
            if (index < 16) {
                retdec_trace_i32("act:publish-layer-list-node", node);
                retdec_trace_i32("act:publish-layer-list-key", key);
                retdec_trace_i32("act:publish-layer-list-value", candidate);
                retdec_trace_i32("act:publish-layer-list-vtable",
                                 candidate != 0
                                     ? field<int32_t>(candidate) : 0);
            }
            if (candidate != 0 &&
                (field<int32_t>(candidate) == address(kinoko_act_host_symbols()->layout_vtable) ||
                 field<int32_t>(candidate) == address(kinoko_act_host_symbols()->map_layout_vtable))) {
                layout = candidate;
                have_layout = 1;
                break;
            }
            node = field<int32_t>(node);
        }
        if (index < 16) {
            retdec_trace_i32("act:publish-layer-have-layout", have_layout);
            retdec_trace_i32("act:publish-layer-layout", layout);
        }
        retdec_sqrat_release_pair(vm, layout_class_pair);
        if (have_layout && field<int32_t>(layout) == address(kinoko_act_host_symbols()->map_layout_vtable))
            retdec_publish_c2dmaplayout_class(vm, address(root_object), layout_class_pair);
        else
            get_pair(address(root_object), "C2DLayout", layout_class_pair);
        if (have_layout && layout_class_pair[0] == 0x08004000 &&
            layout_class_pair[1] != 0 &&
            retdec_create_unbound_instance(vm, layout_class_pair, layout,
                                            layout_pair)) {
            (void)retdec_sqrat_raw_set_pair(vm, layer_pair, "layout",
                                             layout_pair);
            retdec_publish_c2dlayout_values(vm, layout_pair, layout);
            retdec_sqrat_release_pair(vm, layout_pair);

            /* C2DLayout::Register repeats the same native association for
               the script table, producing a distinct C2DLayout instance. */
            if (!retdec_create_unbound_instance(
                    vm, layout_class_pair, layout, script_layout_pair) ||
                !retdec_sqrat_set_pair(vm, script_pair, "layout",
                                       script_layout_pair)) {
                retdec_trace_i32("act:script-layout-publish-failed", index);
                retdec_sqrat_release_pair(vm, script_layout_pair);
                retdec_sqrat_release_pair(vm, layer_pair);
                retdec_sqrat_release_pair(vm, parent_pair);
                retdec_sqrat_release_pair(vm, layout_class_pair);
                retdec_sqrat_release_pair(vm, class_pair);
                retdec_sqrat_object_release(address(root_object));
                return 0;
            }
            retdec_publish_c2dlayout_values(vm, script_layout_pair, layout);
            retdec_sqrat_release_pair(vm, script_layout_pair);
        } else {
            int32_t null_pair[2] = { g483, g484 };
            (void)retdec_sqrat_raw_set_pair(vm, layer_pair, "layout",
                                             null_pair);
            if (have_layout)
                retdec_trace_i32("act:layout-instance-missing", index);
        }
        if (!retdec_publish_act_resource_pairs(
                vm, layer_pair, script_pair,
                field<int32_t>(layer + 0x64))) {
            retdec_trace_i32("act:layer-resource-publish-failed", index);
            retdec_sqrat_release_pair(vm, layer_pair);
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }
        /* CActLayer::Register creates a second native wrapper below the
           script table.  It is distinct from the ACT table's layer entry. */
        if (!retdec_create_bound_instance(vm, script_pair, "layer",
                                          class_pair, layer,
                                          inner_layer_pair)) {
            retdec_trace_i32("act:layer-script-layer-failed", index);
            retdec_sqrat_release_pair(vm, layer_pair);
            retdec_sqrat_release_pair(vm, parent_pair);
            retdec_sqrat_release_pair(vm, layout_class_pair);
            retdec_sqrat_release_pair(vm, class_pair);
            retdec_sqrat_object_release(address(root_object));
            return 0;
        }
        retdec_sqrat_release_pair(vm, inner_layer_pair);
        retdec_trace_i32("act:layer-published", index);
        if (active_count != nullptr)
            ++*active_count;
        retdec_sqrat_release_pair(vm, layer_pair);
    }
    retdec_sqrat_release_pair(vm, parent_pair);
    retdec_sqrat_release_pair(vm, layout_class_pair);
    retdec_sqrat_release_pair(vm, class_pair);
    retdec_sqrat_object_release(address(root_object));
    return 1;
}

int32_t retdec_bind_act_resource_object(int32_t resource_ptr)
{
    int32_t holder;
    int32_t act;
    int32_t link;
    int32_t old_link;
    int32_t source;
    int32_t previous;

    retdec_trace_i32("450950:bind-resource", resource_ptr);
    if (resource_ptr == 0)
        return 0;
    holder = field<int32_t>(resource_ptr);
    act = holder != 0 ? field<int32_t>(holder) : 0;
    retdec_trace_i32("450950:bind-holder", holder);
    retdec_trace_i32("450950:bind-act", act);
    if (act == 0)
        return 0;

    source = act;
    act = kinoko_act_clone(source, nullptr);
    if (!act) return 0;
    link = _3f__3f_2_40_YAPAXI_40_Z(4);
    retdec_trace_i32("450950:bind-new-link", link);
    if (link == 0) {
        retdec_destroy_cact_with_flags(act, 1);
        return 0;
    }
    previous = field<int32_t>(resource_ptr + 12);
    if (previous && previous != source)
        retdec_destroy_cact_with_flags(previous, 1);
    field<int32_t>(link) = act;
    old_link = field<int32_t>(resource_ptr + 16);
    retdec_trace_i32("450950:bind-old-link", old_link);
    if (old_link != 0 && old_link != link) {
        retdec_trace("450950:bind-old-link-free-before");
        /* The generated operator-delete shim receives &g1224 at most call
           sites because RetDec lost the original operand.  This call has a
           recovered operand, so release the owned link directly. */
        std::free(pointer<void>(old_link));
        retdec_trace("450950:bind-old-link-free-after");
    }

    field<int32_t>(resource_ptr + 12) = act;
    field<int32_t>(resource_ptr + 16) = link;
    retdec_trace("450950:bind-link-stored");

    /* Exact field map from the original 44FF90(resource, act) call. */
    field<int32_t>(resource_ptr + 108) = act + 72;
    field<int32_t>(resource_ptr + 112) = act + 80;
    field<int32_t>(resource_ptr + 116) = act + 76;
    field<int32_t>(resource_ptr + 120) = act + 84;
    field<int32_t>(resource_ptr + 124) = act + 88;
    field<int32_t>(resource_ptr + 128) = act + 92;
    field<int32_t>(resource_ptr + 132) = act + 96;
    field<int32_t>(resource_ptr + 136) = act + 4;
    field<int32_t>(resource_ptr + 140) = act + 8;
    field<int32_t>(resource_ptr + 144) = act + 12;
    field<int32_t>(resource_ptr + 148) = act + 16;
    retdec_trace("450950:bind-fields-stored");
    return 1;
}

int32_t retdec_register_runtime_act_script(int32_t vm, int32_t resource_ptr,
                                                 int32_t act)
{
    int32_t root[5] = { 0, vm, 0, 0, 0 };
    int32_t parent[2] = { g483, g484 }, global[2] = { g483, g484 };
    int32_t object[5] = { 0, vm, 0, 0, 0 };
    int32_t script = act + 100, result = 0;
    const char *path = retdec_std_string_data(script + 64);
    root[2] = field<int32_t>(resource_ptr + 156);
    root[3] = field<int32_t>(resource_ptr + 160);
    if (!get_pair(address(root), retdec_std_string_data(act + 16), parent))
        goto done;
    object[2] = parent[0]; object[3] = parent[1];
    if (!get_pair(address(object), "global", global))
        goto done;
    if (path && *path) {
        int32_t environment[3] = { address(kinoko_act_host_symbols()->sq_object_vtable), global[0], global[1] };
        function_402d40(const_cast<char*>(path), address(environment));
    } else if (field<uint8_t>(script + 100)) {
        int32_t bytes = field<int32_t>(script + 92);
        int32_t size = field<int32_t>(script + 96);
        int32_t executed;
        if (field<uint8_t>(script + 101) && bytes && size >= 2 &&
            field<uint16_t>(bytes) == 0xfafa)
            executed = retdec_execute_embedded_act_script(vm, script, global);
        else
            executed = retdec_execute_act_source_script(vm, script, global);
        if (!executed) goto done;
    }
    object[0] = address(kinoko_act_host_symbols()->sq_object_vtable);
    object[2] = global[0]; object[3] = global[1];
    retdec_copy_act_callback(vm, script, 4, address(object), "Init");
    retdec_copy_act_callback(vm, script, 24, address(object), "Update");
    retdec_copy_act_callback(vm, script, 44, address(object), "OnCreate");
    result = 1;
done:
    retdec_sqrat_release_pair(vm, global);
    retdec_sqrat_release_pair(vm, parent);
    return result;
}

int32_t retdec_begin_stage_this(int32_t resource_ptr, int32_t stage)
{
    CRITICAL_SECTION *critical_section;
    int32_t vm;
    int32_t result = -0x7fffbffb;

    retdec_trace_i32("450950:enter-resource", resource_ptr);
    retdec_trace_i32("450950:enter-stage", stage);
    if (resource_ptr == 0)
        return result;
    critical_section = pointer<CRITICAL_SECTION>(resource_ptr + 20);
    EnterCriticalSection(critical_section);

    vm = field<int32_t>(resource_ptr + 152);
    if (field<unsigned char>(resource_ptr + 8) == 0 &&
        vm != 0 && field<int32_t>(resource_ptr + 156) ==
            0x0A000020 && field<int32_t>(resource_ptr + 160) != 0 &&
        retdec_bind_act_resource_object(resource_ptr)) {
        if (stage >= 0)
            field<int32_t>(resource_ptr + 4) = stage;
        field<unsigned char>(resource_ptr + 8) = 1;
        result = 0;
    }

    if (result == 0) {
        int32_t act = field<int32_t>(resource_ptr + 12);
        int32_t layer_count = 0;
        if (act == 0 ||
            !retdec_publish_act_layers(vm, act, resource_ptr, &layer_count)) {
            retdec_trace("450950:layer-publish-failed");
            result = -0x7fffbffb;
        } else {
            retdec_trace_i32("450950:layer-count", layer_count);
            /* The native order is root CActScript::Init first, followed by
               each layer's retained Init callback in vector order. */
            int32_t source = *field<int32_t *>(resource_ptr);
            int32_t init_result = retdec_register_runtime_act_script(vm, resource_ptr, act)
                ? retdec_execute_act_callback(source + 100, 4, "act:callback-init")
                : (int32_t)E_FAIL;
            if (init_result < 0) {
                result = init_result;
            } else {
                int32_t layer_begin = field<int32_t>(act + 208);
                int32_t layer_end = field<int32_t>(act + 212);
                int32_t layer_index;
                for (layer_index = 0;
                     layer_begin != 0 && layer_end >= layer_begin &&
                     layer_index < (layer_end - layer_begin) / 4;
                     ++layer_index) {
                    int32_t layer = field<int32_t>(
                        layer_begin + layer_index * 4);
                    int32_t layer_init_result = retdec_execute_act_callback(
                        layer != 0 ? layer + 204 : 0, 4,
                        "act:layer-callback-init");
                    if (layer_init_result < 0) {
                        result = layer_init_result;
                        break;
                    }
                }
            }
        }
    }

    LeaveCriticalSection(critical_section);
    {
        int32_t act = field<int32_t>(resource_ptr + 12);
        const char *act_name = act != 0
            ? retdec_std_string_data(act + 16) : nullptr;
        retdec_trace_squirrel_name("450950:act-name",
                                   address(act_name));
        retdec_trace_i32("450950:user", resource_ptr);
    }
    retdec_trace_i32("450950:resource", resource_ptr);
    retdec_trace_i32("450950:vm", vm);
    retdec_trace_i32("450950:result", result);
    if (result == 0) {
        retdec_trace_i32("450950:act", field<int32_t>(resource_ptr));
        retdec_trace_i32("450950:active",
                         field<int32_t>(resource_ptr + 8));
    }
    return result;
}

int32_t retdec_root_table_register_resource(int32_t root_object,
                                                    int32_t resource_ptr)
{
    int32_t act_pair[2] = { g483, g484 };
    int32_t global_pair[2] = { g483, g484 };
    int32_t resource_pair[2] = { g483, g484 };
    int32_t player_pair[2] = { g483, g484 };
    int32_t holder;
    int32_t act;
    const char *act_name;
    const char *script_path;
    int32_t global_object[5] = { 0, 0, g483, g484, 1 };
    int32_t script_environment[3] = { 0, g483, g484 };
    int32_t script_ptr;
    int32_t result = 0;
    int32_t vm;

    if (root_object == 0 || resource_ptr == 0)
        return 0;
    vm = field<int32_t>(root_object + 4);
    if (vm == 0)
        return 0;
    retdec_trace_i32("450f30:root", root_object);
    retdec_trace_i32("450f30:resource", resource_ptr);
    retdec_trace_i32("450f30:vm", vm);

    if (!retdec_bind_act_resource_root(resource_ptr, vm, pointer<const int32_t>(root_object + 8))) {
        retdec_trace("450f30:root-bind-failed");
        goto cleanup;
    }
    if (!retdec_publish_cact_resource2d_class(
            vm, root_object)) {
        retdec_trace("450f30:resource2d-class-failed");
        goto cleanup;
    }
    holder = field<int32_t>(resource_ptr);
    act = holder != 0 ? field<int32_t>(holder) : 0;
    if (act == 0)
        goto cleanup;
    act_name = retdec_std_string_data(act + 16);
    if (act_name == nullptr || *act_name == 0)
        goto cleanup;
    /* 451022..45104A retains the registration key for 4513F0 teardown. */
    retdec_string_assign_cstr(pointer<int32_t>(resource_ptr + 164), act_name);
    retdec_trace_squirrel_name("450f30:act-name", address(act_name));

    if (!retdec_publish_cact_layer_class(vm, root_object) ||
        !retdec_publish_acting_player_class(vm, root_object) ||
        !retdec_sqrat_new_table(vm, act_pair) ||
        !retdec_sqrat_set_pair(vm,
                               pointer<const int32_t>(root_object + 8),
                               act_name, act_pair) ||
        !retdec_sqrat_new_table(vm, global_pair) ||
        !retdec_sqrat_new_table(vm, resource_pair))
        goto cleanup;

    if (std::strcmp(act_name, "Fader2") == 0 &&
        global_pair[0] == 0x0A000020 && global_pair[1] != 0 &&
        !retdec_is_release_watch_data(global_pair[1]) &&
        retdec_release_watch_count < 8) {
        retdec_release_watch_data[retdec_release_watch_count++] =
            global_pair[1];
        retdec_trace_squirrel_name("sq-watch:fader-global-name",
                                   address(act_name));
        retdec_trace_i32("sq-watch:fader-global-data", global_pair[1]);
        retdec_trace_i32("sq-watch:fader-global-internal",
                         field<int32_t>(global_pair[1] + 4));
        retdec_trace_ref_watch("fader-global-created",
                               retdec_primary_shared_state,
                               global_pair[0], global_pair[1]);
    }

    retdec_sqrat_set_string(vm, act_pair, "stName", act_name);
    retdec_sqrat_set_int(vm, act_pair, "resolutionMs",
                         field<int32_t>(act + 4));
    retdec_sqrat_set_int(vm, act_pair, "screenWidth",
                         field<int32_t>(act + 8));
    retdec_sqrat_set_int(vm, act_pair, "screenHeight",
                         field<int32_t>(act + 12));
    if (!retdec_sqrat_set_pair(vm, act_pair, "global", global_pair) ||
        !retdec_sqrat_set_pair(vm, act_pair, "resource", resource_pair) ||
        !retdec_sqrat_set_pair(vm, global_pair, "thisAct", act_pair) ||
        !retdec_sqrat_set_delegate(vm, global_pair, act_pair) ||
        !retdec_publish_act_script_constants(vm, global_pair))
        goto cleanup;

    if (!retdec_publish_acting_player(vm, act_pair, "pl", resource_ptr,
                                      player_pair))
        goto cleanup;
    retdec_sqrat_release_pair(vm, player_pair);
    if (!retdec_publish_acting_player(vm, act_pair, "player", resource_ptr,
                                      player_pair))
        goto cleanup;
    retdec_sqrat_release_pair(vm, player_pair);

    script_ptr = act + 100;
    script_path = retdec_std_string_data(script_ptr + 64);
    if (script_path != nullptr && *script_path != 0) {
        script_environment[0] = address(kinoko_act_host_symbols()->sq_object_vtable);
        script_environment[1] = global_pair[0];
        script_environment[2] = global_pair[1];
        retdec_trace_squirrel_name("450f30:script-path",
                                   address(script_path));
        function_402d40(const_cast<char*>(script_path),
                        address(script_environment));
        global_object[0] = address(kinoko_act_host_symbols()->sq_object_vtable);
        global_object[1] = vm;
        global_object[2] = global_pair[0];
        global_object[3] = global_pair[1];
        retdec_copy_act_callback(vm, script_ptr, 4,
                                 address(global_object), "Init");
        retdec_copy_act_callback(vm, script_ptr, 24,
                                 address(global_object), "Update");
        retdec_copy_act_callback(vm, script_ptr, 44,
                                 address(global_object), "OnCreate");
        retdec_execute_act_callback(script_ptr, 44,
                                    "act:callback-oncreate");
    } else if (field<uint8_t>(script_ptr + 100) != 0) {
        int32_t environment_pair[2] = { global_pair[0], global_pair[1] };
        int32_t script_result;
        if (field<uint8_t>(script_ptr + 101) != 0 &&
            field<int32_t>(script_ptr + 92) != 0 &&
            field<int32_t>(script_ptr + 96) >= 2 &&
            field<uint16_t>(field<int32_t>(script_ptr + 92)) ==
                0xFAFAu) {
            script_result = retdec_execute_embedded_act_script(
                vm, script_ptr, environment_pair);
        } else {
            script_result = retdec_execute_act_source_script(
                vm, script_ptr, environment_pair);
        }
        retdec_trace_i32("450f30:embedded-script-result", script_result);
        if (script_result) {
            global_object[0] = address(kinoko_act_host_symbols()->sq_object_vtable);
            global_object[1] = vm;
            global_object[2] = global_pair[0];
            global_object[3] = global_pair[1];
            retdec_copy_act_callback(vm, script_ptr, 4,
                                     address(global_object), "Init");
            retdec_copy_act_callback(vm, script_ptr, 24,
                                     address(global_object), "Update");
            retdec_copy_act_callback(vm, script_ptr, 44,
                                     address(global_object), "OnCreate");
            retdec_execute_act_callback(script_ptr, 44,
                                        "act:callback-oncreate");
        }
    }

    /* 466100 only publishes the ACT.  BeginStage is a script-facing
       operation; forcing it here activates PlayerStatus/StageClear before
       the scene selects them and makes them cover TitleMenu. */
    retdec_trace("act:published-without-begin-stage");
    result = 1;

cleanup:
    if (player_pair[0] != g483)
        retdec_sqrat_release_pair(vm, player_pair);
    if (resource_pair[0] != g483)
        retdec_sqrat_release_pair(vm, resource_pair);
    if (global_pair[0] != g483)
        retdec_sqrat_release_pair(vm, global_pair);
    if (act_pair[0] != g483)
        retdec_sqrat_release_pair(vm, act_pair);
    return result;
}

int32_t retdec_root_table_construct_this(int32_t resource_ptr,
                                                 int32_t vm,
                                                 int32_t output_ptr)
{
    int32_t root_object[5] = { 0, 0, g483, g484, 0 };
    int32_t result;

    if (resource_ptr == 0 || vm == 0)
        return (int32_t)0x80070057u;
    if (!retdec_sqrat_root_construct(address(root_object), vm))
        return (int32_t)0x80004005u;

    /* 450E30 accepts an optional pre-existing Sqrat object only to verify
       that it belongs to the same VM.  The normal loader passes nullptr. */
    if (output_ptr != 0 &&
        field<int32_t>(output_ptr + 4) != vm) {
        retdec_sqrat_object_release(address(root_object));
        return (int32_t)0x80070057u;
    }

    result = retdec_root_table_register_resource(
        address(root_object), resource_ptr);
    retdec_sqrat_object_release(address(root_object));
    return result;
}
