#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/squirrel_legacy_api.h"
#include "script_registration_host.hpp"
#include "kinoko/savedata.h"

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
inline int32_t& primary_vm_slot = kinoko_act_vm_abi_slot;
inline auto show_call_stack_entry = kinoko_native_no_arguments_entry;
template<class Function> void* entry(Function function) {
    return reinterpret_cast<void*>(function);
}
struct NativeMethod { const char* name; void* target; SQFUNCTION wrapper; };
// Original 473010 order; targets retain their existing recovered ABI adapters.
const NativeMethod methods[] = {
    {"PostQuitMessage", entry(kinoko_host_close_window), kinoko_native_void_entry},
    {"ReadCSV", entry(kinoko_script_read_csv), kinoko_native_string_object_result_entry},
    {"LoadTable", entry(kinoko_savedata_load_file_entry), kinoko_native_string_object_result_entry},
    {"SaveTable", entry(kinoko_savedata_save_file_entry), kinoko_native_string_object_result_entry},
    {"SetGlobalUpdateFunction", entry(kinoko_script_set_global_update), kinoko_script_global_update_entry},
    {"SetInitFunctionByID", entry(kinoko_script_set_init), kinoko_native_integer_pair_entry},
    {"LoadAnimationData", entry(kinoko_script_load_animation), kinoko_native_string_bool_result_entry},
    {"CreateActor", entry(kinoko_script_create_actor), kinoko_script_create_actor_entry},
    {"CreateActorFromMap", entry(kinoko_script_create_map_actors), kinoko_native_string_object_callback},
    {"ClearActor", entry(kinoko_script_clear_actors), kinoko_native_void_entry},
    {"MoveActor", entry(kinoko_script_move_actors), kinoko_native_two_floats_entry},
    {"ClearCollision", entry(kinoko_script_clear_collision), kinoko_native_void_entry},
    {"CreateCollision", entry(kinoko_script_create_collision), kinoko_native_string_entry},
    {"CreateEvent", entry(kinoko_script_create_event), kinoko_native_create_event_callback},
    {"ClearRenderLayer", entry(kinoko_script_clear_render_layers), kinoko_native_void_entry},
    {"CreateRenderLayer", entry(kinoko_script_create_render_layer), kinoko_native_string_entry},
    {"LoadAct", entry(kinoko_script_load_act), kinoko_native_string_bool_result_entry},
    {"LoadMap", entry(kinoko_script_load_map), kinoko_native_string_bool_result_entry},
    {"LoadSE", entry(kinoko_audio_load_sound_table), kinoko_native_string_entry},
    {"ReleaseMap", entry(kinoko_script_release_map), kinoko_native_void_entry},
    {"MessageBox", entry(kinoko_host_show_message_abi), kinoko_native_string_entry},
    {"dprint", entry(kinoko_script_dprint_noop), kinoko_native_string_entry},
    {"Sleep", entry(kinoko_host_sleep_abi), kinoko_native_integer_entry},
    {"timeGetTime", entry(kinoko_host_milliseconds), kinoko_native_integer_result_entry},
    {"PlaySE", entry(kinoko_audio_play_sound), kinoko_native_integer_entry},
    {"PlayBgm", entry(kinoko_audio_play_bgm), kinoko_native_string_two_integer_truth_callback},
    {"PlayBgmMargin", entry(kinoko_audio_play_bgm_margin), kinoko_native_string_three_integer_truth_callback},
    {"FadeBgm", entry(kinoko_audio_fade_bgm), kinoko_native_two_integer_entry},
    {"StopBgm", entry(kinoko_audio_stop_bgm), kinoko_native_void_entry},
    {"PauseBgm", entry(kinoko_audio_pause_bgm), kinoko_native_void_entry},
};
struct Constant { const char* name; int32_t value; };
constexpr Constant constants[] = {
    {"GP_CAMERA", 0x20000000}, {"GP_ACT", 0x40000000},
    {"GP_BACKGROUND", INT32_MIN}, {"PR_BACK", -1},
    {"PR_FRONT", 0xffff}, {"PR_WATER", 0x10000},
};
} // namespace

extern "C" void kinoko_register_global_methods(int32_t root_table) {
    // Store the address directly: MSVC's generic entry deduction can lose the
    // explicit throwing C-linkage function type under /EHsc.
    void* target = entry(reinterpret_cast<int32_t (__cdecl *)(void)>(
        &kinoko_script_show_call_stack));
    kinoko_sqrat_bind_object_function(pointer<void>(root_table), "ShowCallStack", &target, 4,
        entry(show_call_stack_entry), 0);
    target = entry(&kinoko_script_compile_file_argument);
    kinoko_sqrat_bind_object_function(pointer<void>(root_table), "CompileFile", &target, 4,
        entry(kinoko_compile_file_native), 0);
    for (const auto& method : methods) {
        auto* vm = current_vm();
        sq_pushroottable(vm);
        sq_pushstring(vm, method.name, -1);
        std::memcpy(sq_newuserdata(vm, sizeof method.target), &method.target, sizeof method.target);
        sq_newclosure(vm, method.wrapper, 1);
        sq_newslot(vm, -3, SQFalse);
        sq_pop(vm, 1);
    }
}

namespace {
// The root Sqrat wrapper is five Win32 words: vtable, VM, HSQOBJECT and flag.
// Its HSQOBJECT is an external reference, not a SQObjectPtr placement object.
struct RootTableStorage {
    const void* vtable;
    SQVM *vm;
    HSQOBJECT value;
    int32_t owns_value;
};
static_assert(sizeof(RootTableStorage) == 20);
static_assert(offsetof(RootTableStorage, value) == 8);
static_assert(offsetof(RootTableStorage, owns_value) == 16);

void bind_root_mask(ObjectStorage &object, int32_t *storage, const char *name,
                    const char *result_label, const char *storage_label,
                    const char *type_label, const char *data_label,
                    const char *present_label) {
    kinoko_sqplus_object_copy_construct(&object, kinoko_sqplus_root_object());
    const auto result = kinoko_script_bind_root_value(reinterpret_cast<int32_t *>(&object),
        storage, const_cast<char *>(name), 0);
    const auto value = ObjectView(&object).value();
    kinoko_trace_i32(result_label, result);
    kinoko_trace_i32(storage_label, *storage);
    kinoko_trace_i32(type_label, value._type);
    kinoko_trace_i32(data_label, data_bits(value));
    kinoko_trace_i32(present_label, kinoko_sqplus_object_exists(&object, name));
    kinoko_sqplus_object_destroy(&object);
}

int32_t kinoko_register_root_bindings() {
    kinoko_trace("473010:enter");
    kinoko_script_initialize_root();
    primary_vm_slot = address(current_vm());
    const int32_t vm_address = primary_vm_slot;
    RootTableStorage root{};
    root.vtable = kinoko_sqrat_object_vtable();
    root.vm = current_vm();
    root.owns_value = 1;
    sq_resetobject(&root.value);
    root.vtable = kinoko_sqrat_root_vtable();
    sq_pushroottable(current_vm());
    sq_getstackobj(current_vm(), -1, &root.value);
    sq_addref(root.vm, &root.value);
    sq_pop(current_vm(), 1);
    kinoko_register_global_methods(address(&root));

    ObjectStorage object{};
    bind_root_mask(object, &kinoko_game_masks.update, "updateMask",
        "473010:update-bind-result", "473010:update-storage",
        "473010:update-object-type", "473010:update-object-data",
        "473010:update-present");
    bind_root_mask(object, &kinoko_game_masks.render, "renderMask",
        "473010:render-bind-result", "473010:render-storage",
        "473010:render-object-type", "473010:render-object-data",
        "473010:render-present");
    for (const auto &constant : constants) {
        kinoko_sqplus_object_copy_construct(&object, kinoko_sqplus_root_object());
        kinoko_script_bind_root_integer(reinterpret_cast<int32_t *>(&object),
                        constant.value, const_cast<char *>(constant.name));
        kinoko_sqplus_object_destroy(&object);
    }
    kinoko_actor_register_script_class();
    kinoko_register_input_class();
    kinoko_register_camera_binding();
    kinoko_register_map_binding();
    kinoko_script_load_file(const_cast<char *>("data/script/class_def.nut"),
                            kinoko_script_root());
    return sq_release(root.vm, &root.value);
}
} // namespace
extern "C" int32_t kinoko_register_root_bindings_entry(void) { return kinoko_register_root_bindings(); }
