// Windows x86 runtime host. Original evidence: src/decompiled/6kinoko.exe.c.
#include "runtime_host_internal.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/map_manager_records.hpp"
#include "kinoko/collision_records.hpp"
#include "kinoko/input_manager.h"
#include <array>
#include <cstddef>

namespace {
// Retain the established host extents; record sizes are verified prefixes,
// not guesses at the original complete C++ class sizes. All storage is trivial.
template<class Record, size_t Extent> struct alignas(8) HostStorage {
    Record record;
    std::array<unsigned char, Extent - sizeof(Record)> reserved;
};
HostStorage<kinoko::actor::ManagerPrefix, 0x200> actor_state{};
HostStorage<kinoko::map::ManagerRecord, 0x200> map_state{};
HostStorage<kinoko::camera::Record, 0x200> camera_state{};
HostStorage<KinokoInputManager, 0x600> input_state{};
HostStorage<kinoko::collision::StateRecord, 120> collision_state{};
KinokoScriptCallback global_callback{};
static_assert(sizeof(actor_state) == 0x200 && sizeof(map_state) == 0x200);
static_assert(sizeof(camera_state) == 0x200 && sizeof(input_state) == 0x600);
static_assert(sizeof(collision_state) == 120 && sizeof(global_callback) == 28);
}

#pragma comment(linker, "/alternatename:_D3DXCreateTexture@32=_D3DXCreateTexture")

int32_t retdec_primary_shared_state;

int32_t retdec_release_watch_data[8];

int32_t retdec_release_watch_count;

int32_t retdec_is_release_watch_data(int32_t data) {
    int32_t index;

    if (data == 0)
        return 0;
    for (index = 0; index < retdec_release_watch_count && index < 8;
         ++index) {
        if (retdec_release_watch_data[index] == data)
            return 1;
    }
    return 0;
}

int32_t kinoko_host_get_delegate_abi(int32_t source_ptr, int32_t *target_ptr);

const char * kinoko_application_error_text = "\x8f\x89\x8a\xfa\x89\xbb\x8e\xb8\x94s";

const char * kinoko_application_title_text = "\x96\x82\x97\x9d\x8d\xb9\x82\xc6\x82U\x82\xc2\x82\xcc\x83L\x83m\x83R";

const char * kinoko_audio_error_text = "DirectSound\x83I\x83u\x83W\x83\x46\x83N\x83g\x82\xcc\x8d\xec\x90\xac\x82\xc9\x8e\xb8\x94s";

int32_t kinoko_null_object_type = 0x01000001;

int32_t kinoko_null_object_value = 0;

int32_t kinoko_ime_context_slot = 0;

int32_t kinoko_ime_default_window_slot = 0;

int32_t kinoko_ime_text_limit = 1023;

int32_t kinoko_ime_commit_pending = 0;

char kinoko_ime_text_changed = 0;

char kinoko_ime_composition_changed = 0;

char kinoko_ime_enabled = 0;

int32_t kinoko_ime_cursor = 0;

int32_t kinoko_act_script_extension[7] = {0, 0, 0, 0, 0, 15, 0};

char kinoko_sqrat_trace_enabled = 1;

int32_t kinoko_actor_user_key_storage[3] = { 0, 0, 0 };

int32_t kinoko_actor_step_key_storage[3] = { 0, 0, 0 };

int32_t kinoko_actor_class_storage[3] = { 0, 0, 0 };

int32_t kinoko_stage_list_slot = 0;

int32_t kinoko_stage_count = 0;

int32_t kinoko_camera_class_storage[3] = { 0, 0, 0 };

int32_t kinoko_render_layer_owner_slot = 0;

int32_t kinoko_input_class_storage[3] = { 0, 0, 0 };

int32_t kinoko_map_class_storage[3] = { 0, 0, 0 };

int32_t kinoko_active_bgm_slot = 0;

KinokoIntegerMap* kinoko_sound_lookup = NULL;

int32_t kinoko_sound_lookup_count = 0;

char kinoko_skip_vm_owner_reset = 0;

int32_t kinoko_newest_shared_state = 0;

#pragma data_seg(".g644")
__declspec(align(4096)) struct SQVM *kinoko_primary_vm = NULL;
#pragma data_seg()

int32_t kinoko_cached_root_slot = 0;

int32_t kinoko_vm_thread_wrapper[3] = { 0, 0, 0 };

int32_t kinoko_act_vm_abi_slot = 0;

char kinoko_compile_act_output = 0;

KinokoRenderer kinoko_renderer = {0};

KinokoGraphics kinoko_graphics = {0};

KinokoCriticalSection kinoko_graphics_lock = { 0 };

int32_t kinoko_script_root_storage[3] = { 0, 0, 0 };

char * kinoko_game_window_slot;

unsigned char g_retdec_keyboard_state[256];

char kinoko_packed_assets = 0;

int32_t kinoko_audio_primary_device_slot = 0;

char * kinoko_audio_device_slot;

int32_t kinoko_audio_listener_slot = 0;

int32_t kinoko_layout_type_identity = 0;

int32_t kinoko_chip_type_identity = 0;

int32_t kinoko_map_type_identity = 0;

int32_t kinoko_string_layout_type_identity = 0;

int32_t kinoko_texture_type_identity = 0;

int32_t kinoko_render_target_type_identity = 0;

int32_t kinoko_mesh_manager_slot = 0;

char kinoko_resource2d_class_published = 0;

int32_t kinoko_acting_player_class_pair[2] = {0, 0};

int32_t kinoko_resource2d_class_pair[2] = {0, 0};

int32_t kinoko_layout_set_pair[2] = {0, 0};

int32_t kinoko_layout_get_pair[2] = {0, 0};

int32_t kinoko_layout_class_pair[2] = {0, 0};

int32_t kinoko_layer_set_pair[2] = {0, 0};

int32_t kinoko_layer_get_pair[2] = {0, 0};

int32_t kinoko_script_void_result_identity;

int32_t retdec_layout_submit_impl(int32_t vertex_buffer,
                                          float32_t x, float32_t y)
{
    return kinoko_quad_submit((KinokoQuad *)(intptr_t)vertex_buffer,x,y);
}

static int32_t kinoko_register_act_script_objects(void* script, void* environment) {
    return retdec_register_act_script((int32_t)(intptr_t)script, (int32_t)(intptr_t)environment);
}

int32_t kinoko_host_register_act_script_abi(int32_t script, int32_t environment) {
    return kinoko_register_act_script_objects((void *)(intptr_t)script, (void *)(intptr_t)environment);
}

static int32_t kinoko_construct_layer_global_vm(void* storage) {
    return retdec_construct_cact_layer((int32_t)(intptr_t)storage, kinoko_act_vm_abi_slot);
}

int32_t kinoko_host_construct_layer_abi(int32_t storage) {
    return kinoko_construct_layer_global_vm((void *)(intptr_t)storage);
}

int32_t __fastcall kinoko_act_layer_associate_method(int32_t receiver, void* unused_edx) {
    int32_t *methods = *(int32_t **)(intptr_t)receiver;
    return retdec_call_thiscall0_result((void *)(intptr_t)receiver,
        (void *)(intptr_t)methods[6]);
}

int32_t *kinoko_c2d_layout_type(void) {
    return &kinoko_layout_type_identity;
}

int32_t *kinoko_chip_resource_type(void) {
    return &kinoko_chip_type_identity;
}

int32_t *kinoko_map_layout_type(void) {
    return &kinoko_map_type_identity;
}

int32_t kinoko_script_dprint_noop(void) { return 0; }

int32_t *kinoko_texture_resource_type(void) {
    return &kinoko_texture_type_identity;
}

int32_t *kinoko_render_target_type(void) {
    return &kinoko_render_target_type_identity;
}

int32_t __fastcall kinoko_color_destroy(int32_t receiver, void* unused_edx, char flags) {
    *(int32_t *)(intptr_t)receiver = (int32_t)(intptr_t)&kinoko_color_methods_storage;
    if (flags & 1) kinoko_host_free_allocation((int32_t *)(intptr_t)receiver);
    return receiver;
}

void retdec_trace_squirrel_name(const char *label, int32_t name_ptr) {
    char message[512];
    if (!kinoko_diagnostics_accepts(label)) return;

    if (name_ptr == 0) {
        wsprintfA(message, "%s:ptr=0x00000000", label);
    } else {
        wsprintfA(message, "%s:ptr=0x%08lX text=%s", label,
                  (unsigned long)(uint32_t)name_ptr,
                  (const char *)(intptr_t)name_ptr);
    }
    retdec_trace(message);
}

uint32_t kinoko_actor_motion_update_mask(void) { return (uint32_t)kinoko_game_masks.update; }

void kinoko_actor_render_set_blend(int32_t mode) { kinoko_render_set_blend(mode); }

int32_t kinoko_actor_render_submit(KinokoAnimationFrame *frame) {
    return kinoko_quad_submit((KinokoQuad *)frame,0.0f,0.0f);
}

const void *kinoko_pat_frame_methods(void) { return &kinoko_sprite_methods_storage; }

int32_t kinoko_host_clear_stages(void) {
    return kinoko_clear_global_stages();
}

int32_t kinoko_host_initialize_camera(void) {
    return kinoko_camera_initialize(reinterpret_cast<KinokoCamera*>(&camera_state.record));
}

int32_t kinoko_camera_class_copy_abi(int32_t a1, int32_t a2) {

    return (int32_t)(intptr_t)kinoko_camera_copy(
        (KinokoCamera *)(intptr_t)a1, (KinokoCamera *)(intptr_t)a2);
}

int32_t kinoko_collision_refresh_abi(int32_t this_ptr) {
    return (int32_t)(intptr_t)kinoko_collision_refresh((KinokoCollisionState *)(intptr_t)this_ptr);
}

int32_t kinoko_collision_reset_abi(int32_t this_ptr, int32_t actor_ptr) {
    return kinoko_collision_reset((KinokoCollisionState *)(intptr_t)this_ptr,
        (KinokoActorManager *)(intptr_t)actor_ptr);
}

int32_t kinoko_host_register_collision_map_abi(int32_t layout) {
    return (int32_t)(intptr_t)kinoko_collision_register_map(
        reinterpret_cast<KinokoCollisionState*>(&collision_state.record), (KinokoActLayout *)(intptr_t)layout);
}

static int32_t kinoko_append_render_item(int32_t *item) {
    static volatile LONG trace_count;
    LONG trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 16) {
        retdec_trace("46a210:entry");
        retdec_trace_i32("46a210:value", item != NULL ? *item : 0);
        retdec_trace_i32("46a210:g613", kinoko_render_queue_identity());
    }
    return item ? kinoko_append_render_queue(*item) : 0;
}

int32_t kinoko_host_append_render_item(int32_t * a1) {
    return kinoko_append_render_item(a1);
}

int32_t kinoko_host_find_map_layout_abi(int32_t name_ptr) {
    return kinoko_map_find_layout((int32_t)(intptr_t)&map_state.record,
                                  (const char *)(intptr_t)name_ptr);
}

int32_t kinoko_host_create_map_layer_abi(int32_t name_ptr)
{
    return kinoko_map_create_render_layer(
        (int32_t)(intptr_t)&map_state.record,
        (const char *)(intptr_t)name_ptr);
}

int32_t kinoko_host_clear_sound(void) {
    return kinoko_clear_global_sound();
}

static int32_t kinoko_install_root_integer_delegate(int32_t a1) {
    int32_t temporary_object[3];
    int32_t binding_object[3];
    int32_t setdelegate_result;

    retdec_trace_i32("470d00:enter-g582", (*kinoko_native_binding_type(0)));
    kinoko_host_get_delegate_abi(a1, binding_object);
    retdec_trace_i32("470d00:after-4aa210-g582", (*kinoko_native_binding_type(0)));
    retdec_trace_i32("470d00:delegate-type", binding_object[1]);
    retdec_trace_i32("470d00:delegate-data", binding_object[2]);
    if (kinoko_sqplus_object_exists(binding_object, "_set") == 0) {
        retdec_trace_i32("470d00:after-4aa1a0-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_object_new_table(temporary_object);
        retdec_trace_i32("470d00:after-4a91c0-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_object_assign(binding_object, temporary_object);
        retdec_trace_i32("470d00:after-4a95c0-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_object_destroy(temporary_object);
        retdec_trace_i32("470d00:after-first-dtor-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_bind_object_function(temporary_object, binding_object, reinterpret_cast<void*>(&kinoko_sqplus_table_set), "_set", "sn|b|s");
        retdec_trace_i32("470d00:after-set-binding-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_object_destroy(temporary_object);
        retdec_trace_i32("470d00:after-second-dtor-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_bind_object_function(temporary_object, binding_object, reinterpret_cast<void*>(&kinoko_sqplus_table_get), "_get", "s");
        retdec_trace_i32("470d00:after-get-binding-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_object_destroy(temporary_object);
        retdec_trace_i32("470d00:after-third-dtor-g582", (*kinoko_native_binding_type(0)));
        setdelegate_result = kinoko_sqplus_object_set_delegate((void *)(uintptr_t)(uint32_t)a1, binding_object);
        retdec_trace_i32("470d00:setdelegate-result", setdelegate_result);
        retdec_trace_i32("470d00:after-4a9f60-g582", (*kinoko_native_binding_type(0)));
    }
    int32_t result = (int32_t)(uintptr_t)kinoko_sqplus_object_destroy(binding_object);
    retdec_trace_i32("470d00:exit-g582", (*kinoko_native_binding_type(0)));
    return result;
}

static int32_t kinoko_script_show_message(const char* text) {
    return MessageBoxA((HWND)kinoko_game_window_slot, text, "Message", 0);
}

int32_t kinoko_host_show_message_abi(int32_t a1) {
    return kinoko_script_show_message((const char *)(intptr_t)a1);
}

static int32_t kinoko_script_sleep(DWORD milliseconds) {
    Sleep(milliseconds);
    return (int32_t)(intptr_t)&kinoko_script_void_result_identity;
}

int32_t kinoko_host_sleep_abi(int32_t dwMilliseconds) {
    return kinoko_script_sleep((DWORD)dwMilliseconds);
}

static int32_t kinoko_script_milliseconds(void) {
    return (int32_t)timeGetTime();
}

int32_t kinoko_host_milliseconds(void) {
    return kinoko_script_milliseconds();
}

static int32_t kinoko_script_close_window(void) {
    return (int32_t)SendMessageA((HWND)kinoko_game_window_slot, WM_CLOSE, 0, 0);
}

int32_t kinoko_host_close_window(void) {
    return kinoko_script_close_window();
}

int32_t retdec_compile_file_native(int32_t vm) {
    int32_t path;
    int32_t result;
    int32_t environment[2] = { retdec_compile_environment_type, retdec_compile_environment_slot };

    if (!kinoko_native_string_arg(kinoko_vm(vm), 2, &path))
        return 0;
    
    if (sq_gettop(kinoko_vm(vm)) > 3 &&
        sq_getstackobj(kinoko_vm(vm), 3, (HSQOBJECT*)(environment)) < 0)
        return -1;
    result = kinoko_script_compile_file_argument(path, (int32_t)(uintptr_t)&retdec_compile_environment_vtable, vm,
                             environment[0], environment[1], 0);
    sq_pushbool(kinoko_vm(vm), ((result) != 0));
    return 1;
}

static int32_t kinoko_bind_root_integer(int32_t *object, int32_t value,
                                        const char *name, int32_t flags) {
    int32_t *slot = (int32_t *)kinoko_sqplus_create_variable(object, name);
    int32_t metadata[5];
    int32_t *type = kinoko_native_binding_type(0);
    retdec_trace_i32("root-integer:g582-before", *type);
    kinoko_sqplus_initialize_variable(metadata, value, 0, 0, type, 4, flags);
    retdec_trace_i32("root-integer:g582-after", *type);
    
    memcpy(slot, metadata, sizeof(metadata));
    return kinoko_install_root_integer_delegate((int32_t)(intptr_t)object);
}

int32_t kinoko_script_bind_root_value(int32_t *object, int32_t *value, char *name, int32_t flags) {
    return kinoko_bind_root_integer(object, (int32_t)(intptr_t)value, name, flags);
}

int32_t kinoko_script_bind_root_integer(int32_t *object, int32_t value, char *name) {
    return kinoko_bind_root_integer(object, value, name, 2);
}

static int32_t retdec_active_vm;
static thread_local int32_t retdec_explicit_vm;

static int32_t retdec_exchange_source_receiver(int32_t vm) {
    int32_t previous = retdec_explicit_vm;
    retdec_explicit_vm = vm;
    return previous;
}

__declspec(noinline) int32_t retdec_stack_vm(void) {
    if (retdec_explicit_vm != 0)
        return retdec_explicit_vm;
    static int32_t null_stack_trace_count;
    
    if (kinoko_primary_vm != NULL) {
        int32_t vm = (int32_t)(intptr_t)kinoko_primary_vm;
        const auto stack = kinoko_sq_stack_snapshot(kinoko_primary_vm);
        const uintptr_t stack_block = reinterpret_cast<uintptr_t>(stack.storage);
        if ((stack_block == 0 || stack_block < 0x02000000u ||
             stack_block >= 0x70000000u) &&
            null_stack_trace_count < 16) {
            retdec_trace_i32("stack-vm-invalid", vm);
            retdec_trace_i32("stack-vm-block", (int32_t)stack_block);
            retdec_trace_i32("stack-vm-top", stack.top);
            retdec_trace_i32("stack-vm-base", stack.base);
            retdec_trace_i32("stack-vm-caller",
                             (int32_t)(uintptr_t)_ReturnAddress());
            ++null_stack_trace_count;
        }
        return vm;
    }
    if (retdec_active_vm != 0)
        return retdec_active_vm;
    return 0;
}

static volatile int32_t retdec_g594_watch_value;

static volatile int32_t retdec_g594_watch_initialized;

static volatile int32_t retdec_g594_watch_busy;

static volatile int32_t retdec_g644_watch_value;

static volatile int32_t retdec_g644_watch_initialized;

static volatile int32_t retdec_g644_watch_busy;

static __declspec(noinline) void retdec_watch_g594(void) {
    int32_t current;
    char message[160];

    if (retdec_g594_watch_busy != 0)
        return;
    retdec_g594_watch_busy = 1;
    current = (*kinoko_native_binding_type(3));
    if (retdec_g594_watch_initialized == 0 ||
        current != retdec_g594_watch_value) {
        wsprintfA(message,
                  "watch:g594=0x%08lX caller=0x%08lX",
                  (unsigned long)current,
                  (unsigned long)(uintptr_t)_ReturnAddress());
        retdec_trace(message);
        retdec_g594_watch_value = current;
        retdec_g594_watch_initialized = 1;
    }
    retdec_g594_watch_busy = 0;
}

static __declspec(noinline) void retdec_watch_g644(void) {
    int32_t current;
    char message[160];

    if (retdec_g644_watch_busy != 0)
        return;
    retdec_g644_watch_busy = 1;
    current = (int32_t)(intptr_t)kinoko_primary_vm;
    if (retdec_g644_watch_initialized == 0 ||
        current != retdec_g644_watch_value) {
        wsprintfA(message,
                  "watch:g644=0x%08lX caller=0x%08lX",
                  (unsigned long)current,
                  (unsigned long)(uintptr_t)_ReturnAddress());
        retdec_trace(message);
        retdec_g644_watch_value = current;
        retdec_g644_watch_initialized = 1;
    }
    retdec_g644_watch_busy = 0;
}

__declspec(noinline) void retdec_trace_i32(const char *label,
                                                  int32_t value) {
    char message[128];
    if (!kinoko_diagnostics_accepts(label)) return;
    retdec_watch_g594();
    retdec_watch_g644();
    wsprintfA(message, "%s:0x%08lX", label, (unsigned long)value);
    retdec_trace(message);
}

static int32_t kinoko_open_primary_script_vm(int32_t stack_size) {
    kinoko_sq_set_context_exchange(retdec_exchange_source_receiver);
    int32_t vm = kinoko_sq_open(stack_size);
    retdec_active_vm = vm;
    retdec_primary_shared_state = kinoko_sq_shared_state(vm);
    return vm;
}

const KinokoSqplusVmSlots *kinoko_sqplus_vm_slots(void) {
    static KinokoSqplusVmSlots slots = { &kinoko_skip_vm_owner_reset, &kinoko_newest_shared_state, &kinoko_primary_vm, &kinoko_cached_root_slot, kinoko_vm_thread_wrapper };
    return &slots;
}

struct SQVM *kinoko_script_open_primary_vm(int32_t stack_size) {
    return (struct SQVM *)(intptr_t)kinoko_open_primary_script_vm(stack_size);
}

int32_t kinoko_host_open_vm_abi(int32_t a1) {
    return (int32_t)(intptr_t)kinoko_script_open_primary_vm(a1);
}

int32_t kinoko_sqrat_object_vtable(void) { return (int32_t)(intptr_t)&kinoko_sqrat_object_methods_storage; }

int32_t kinoko_sqrat_root_vtable(void) { return (int32_t)(intptr_t)&kinoko_sqrat_root_methods_storage; }

int32_t kinoko_actor_vtable(void) { return (int32_t)(intptr_t)&kinoko_actor_methods_storage; }

int32_t kinoko_actor_step_key(void) { return (int32_t)(intptr_t)&kinoko_actor_step_key_storage; }

int32_t kinoko_squirrel_object_vtable(void) {
    return (int32_t)(intptr_t)&kinoko_squirrel_object_methods_storage;
}

int32_t kinoko_host_get_delegate_abi(int32_t source_ptr, int32_t *target_ptr) {
    return (int32_t)(intptr_t)kinoko_sqplus_object_get_delegate(
        (void *)(intptr_t)source_ptr, target_ptr);
}

int32_t *kinoko_native_binding_type(int32_t category) {
    return category == -1 ? kinoko_sqplus_game_type(0, kinoko_actor_assign_instance)
                          : kinoko_sqplus_scalar_type(category);
}

int32_t kinoko_native_void_type(void) {
    return (int32_t)(intptr_t)kinoko_sqplus_scalar_type(-1);
}

int32_t kinoko_host_create_native_instance_abi(int32_t vm, int32_t class_name,
                        int32_t native_pointer, int32_t release_hook) {
    kinoko_sqplus_select_vm((struct SQVM *)(intptr_t)(vm));
    return kinoko_native_instance_create(vm, class_name, native_pointer,
        release_hook, (int32_t)(intptr_t)&kinoko_squirrel_object_methods_storage);
}

void kinoko_host_free_allocation(int32_t * a1) {
    free(a1);
}

int32_t kinoko_host_destroy_global_callback(void) {
    return kinoko_destroy_script_callback(&global_callback);
}

int32_t kinoko_csv_load_bytes(const char *path, char **bytes) {
    KinokoArchiveReader *reader = NULL;
    *bytes = NULL;
    if (!kinoko_reader_open(&reader, path)) return 0;
    uint32_t size = kinoko_reader_size(reader);
    char *buffer = size < UINT32_MAX ? (char *)malloc((size_t)size + 1) : NULL;
    int32_t ok = buffer != NULL && (size == 0 || kinoko_reader_read_exact(reader, buffer, size));
    kinoko_reader_close(reader);
    if (!ok) { free(buffer); return 0; }
    if (kinoko_script_assets_packed()) {
        unsigned char key = 0x8b, step = 0x71;
        for (uint32_t i = 0; i < size; ++i) {
            buffer[i] ^= key;
            key = (unsigned char)(key + step);
            step = (unsigned char)(step - 0x6b);
        }
    }
    buffer[size] = 0;
    *bytes = buffer;
    return 1;
}

int32_t kinoko_act_script_output_compiled(void) { return kinoko_compile_act_output != 0; }

const struct KinokoActHostSymbols* kinoko_act_host_symbols(void)
{
    static const struct KinokoActHostSymbols symbols = {
        &kinoko_act_script_methods_storage, 
        &kinoko_act_layer_reference_methods_storage, 
        &kinoko_act_layer_methods_storage, 
        &kinoko_act_layer_layout_methods_storage, 
        &kinoko_act_key_methods_storage, 
        &kinoko_act_document_methods_storage, 
        &kinoko_act_layout_methods_storage, 
        &kinoko_layout_color_methods_storage, 
        &kinoko_chip_resource_methods_storage, 
        &kinoko_map_layout_methods_storage, 
        &kinoko_map_color_methods_storage, 
        &kinoko_texture_resource_methods_storage, 
        &kinoko_sqrat_object_methods_storage, 
        &kinoko_sqrat_root_methods_storage, 
        &kinoko_sprite_methods_storage, 
        &kinoko_color_methods_storage, 
        &kinoko_render_target_methods_storage, 
        &kinoko_chip_quad_methods_storage, 
    };
    return &symbols;
}

const struct KinokoAudioHostSymbols* kinoko_audio_host_symbols(void) {
    static struct KinokoAudioHostSymbols symbols;
    symbols.critical_section_vtable = &kinoko_critical_section_methods;
    symbols.device_error_message = kinoko_audio_error_text;
    return &symbols;
}

static int32_t kinoko_input_copy_abi(int32_t destination, int32_t source) {
    return (int32_t)(intptr_t)kinoko_input_manager_assign((KinokoInputManager*)(intptr_t)destination,
        (const KinokoInputManager*)(intptr_t)source);
}

const KinokoCameraMapScriptSymbols *kinoko_camera_map_script_symbols(void) {
    static KinokoCameraMapScriptSymbols symbols;
    symbols.camera_class = kinoko_camera_class_storage;
    symbols.map_class = kinoko_map_class_storage;
    return &symbols;
}

const KinokoInputScriptSymbols *kinoko_input_script_symbols(void) {
    static KinokoInputScriptSymbols symbols;
    symbols.object_vtable = &kinoko_squirrel_object_methods_storage;
    symbols.null_type = kinoko_null_object_type;
    symbols.null_data = kinoko_null_object_value;
    symbols.input_class = kinoko_input_class_storage;
    symbols.root = kinoko_script_root_storage;
    symbols.vm = kinoko_primary_vm;
    symbols.manager_storage_bytes = sizeof(input_state);
    return &symbols;
}

int32_t *kinoko_input_binding_type(void) {
    return kinoko_sqplus_game_type(2, kinoko_input_copy_abi);
}

int32_t *kinoko_camera_binding_type(void) {
    return kinoko_sqplus_game_type(1, kinoko_camera_class_copy_abi);
}

static int32_t kinoko_map_copy_abi(int32_t destination, int32_t source) {
    return (int32_t)(intptr_t)kinoko_map_manager_assign(
        (KinokoMapManager*)(intptr_t)destination, (KinokoMapManager*)(intptr_t)source);
}

int32_t *kinoko_map_binding_type(void) {
    return kinoko_sqplus_game_type(3, kinoko_map_copy_abi);
}

const void *kinoko_actor_owner_methods(void) { return &kinoko_actor_owner_methods_storage; }

const void *kinoko_actor_render_layer_methods(void) { return &kinoko_actor_render_layer_methods_storage; }

void *kinoko_actor_class_object(void) { return kinoko_actor_class_storage; }

void *kinoko_actor_user_key(void) { return kinoko_actor_user_key_storage; }

struct SQVM *kinoko_actor_default_vm(void) { return (struct SQVM *)kinoko_primary_vm; }

void kinoko_actor_motion_host(KinokoActor *actor) { kinoko_actor_update_motion(kinoko_game_collision_state(), actor); }

int32_t kinoko_actor_render_host(KinokoActor *actor, KinokoCamera *camera) {
    return kinoko_actor_render(actor, camera);
}

void kinoko_actor_manager_refresh_collision(void) { kinoko_script_refresh_collision(); }

const KinokoRuntimeBootSymbols *kinoko_runtime_boot_symbols(void) {
    static KinokoRuntimeBootSymbols symbols;
    symbols.map = reinterpret_cast<KinokoMapManager*>(&map_state.record);
    symbols.actors = reinterpret_cast<KinokoActorManager*>(&actor_state.record);
    symbols.renderer_methods = &kinoko_renderer_methods_storage;
    symbols.render_layer_owner_slot = &kinoko_render_layer_owner_slot;
    return &symbols;
}

void kinoko_application_initialize_host(void) {
    kinoko_runtime_initialize_objects(kinoko_runtime_boot_symbols());
}

const char *kinoko_application_title(void) { return kinoko_application_title_text; }

const char *kinoko_application_error(void) { return kinoko_application_error_text; }

void kinoko_application_set_archive_mode(int32_t enabled) { kinoko_packed_assets = enabled != 0; }

void kinoko_application_open_archives(void) {
    kinoko_archive_mount("6kinoko_a.dat");
    kinoko_archive_mount("6kinoko_b.dat");
    kinoko_archive_mount("6kinoko_c.dat");
    kinoko_string_assign_n(kinoko_act_script_extension, ".cv4", 4);
}

const KinokoGameObjects *kinoko_game_objects(void) {
    static const KinokoGameObjects objects = {
        reinterpret_cast<KinokoInputManager*>(&input_state.record),
        reinterpret_cast<KinokoActorManager*>(&actor_state.record),
        reinterpret_cast<KinokoCamera*>(&camera_state.record),
        reinterpret_cast<KinokoMapManager*>(&map_state.record)
    };
    return &objects;
}

static struct SQVM *game_startup_vm;

void kinoko_game_prepare_scripts(void) {
    if (!game_startup_vm) game_startup_vm = (struct SQVM *)kinoko_primary_vm;
    retdec_trace_i32("469640:vm-before", (int32_t)(intptr_t)kinoko_primary_vm);
}

void kinoko_game_register_scripts(void) {
    retdec_trace("469640:sqrat-begin");
    retdec_trace("469640:before-473010");
    kinoko_register_root_bindings_entry();
    retdec_trace("469640:after-473010");
    if (kinoko_primary_vm) game_startup_vm = (struct SQVM *)kinoko_primary_vm;
    else if (game_startup_vm) kinoko_primary_vm = (struct SQVM *)game_startup_vm;
    retdec_trace_i32("469640:vm-after-sqrat", (int32_t)(intptr_t)kinoko_primary_vm);
    retdec_trace("469640:sqrat-done");
}

int32_t kinoko_game_initialize_input(KinokoInputManager *input) {
    return kinoko_input_initialize_script_instance(input);
}

int32_t kinoko_game_update_input(KinokoInputManager *input) {
    return kinoko_input_manager_update(input);
}

int32_t kinoko_game_load_boot_script(void) { return kinoko_script_load_file("data/script/boot.nut", 0); }

void kinoko_game_update_callback(int32_t trace_index) {
    if (kinoko_sqplus_object_type(&global_callback.closure) ==
        0x08000100) {

        retdec_trace("stagevm:global-callback");
        if (trace_index <= 16) {
            retdec_trace_i32("stagevm:global-state-vm", (int32_t)(intptr_t)global_callback.vm);
            retdec_trace_i32("stagevm:global-env-type", global_callback.environment.type);
            retdec_trace_i32("stagevm:global-env-data", global_callback.environment.value);
            retdec_trace_i32("stagevm:global-func-type", global_callback.closure.type);
            retdec_trace_i32("stagevm:global-func-data", global_callback.closure.value);
        }
        if (kinoko_script_callback_invoke(&global_callback) < 0)
            kinoko_script_callback_clear(&global_callback);
    }
}

int32_t kinoko_game_update_map(KinokoMapManager *map) {
    return kinoko_map_manager_update(map);
}

void kinoko_game_prepare_map(KinokoMapManager *map, KinokoCamera *camera) {
    kinoko_map_manager_prepare(map, camera);
}

void kinoko_game_clear_map(KinokoMapManager *map) {
    kinoko_map_manager_clear(map);
}

void kinoko_game_clear_actors(KinokoActorManager *actors) {
    kinoko_actor_manager_clear_resources(actors);
}

void kinoko_game_trace_map(KinokoMapManager *map, int32_t drawing) {
    const auto state = kinoko::map::ManagerView(map).load();
    if (drawing) retdec_trace_i32("render:map-layer", (int32_t)(intptr_t)state.source_act);
    else {
        retdec_trace_i32("469900:map-act", (int32_t)(intptr_t)state.source_act);
        retdec_trace_i32("469900:map-resource", (int32_t)(intptr_t)state.player);
    }
}

void kinoko_game_release_script_reference(uint32_t index) {
    int32_t *references[] = { kinoko_actor_class_storage, kinoko_input_class_storage, kinoko_camera_class_storage, kinoko_map_class_storage };
    if (index < 4) kinoko_sqplus_object_reset(references[index]);
}

int32_t kinoko_game_close_vm(void) { return kinoko_script_close_vm(); }

KinokoScriptCallback *kinoko_game_global_callback(void) { return &global_callback; }

KinokoCollisionState *kinoko_game_collision_state(void) { return reinterpret_cast<KinokoCollisionState*>(&collision_state.record); }

void kinoko_game_initialize_callback(KinokoScriptCallback *callback) {
    kinoko_script_callback_construct(callback, nullptr);
}

int32_t kinoko_game_load_map_file(const char *path) { return kinoko_map_manager_load(reinterpret_cast<KinokoMapManager*>(&map_state.record), path, (struct SQVM *)kinoko_primary_vm, kinoko_map_class_storage, &kinoko_script_root_storage); }

int32_t kinoko_game_release_map_state(void) { kinoko_map_manager_clear(reinterpret_cast<KinokoMapManager*>(&map_state.record)); return 0; }

void kinoko_game_split_path(const char *path, char *directory) {
    kinoko_path_split(path, directory, NULL);
}

int32_t kinoko_host_explicit_vm(void) { return retdec_explicit_vm; }

