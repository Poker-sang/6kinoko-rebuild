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
    {"ReadCSV", entry(kinoko_script_read_csv), entry(function_471c10)},
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
    {"dprint", entry(kinoko_script_dprint_noop), entry(function_471f10)},
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
    // Store the address directly: MSVC's generic entry deduction can lose the
    // explicit throwing C-linkage function type under /EHsc.
    int32_t target = static_cast<int32_t>(reinterpret_cast<intptr_t>(&kinoko_script_show_call_stack));
    kinoko_sqrat_bind_object_function((void *)(intptr_t)(root_table), (const char *)("ShowCallStack"), (const void *)(&target), 4, (void *)(intptr_t)(entry(function_470ee0)), 0);
    target = static_cast<int32_t>(reinterpret_cast<intptr_t>(&kinoko_script_compile_file_argument));
    kinoko_sqrat_bind_object_function((void *)(intptr_t)(root_table), (const char *)("CompileFile"), (const void *)(&target), 4, (void *)(intptr_t)(entry(retdec_compile_file_native)), 0);
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

namespace {
// The root Sqrat wrapper is five Win32 words: vtable, VM, HSQOBJECT and flag.
// Its HSQOBJECT is an external reference, not a SQObjectPtr placement object.
struct RootTableStorage {
    uint32_t vtable;
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
    retdec_trace_i32(result_label, result);
    retdec_trace_i32(storage_label, *storage);
    retdec_trace_i32(type_label, value._type);
    retdec_trace_i32(data_label, data_bits(value));
    retdec_trace_i32(present_label, kinoko_sqplus_object_exists(&object, name));
    kinoko_sqplus_object_destroy(&object);
}

int32_t kinoko_register_root_bindings() {
    retdec_trace("473010:enter");
    kinoko_script_initialize_root();
    const int32_t vm_address = address(current_vm());
    RootTableStorage root{};
    root.vtable = kinoko_sqrat_object_vtable();
    root.vm = current_vm();
    root.owns_value = 1;
    sq_resetobject(&root.value);
    root.vtable = kinoko_sqrat_root_vtable();
    sq_pushroottable(current_vm());
    sq_getstackobj(current_vm(), -1, &root.value);
    function_48a400(vm_address, address(&root.value));
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
    return function_48a430(vm_address, address(&root.value));
}
} // namespace
extern "C" int32_t function_473010(void) { return kinoko_register_root_bindings(); }
