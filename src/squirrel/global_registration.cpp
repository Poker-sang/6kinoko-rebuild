#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/squirrel_legacy_api.h"
#include "script_registration_host.hpp"

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
template<class Function> int32_t entry(Function function) {
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(function));
}
struct NativeMethod { const char* name; int32_t target; int32_t wrapper; };
// Original 473010 order; targets retain their existing recovered ABI adapters.
const NativeMethod methods[] = {
    {"PostQuitMessage", entry(function_471080), entry(function_471bc0)},
    {"ReadCSV", entry(function_403000), entry(function_471c10)},
    {"LoadTable", entry(function_472c90), entry(function_471c10)},
    {"SaveTable", entry(function_472e50), entry(function_471c10)},
    {"SetGlobalUpdateFunction", entry(kinoko_script_set_global_update), entry(kinoko_script_global_update_entry)},
    {"SetInitFunctionByID", entry(kinoko_script_set_init), entry(function_471d30)},
    {"LoadAnimationData", entry(kinoko_script_load_animation), entry(function_471d90)},
    {"CreateActor", entry(kinoko_script_create_actor), entry(kinoko_script_create_actor_entry)},
    {"CreateActorFromMap", entry(kinoko_script_create_map_actors), entry(function_471e50)},
    {"ClearActor", entry(kinoko_script_clear_actors), entry(function_471bc0)},
    {"MoveActor", entry(kinoko_script_move_actors), entry(function_471eb0)},
    {"ClearCollision", entry(kinoko_script_clear_collision), entry(function_471bc0)},
    {"CreateCollision", entry(kinoko_script_create_collision), entry(function_471f10)},
    {"CreateEvent", entry(kinoko_script_create_event), entry(function_471f70)},
    {"ClearRenderLayer", entry(kinoko_script_clear_render_layers), entry(function_471bc0)},
    {"CreateRenderLayer", entry(kinoko_script_create_render_layer), entry(function_471f10)},
    {"LoadAct", entry(kinoko_script_load_act), entry(function_471d90)},
    {"LoadMap", entry(kinoko_script_load_map), entry(function_471d90)},
    {"LoadSE", entry(kinoko_audio_load_sound_table), entry(function_471f10)},
    {"ReleaseMap", entry(kinoko_script_release_map), entry(function_471bc0)},
    {"MessageBox", entry(function_470f60), entry(function_471f10)},
    {"dprint", entry(function_43e100), entry(function_471f10)},
    {"Sleep", entry(function_470f80), entry(function_471fd0)},
    {"timeGetTime", entry(function_470f90), entry(function_472030)},
    {"PlaySE", entry(kinoko_audio_play_sound), entry(function_471fd0)},
    {"PlayBgm", entry(kinoko_audio_play_bgm), entry(function_472080)},
    {"PlayBgmMargin", entry(kinoko_audio_play_bgm_margin), entry(function_4720e0)},
    {"FadeBgm", entry(kinoko_audio_fade_bgm), entry(function_472140)},
    {"StopBgm", entry(kinoko_audio_stop_bgm), entry(function_471bc0)},
    {"PauseBgm", entry(kinoko_audio_pause_bgm), entry(function_471bc0)},
};
struct Constant { const char* name; int32_t value; };
constexpr Constant constants[] = {
    {"GP_CAMERA", 0x20000000}, {"GP_ACT", 0x40000000},
    {"GP_BACKGROUND", INT32_MIN}, {"PR_BACK", -1},
    {"PR_FRONT", 0xffff}, {"PR_WATER", 0x10000},
};
} // namespace

extern "C" void kinoko_register_global_methods(int32_t root_table) {
    int32_t target = entry(function_402af0);
    (int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(root_table), (const char *)("ShowCallStack"), (const void *)(&target), 4, (void *)(intptr_t)(entry(function_470ee0)), 0));
    target = entry(function_471b30);
    (int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(root_table), (const char *)("CompileFile"), (const void *)(&target), 4, (void *)(intptr_t)(entry(retdec_compile_file_native)), 0));
    for (const auto& method : methods) {
        auto* vm = current_vm();
        sq_pushroottable(vm);
        sq_pushstring(vm, method.name, -1);
        std::memcpy(sq_newuserdata(vm, 4), &method.target, 4);
        sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(pointer(method.wrapper)), 1);
        sq_newslot(vm, -3, SQFalse);
        sq_pop(vm, 1);
    }
}

extern "C" int32_t function_473010(void) {
    retdec_trace("473010:enter");
    function_402aa0();
    g664 = address(g644);
    int32_t root_table[5];
    root_table[0] = kinoko_sqrat_object_vtable();
    root_table[1] = g664;
    root_table[4] = 1;
    auto* root_value = reinterpret_cast<HSQOBJECT*>(root_table + 2);
    sq_resetobject(root_value);
    root_table[0] = kinoko_sqrat_root_vtable();
    sq_pushroottable(current_vm());
    sq_getstackobj(current_vm(), -1, root_value);
    function_48a400(g664, address(root_value));
    sq_pop(current_vm(), 1);
    kinoko_register_global_methods(address(root_table));
    int32_t object[3];
    (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(object), (const void *)(intptr_t)((int32_t)(intptr_t)(kinoko_sqplus_root_object()))));
    const auto update_result = function_4721a0(object, &kinoko_game_masks.update, const_cast<char*>("updateMask"), 0);
    retdec_trace_i32("473010:update-bind-result", update_result);
    retdec_trace_i32("473010:update-storage", kinoko_game_masks.update);
    retdec_trace_i32("473010:update-object-type", object[1]);
    retdec_trace_i32("473010:update-object-data", object[2]);
    retdec_trace_i32("473010:update-present", kinoko_sqplus_object_exists((void *)(object), "updateMask"));
    kinoko_sqplus_object_destroy((void *)(object));
    (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(object), (const void *)(intptr_t)((int32_t)(intptr_t)(kinoko_sqplus_root_object()))));
    const auto render_result = function_4721a0(object, &kinoko_game_masks.render, const_cast<char*>("renderMask"), 0);
    retdec_trace_i32("473010:render-bind-result", render_result);
    retdec_trace_i32("473010:render-storage", kinoko_game_masks.render);
    retdec_trace_i32("473010:render-object-type", object[1]);
    retdec_trace_i32("473010:render-object-data", object[2]);
    retdec_trace_i32("473010:render-present", kinoko_sqplus_object_exists((void *)(object), "renderMask"));
    kinoko_sqplus_object_destroy((void *)(object));
    for (const auto& constant : constants) {
        (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(object), (const void *)(intptr_t)((int32_t)(intptr_t)(kinoko_sqplus_root_object()))));
        function_472240(object, constant.value, const_cast<char*>(constant.name));
        kinoko_sqplus_object_destroy((void *)(object));
    }
    kinoko_actor_register_script_class();
    function_46d950();
    function_4669d0();
    function_46fac0();
    function_402d40(const_cast<char*>("data/script/class_def.nut"), function_402d30());
    return function_48a430(address(g644), address(root_table + 2));
}
