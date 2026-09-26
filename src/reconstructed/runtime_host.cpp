// Windows x86 runtime host. Original evidence: src/decompiled/6kinoko.exe.c.
#include "runtime_host_internal.h"

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

void retdec_mcd_free(struct retdec_mcd_data *data);

int32_t retdec_publish_act_layers(int32_t vm, int32_t act,
                                          int32_t resource_ptr,
                                          int32_t *active_count);

int32_t retdec_load_act_texture(const char *texture_name);

void retdec_trace(const char *message);



int32_t function_4aa210(int32_t source_ptr, int32_t *target_ptr);

static int32_t (*resolve_root_binding)(int32_t, int32_t *) = function_4aa210;

int32_t (__fastcall *g23)(int32_t, void*, char) = kinoko_color_destroy;

int32_t g25 = 0x44fd30;

int32_t g28 = (int32_t)(intptr_t)&kinoko_method_actor_pool_base_delete;

const char * g42 = "\x8f\x89\x8a\xfa\x89\xbb\x8e\xb8\x94s";

const char * g43 = "\x96\x82\x97\x9d\x8d\xb9\x82\xc6\x82U\x82\xc2\x82\xcc\x83L\x83m\x83R";

const char * g209 = "DirectSound\x83I\x83u\x83W\x83\x46\x83N\x83g\x82\xcc\x8d\xec\x90\xac\x82\xc9\x8e\xb8\x94s";

struct vtable_4ec7cc_type g300 = {
    kinoko_delete_layout_sprite,
    kinoko_quad_set_color,
    kinoko_quad_set_vertex_colors,
    kinoko_quad_modulate_color
};

int32_t g350[11] = { 
    (int32_t)kinoko_method_write_string_layout,
    (int32_t)kinoko_method_read_string_layout,
    (int32_t)kinoko_method_query_serializable,
    (int32_t)kinoko_method_destroy_string_layout,
    (int32_t)kinoko_method_string_layout_type,
    (int32_t)kinoko_method_clone_string_layout,
    (int32_t)kinoko_method_set_string_layer,
    (int32_t)kinoko_method_update_string_layout,
    (int32_t)kinoko_method_draw_string_layout,
    (int32_t)kinoko_method_register_string_layout,
    (int32_t)kinoko_method_delete_string_layout
};

int32_t g483 = 0x01000001;

int32_t g484 = 0;

int32_t g534 = 0;

int32_t g535 = 0;

int32_t g545 = 1023;

int32_t g546 = 0;

char g547 = 0;

char g548 = 0;

char g549 = 0;

int32_t g550 = 0;

int32_t kinoko_act_script_extension[7] = {0, 0, 0, 0, 0, 15, 0};

char g560 = 1;

int32_t g600[3] = { 0, 0, 0 };

int32_t g601[3] = { 0, 0, 0 };

int32_t g602[3] = { 0, 0, 0 };

int32_t g603 = 0;

int32_t g604 = 0;

int32_t g611[3] = { 0, 0, 0 };

int32_t retdec_game_callback_storage[7] = { 0 };

int32_t g_514300_storage[30] = { 0 };

__declspec(align(8)) unsigned char g_retdec_actor_manager_state[0x200] = { 0 };

__declspec(align(8)) unsigned char g_retdec_map_manager_state[0x200] = { 0 };

__declspec(align(8)) unsigned char g_retdec_camera_state[0x200] = { 0 };

int32_t g617 = 0;

int32_t g629[3] = { 0, 0, 0 };

int32_t g636[3] = { 0, 0, 0 };

int32_t g637 = 0;

KinokoIntegerMap* g638 = NULL;

int32_t g639 = 0;

char g642 = 0;

int32_t g643 = 0;

#pragma data_seg(".g644")
__declspec(align(4096)) char * g644 = NULL;
#pragma data_seg()

int32_t g645 = 0;

int32_t unk_5149EC[3] = { 0, 0, 0 };

int32_t g664 = 0;

char g673 = 0;

KinokoRenderer kinoko_renderer = {0};

KinokoGraphics kinoko_graphics = {0};

KinokoCriticalSection kinoko_graphics_lock = { 0 };

int32_t g722[3] = { 0, 0, 0 };

char * g767;

unsigned char g_retdec_keyboard_state[256];

char g874 = 0;

int32_t g876 = 0;

char * g877;

int32_t g878 = 0;

int32_t g910 = 0;

int32_t g914 = 0;

int32_t g918 = 0;

int32_t g926 = 0;

int32_t g930 = 0;

int32_t g934 = 0;

int32_t kinoko_mesh_manager_slot = 0;

char kinoko_resource2d_class_published = 0;

int32_t kinoko_acting_player_class_pair[2] = {0, 0};

int32_t kinoko_resource2d_class_pair[2] = {0, 0};

int32_t kinoko_layout_set_pair[2] = {0, 0};

int32_t kinoko_layout_get_pair[2] = {0, 0};

int32_t kinoko_layout_class_pair[2] = {0, 0};

int32_t kinoko_layer_set_pair[2] = {0, 0};

int32_t kinoko_layer_get_pair[2] = {0, 0};

int32_t g1224;

struct vtable_4d54a4_type g16 = {
    (int32_t (*)(char))kinoko_squirrel_object_delete
};

struct vtable_4d54ac_type g17 = {
    (int32_t (*)(char))kinoko_actor_delete_method
};

struct vtable_4d59bc_type g27 = {
    (int32_t (*)(int32_t))kinoko_method_render_layer_update
};

struct vtable_4d5a04_type g29 = {
    kinoko_method_actor_pool_delete,
    (int32_t (*)(int32_t))kinoko_method_actor_manager_top,
    (int32_t (*)(uint32_t))kinoko_method_actor_manager_remove,
    kinoko_method_lookup_actor,
    kinoko_method_actor_pool_count
};

struct vtable_4d5a5c_type g31 = {
    kinoko_method_actor_owner_delete,
    (int32_t (*)(void))kinoko_method_actor_manager_push
};

struct vtable_4d5ba0_type g37 = {
    kinoko_map_render_layer_entry
};

struct vtable_4d5c68_type g39 = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct vtable_4d5c80_type g40 = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct vtable_4eb20c_type g184 = {
    kinoko_renderer_before_reset,
    kinoko_renderer_after_reset
};

struct vtable_4ebacc_type g231 = {
    (int32_t (*)(int32_t))kinoko_method_write_act_script,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_act_script,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_delete_act_script
};

struct vtable_4ebe58_type g251 = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct vtable_4ebe68_type g252 = {
    (int32_t (*)(int32_t))kinoko_method_write_act_layer,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_act_layer,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_serializable,
    (int32_t (*)(unsigned char))kinoko_method_delete_act_layer,
    (int32_t (*)(void))kinoko_method_clone_act_layer,
    (int32_t (*)(int32_t))kinoko_act_layer_set_resource,
    (int32_t (*)(int32_t, int32_t, int32_t))kinoko_act_layer_world_position,
    (int32_t (*)(int32_t))kinoko_method_register_act_layer
};

struct vtable_4ebe90_type g253 = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct vtable_4ec0b0_type g277 = {
    (int32_t (*)(int32_t))kinoko_method_write_act_key,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_act_key,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_serializable,
    (int32_t (*)(char))kinoko_method_delete_act_key,
    (int32_t (*)(void))kinoko_method_clone_act_key
};

struct vtable_4ec1d4_type g285 = {
    (int32_t (*)(int32_t))kinoko_method_write_act,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_act,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_serializable,
    (int32_t (*)(unsigned char))kinoko_method_destroy_act,
    kinoko_act_clone,
    (int32_t (*)(int32_t))kinoko_method_load_act_resources,
    (int32_t (*)(void))kinoko_method_suspend_act_resources,
    (int32_t (*)(void))kinoko_method_resume_act_resources
};

struct vtable_4ec358_type g299 = {
    (int32_t (*)(int32_t))kinoko_method_write_layout_properties,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_layout_properties,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_layout,
    (int32_t (*)(void))kinoko_c2d_layout_type,
    (int32_t (*)(void))kinoko_method_clone_c2d_layout,
    (int32_t (*)(int32_t))kinoko_method_layout_set_layer,
    (int32_t (*)(void))kinoko_method_layout_update,
    (int32_t (*)(float, float))kinoko_method_layout_draw,
    (int32_t (*)(void))kinoko_method_register_layout
};

struct vtable_4ec548_type g313 = {
    (int32_t (*)(int32_t))kinoko_method_write_chip_resource,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_chip_resource,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_serializable,
    (int32_t (*)(unsigned char))kinoko_method_delete_act_resource,
    (int32_t (*)(void))kinoko_chip_resource_type,
    (int32_t (*)(int32_t))kinoko_method_register_chip_resource,
    (int32_t (*)(int32_t, int32_t))kinoko_method_resource_42f800,
    (int32_t (*)(int32_t, int32_t))kinoko_method_resource_42f6c0,
    (int32_t (*)(void))kinoko_method_clone_chip_resource,
    (int32_t (*)(int32_t))kinoko_method_load_chip_resource
};

struct vtable_4ec79c_type g327 = {
    (int32_t (*)(int32_t))kinoko_method_write_map_layout,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_map_layout,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_layout,
    (int32_t (*)(void))kinoko_map_layout_type,
    kinoko_clone_map_layout,
    (int32_t (*)(int32_t))kinoko_method_map_set_layer,
    kinoko_map_update_all_entry,
    kinoko_map_draw_entry,
    (int32_t (*)(void))kinoko_method_register_map_layout,
    kinoko_map_update_visible_entry
};

struct vtable_4ec7cc_type g328 = {
    kinoko_delete_map_sprite,
    kinoko_quad_set_color,
    kinoko_quad_set_vertex_colors,
    kinoko_quad_modulate_color
};

struct vtable_4eccfc_type g365 = {
    (int32_t (*)(int32_t))kinoko_method_write_texture_resource,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_texture_resource,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_serializable,
    (int32_t (*)(unsigned char))kinoko_method_delete_act_resource,
    (int32_t (*)(void))kinoko_texture_resource_type,
    (int32_t (*)(int32_t))kinoko_method_register_texture_resource,
    (int32_t (*)(int32_t, int32_t))kinoko_method_resource_446920,
    (int32_t (*)(int32_t, int32_t))kinoko_method_resource_4467e0,
    (int32_t (*)(void))kinoko_method_clone_texture_resource,
    (int32_t (*)(int32_t))kinoko_method_load_resource_texture,
    (int32_t (*)(void))kinoko_method_unload_resource_texture
};

struct vtable_4ece50_type g379 = {
    (int32_t (*)(int32_t))kinoko_method_write_render_target,
    (int32_t (*)(int32_t, int32_t))kinoko_method_read_render_target,
    (int32_t (*)(int32_t, int32_t))kinoko_method_query_serializable,
    (int32_t (*)(void))kinoko_method_destroy_serializable,
    (int32_t (*)(unsigned char))kinoko_method_delete_act_resource,
    (int32_t (*)(void))kinoko_render_target_type,
    (int32_t (*)(int32_t))kinoko_method_register_render_target,
    (int32_t (*)(int32_t, int32_t))kinoko_method_resource_4499a0,
    (int32_t (*)(int32_t, int32_t))kinoko_method_resource_449860,
    (int32_t (*)(void))kinoko_method_clone_render_target,
    (int32_t (*)(int32_t))kinoko_method_load_resource_texture,
    (int32_t (*)(void))kinoko_method_unload_resource_texture,
    (int32_t (*)(int32_t, int32_t))kinoko_method_create_render_target
};

struct vtable_4ed2cc_type g407 = {
    (int32_t (*)(char))kinoko_color_destroy,
    kinoko_quad_set_color,
    kinoko_quad_set_vertex_colors,
    kinoko_quad_modulate_color,
    kinoko_sprite_set_rect_pivot,
    kinoko_sprite_set_rect,
    kinoko_sprite_draw_bounds,
    kinoko_sprite_draw_404770,
    kinoko_sprite_draw_404bc0,
    kinoko_sprite_draw_4049c0
};

int32_t retdec_layout_submit_impl(int32_t vertex_buffer,
                                          float32_t x, float32_t y)
{
    return kinoko_quad_submit((KinokoQuad *)(intptr_t)vertex_buffer,x,y);
}

static int32_t kinoko_register_act_script_objects(void* script, void* environment) {
    return retdec_register_act_script((int32_t)(intptr_t)script, (int32_t)(intptr_t)environment);
}

int32_t function_415fd0(int32_t script, int32_t environment) {
    return kinoko_register_act_script_objects((void *)(intptr_t)script, (void *)(intptr_t)environment);
}

static int32_t kinoko_construct_layer_global_vm(void* storage) {
    return retdec_construct_cact_layer((int32_t)(intptr_t)storage, g664);
}

int32_t function_41e390(int32_t storage) {
    return kinoko_construct_layer_global_vm((void *)(intptr_t)storage);
}

int32_t __fastcall kinoko_act_layer_associate_method(int32_t receiver, void* unused_edx) {
    int32_t *methods = *(int32_t **)(intptr_t)receiver;
    return retdec_call_thiscall0_result((void *)(intptr_t)receiver,
        (void *)(intptr_t)methods[6]);
}

int32_t *kinoko_c2d_layout_type(void) {
    return &g910;
}

int32_t *kinoko_chip_resource_type(void) {
    return &g914;
}

int32_t *kinoko_map_layout_type(void) {
    return &g918;
}

int32_t kinoko_script_dprint_noop(void) { return 0; }

int32_t *kinoko_texture_resource_type(void) {
    return &g930;
}

int32_t *kinoko_render_target_type(void) {
    return &g934;
}

int32_t __fastcall kinoko_color_destroy(int32_t receiver, void* unused_edx, char flags) {
    *(int32_t *)(intptr_t)receiver = (int32_t)(intptr_t)&g23;
    if (flags & 1) _3f__3f_3_40_YAXPAX_40_Z((int32_t *)(intptr_t)receiver);
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

const void *kinoko_pat_frame_methods(void) { return &g407; }

int32_t function_465f70(void) {
    return kinoko_clear_global_stages();
}

int32_t function_466270(void) {
    return kinoko_camera_initialize((KinokoCamera *)g_retdec_camera_state);
}

int32_t function_466540(int32_t a1, int32_t a2) {

    return (int32_t)(intptr_t)kinoko_camera_copy(
        (KinokoCamera *)(intptr_t)a1, (KinokoCamera *)(intptr_t)a2);
}

int32_t function_468620_this(int32_t this_ptr) {
    return (int32_t)(intptr_t)kinoko_collision_refresh((KinokoCollisionState *)(intptr_t)this_ptr);
}

int32_t function_468950_this(int32_t this_ptr, int32_t actor_ptr) {
    return kinoko_collision_reset((KinokoCollisionState *)(intptr_t)this_ptr,
        (KinokoActorManager *)(intptr_t)actor_ptr);
}

int32_t function_4693a0(int32_t layout) {
    return (int32_t)(intptr_t)kinoko_collision_register_map(
        (KinokoCollisionState *)g_514300_storage, (KinokoActLayout *)(intptr_t)layout);
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

int32_t function_46a210(int32_t * a1) {
    return kinoko_append_render_item(a1);
}

int32_t function_46f140(int32_t name_ptr) {
    return kinoko_map_find_layout((int32_t)(intptr_t)g_retdec_map_manager_state,
                                  (const char *)(intptr_t)name_ptr);
}

int32_t function_470030(int32_t name_ptr)
{
    return kinoko_map_create_render_layer(
        (int32_t)(intptr_t)g_retdec_map_manager_state,
        (const char *)(intptr_t)name_ptr);
}

int32_t function_470890(void) {
    return kinoko_clear_global_sound();
}

static int32_t kinoko_install_root_integer_delegate(int32_t a1) {
    int32_t temporary_object[3];
    int32_t binding_object[3];
    int32_t setdelegate_result;

    retdec_trace_i32("470d00:enter-g582", (*kinoko_native_binding_type(0)));
    resolve_root_binding(a1, binding_object);
    retdec_trace_i32("470d00:after-4aa210-g582", (*kinoko_native_binding_type(0)));
    retdec_trace_i32("470d00:delegate-type", binding_object[1]);
    retdec_trace_i32("470d00:delegate-data", binding_object[2]);
    if (kinoko_sqplus_object_exists((void *)(uintptr_t)(uint32_t)(uintptr_t)binding_object, "_set") == 0) {
        retdec_trace_i32("470d00:after-4aa1a0-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_object_new_table((void *)(uintptr_t)(uint32_t)(uintptr_t)temporary_object);
        retdec_trace_i32("470d00:after-4a91c0-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_object_assign((void *)(uintptr_t)(uint32_t)(uintptr_t)binding_object, (const void *)(uintptr_t)(uint32_t)(uintptr_t)temporary_object);
        retdec_trace_i32("470d00:after-4a95c0-g582", (*kinoko_native_binding_type(0)));
        (int32_t)(uintptr_t)kinoko_sqplus_object_destroy((void *)(uintptr_t)(uint32_t)(uintptr_t)temporary_object);
        retdec_trace_i32("470d00:after-first-dtor-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_bind_object_function(temporary_object, (void *)(uintptr_t)(uint32_t)(uintptr_t)binding_object, (void *)(uintptr_t)(uint32_t)(uintptr_t)&kinoko_sqplus_table_set, "_set", "sn|b|s");
        retdec_trace_i32("470d00:after-set-binding-g582", (*kinoko_native_binding_type(0)));
        (int32_t)(uintptr_t)kinoko_sqplus_object_destroy((void *)(uintptr_t)(uint32_t)(uintptr_t)temporary_object);
        retdec_trace_i32("470d00:after-second-dtor-g582", (*kinoko_native_binding_type(0)));
        kinoko_sqplus_bind_object_function(temporary_object, (void *)(uintptr_t)(uint32_t)(uintptr_t)binding_object, (void *)(uintptr_t)(uint32_t)(uintptr_t)&kinoko_sqplus_table_get, "_get", "s");
        retdec_trace_i32("470d00:after-get-binding-g582", (*kinoko_native_binding_type(0)));
        (int32_t)(uintptr_t)kinoko_sqplus_object_destroy((void *)(uintptr_t)(uint32_t)(uintptr_t)temporary_object);
        retdec_trace_i32("470d00:after-third-dtor-g582", (*kinoko_native_binding_type(0)));
        setdelegate_result = kinoko_sqplus_object_set_delegate((void *)(uintptr_t)(uint32_t)a1, (const void *)(uintptr_t)(uint32_t)(uintptr_t)binding_object);
        retdec_trace_i32("470d00:setdelegate-result", setdelegate_result);
        retdec_trace_i32("470d00:after-4a9f60-g582", (*kinoko_native_binding_type(0)));
    }
    int32_t result = (int32_t)(uintptr_t)kinoko_sqplus_object_destroy((void *)(uintptr_t)(uint32_t)(uintptr_t)binding_object);
    retdec_trace_i32("470d00:exit-g582", (*kinoko_native_binding_type(0)));
    return result;
}

static int32_t kinoko_script_show_message(const char* text) {
    return MessageBoxA((HWND)g767, text, "Message", 0);
}

int32_t function_470f60(int32_t a1) {
    return kinoko_script_show_message((const char *)(intptr_t)a1);
}

static int32_t kinoko_script_sleep(DWORD milliseconds) {
    Sleep(milliseconds);
    return (int32_t)(intptr_t)&g1224;
}

int32_t function_470f80(int32_t dwMilliseconds) {
    return kinoko_script_sleep((DWORD)dwMilliseconds);
}

static int32_t kinoko_script_milliseconds(void) {
    return (int32_t)timeGetTime();
}

int32_t function_470f90(void) {
    return kinoko_script_milliseconds();
}

static int32_t kinoko_script_close_window(void) {
    return (int32_t)SendMessageA((HWND)g767, WM_CLOSE, 0, 0);
}

int32_t function_471080(void) {
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
    
    if (g644 != NULL) {
        int32_t vm = (int32_t)(intptr_t)g644;
        uint32_t stack_block =
            (uint32_t)*(int32_t *)(intptr_t)(vm + 24);
        if ((stack_block == 0 || stack_block < 0x02000000u ||
             stack_block >= 0x70000000u) &&
            null_stack_trace_count < 16) {
            retdec_trace_i32("stack-vm-invalid", vm);
            retdec_trace_i32("stack-vm-block", (int32_t)stack_block);
            retdec_trace_i32("stack-vm-top", *(int32_t *)(vm + 48));
            retdec_trace_i32("stack-vm-base", *(int32_t *)(vm + 52));
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
    current = (int32_t)(intptr_t)g644;
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
    static KinokoSqplusVmSlots slots = { &g642, &g643, &g644, &g645, unk_5149EC };
    return &slots;
}

struct SQVM *kinoko_script_open_primary_vm(int32_t stack_size) {
    return (struct SQVM *)(intptr_t)kinoko_open_primary_script_vm(stack_size);
}

int32_t function_48a170(int32_t a1) {
    return (int32_t)(intptr_t)kinoko_script_open_primary_vm(a1);
}

int32_t kinoko_sqrat_object_vtable(void) { return (int32_t)(intptr_t)&g39; }

int32_t kinoko_sqrat_root_vtable(void) { return (int32_t)(intptr_t)&g40; }

int32_t kinoko_actor_vtable(void) { return (int32_t)(intptr_t)&g17; }

int32_t kinoko_actor_step_key(void) { return (int32_t)(intptr_t)&g601; }

int32_t kinoko_squirrel_object_vtable(void) {
    return (int32_t)(intptr_t)&g16;
}

int32_t function_4aa210(int32_t source_ptr, int32_t *target_ptr) {
    return (int32_t)(intptr_t)(int32_t*)(intptr_t)(kinoko_sqplus_object_get_delegate((void *)(intptr_t)(source_ptr), (void *)(intptr_t)((int32_t)(intptr_t)target_ptr)));
}

int32_t *kinoko_native_binding_type(int32_t category) {
    return category == -1 ? kinoko_sqplus_game_type(0, kinoko_actor_assign_instance)
                          : kinoko_sqplus_scalar_type(category);
}

int32_t kinoko_native_void_type(void) {
    return (int32_t)(intptr_t)kinoko_sqplus_scalar_type(-1);
}

int32_t function_4ab170(int32_t vm, int32_t class_name,
                        int32_t native_pointer, int32_t release_hook) {
    kinoko_sqplus_select_vm((struct SQVM *)(intptr_t)(vm));
    return kinoko_native_instance_create(vm, class_name, native_pointer,
        release_hook, (int32_t)(intptr_t)&g16);
}

void _3f__3f_3_40_YAXPAX_40_Z(int32_t * a1) {
    free(a1);
}

int32_t function_4d4860(void) {
    return kinoko_destroy_script_callback((KinokoScriptCallback *)(intptr_t)((int32_t)(intptr_t)g612));
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

int32_t kinoko_act_script_output_compiled(void) { return g673 != 0; }

const struct KinokoActHostSymbols* kinoko_act_host_symbols(void)
{
    static const struct KinokoActHostSymbols symbols = {
        &g231, 
        &g251, 
        &g252, 
        &g253, 
        &g277, 
        &g285, 
        &g299, 
        &g300, 
        &g313, 
        &g327, 
        &g328, 
        &g365, 
        &g39, 
        &g40, 
        &g407, 
        &g23, 
        &g379, 
        &g25, 
    };
    return &symbols;
}

const struct KinokoAudioHostSymbols* kinoko_audio_host_symbols(void) {
    static struct KinokoAudioHostSymbols symbols;
    symbols.critical_section_vtable = &kinoko_critical_section_methods;
    symbols.device_error_message = g209;
    return &symbols;
}

static int32_t kinoko_input_copy_abi(int32_t destination, int32_t source) {
    return (int32_t)(intptr_t)kinoko_input_manager_assign((KinokoInputManager*)(intptr_t)destination,
        (const KinokoInputManager*)(intptr_t)source);
}

const KinokoCameraMapScriptSymbols *kinoko_camera_map_script_symbols(void) {
    static KinokoCameraMapScriptSymbols symbols;
    symbols.camera_class = g611;
    symbols.map_class = g636;
    return &symbols;
}

const KinokoInputScriptSymbols *kinoko_input_script_symbols(void) {
    static KinokoInputScriptSymbols symbols;
    symbols.object_vtable = &g16;
    symbols.null_type = g483;
    symbols.null_data = g484;
    symbols.input_class = g629;
    symbols.root = g722;
    symbols.vm = g644;
    symbols.manager_storage_bytes = sizeof(g_retdec_input_manager_state);
    return &symbols;
}

int32_t *kinoko_input_binding_type(void) {
    return kinoko_sqplus_game_type(2, kinoko_input_copy_abi);
}

int32_t *kinoko_camera_binding_type(void) {
    return kinoko_sqplus_game_type(1, function_466540);
}

static int32_t kinoko_map_copy_abi(int32_t destination, int32_t source) {
    return (int32_t)(intptr_t)kinoko_map_manager_assign(
        (KinokoMapManager*)(intptr_t)destination, (KinokoMapManager*)(intptr_t)source);
}

int32_t *kinoko_map_binding_type(void) {
    return kinoko_sqplus_game_type(3, kinoko_map_copy_abi);
}

const void *kinoko_actor_owner_methods(void) { return &g31; }

const void *kinoko_actor_render_layer_methods(void) { return &g27; }

void *kinoko_actor_class_object(void) { return g602; }

void *kinoko_actor_user_key(void) { return g600; }

struct SQVM *kinoko_actor_default_vm(void) { return (struct SQVM *)g644; }

void kinoko_actor_motion_host(KinokoActor *actor) { kinoko_actor_update_motion(kinoko_game_collision_state(), actor); }

int32_t kinoko_actor_render_host(KinokoActor *actor, KinokoCamera *camera) {
    return kinoko_actor_render(actor, camera);
}

void kinoko_actor_manager_refresh_collision(void) { kinoko_script_refresh_collision(); }

const KinokoRuntimeBootSymbols *kinoko_runtime_boot_symbols(void) {
    static KinokoRuntimeBootSymbols symbols;
    symbols.map = (KinokoMapManager *)g_retdec_map_manager_state;
    symbols.actors = (KinokoActorManager *)g_retdec_actor_manager_state;
    symbols.renderer_methods = &g184;
    symbols.render_layer_owner_slot = &g617;
    return &symbols;
}

void kinoko_application_initialize_host(void) {
    kinoko_runtime_initialize_objects(kinoko_runtime_boot_symbols());
}

const char *kinoko_application_title(void) { return g43; }

const char *kinoko_application_error(void) { return g42; }

void kinoko_application_set_archive_mode(int32_t enabled) { g874 = enabled != 0; }

void kinoko_application_open_archives(void) {
    kinoko_archive_mount("6kinoko_a.dat");
    kinoko_archive_mount("6kinoko_b.dat");
    kinoko_archive_mount("6kinoko_c.dat");
    kinoko_string_assign_n(&g554, ".cv4", 4);
}

const KinokoGameObjects *kinoko_game_objects(void) {
    static const KinokoGameObjects objects = {
        (KinokoInputManager *)g_retdec_input_manager_state,
        (KinokoActorManager *)g_retdec_actor_manager_state,
        (KinokoCamera *)g_retdec_camera_state,
        (KinokoMapManager *)g_retdec_map_manager_state
    };
    return &objects;
}

static struct SQVM *game_startup_vm;

void kinoko_game_prepare_scripts(void) {
    if (!game_startup_vm) game_startup_vm = (struct SQVM *)g644;
    retdec_trace_i32("469640:vm-before", (int32_t)(intptr_t)g644);
}

void kinoko_game_register_scripts(void) {
    retdec_trace("469640:sqrat-begin");
    retdec_trace("469640:before-473010");
    function_473010();
    retdec_trace("469640:after-473010");
    if (g644) game_startup_vm = (struct SQVM *)g644;
    else if (game_startup_vm) g644 = (char *)game_startup_vm;
    retdec_trace_i32("469640:vm-after-sqrat", (int32_t)(intptr_t)g644);
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
    if (kinoko_sqplus_object_type(retdec_game_callback_storage + 4) ==
        0x08000100) {

        retdec_trace("stagevm:global-callback");
        if (trace_index <= 16) {
            retdec_trace_i32("stagevm:global-state-vm", retdec_game_callback_storage[0]);
            retdec_trace_i32("stagevm:global-env-type", retdec_game_callback_storage[2]);
            retdec_trace_i32("stagevm:global-env-data", retdec_game_callback_storage[3]);
            retdec_trace_i32("stagevm:global-func-type", retdec_game_callback_storage[5]);
            retdec_trace_i32("stagevm:global-func-data", retdec_game_callback_storage[6]);
        }
        if (kinoko_script_callback_invoke((KinokoScriptCallback *)(intptr_t)((int32_t)(intptr_t)retdec_game_callback_storage)) < 0)
            kinoko_script_callback_clear((KinokoScriptCallback *)(intptr_t)((int32_t)(intptr_t)retdec_game_callback_storage));
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
    const int32_t *legacy = (const int32_t *)map;
    if (drawing) retdec_trace_i32("render:map-layer", legacy[3]);
    else {
        retdec_trace_i32("469900:map-act", legacy[3]);
        retdec_trace_i32("469900:map-resource", legacy[5]);
    }
}

void kinoko_game_release_script_reference(uint32_t index) {
    int32_t *references[] = { g602, g629, g611, g636 };
    if (index < 4) (int32_t)(intptr_t)(kinoko_sqplus_object_reset((void *)(intptr_t)((int32_t)(intptr_t)references[index])));
}

int32_t kinoko_game_close_vm(void) { return kinoko_script_close_vm(); }

KinokoScriptCallback *kinoko_game_global_callback(void) { return (KinokoScriptCallback *)g612; }

KinokoCollisionState *kinoko_game_collision_state(void) { return (KinokoCollisionState *)g_514300_storage; }

void kinoko_game_initialize_callback(KinokoScriptCallback *callback) {
    kinoko_script_callback_construct((KinokoScriptCallback *)(intptr_t)((int32_t)(intptr_t)callback), (const char *)(intptr_t)(0));
}

int32_t kinoko_game_load_map_file(const char *path) { return kinoko_map_manager_load((KinokoMapManager *)g_retdec_map_manager_state, path, (struct SQVM *)g644, g636, &g722); }

int32_t kinoko_game_release_map_state(void) { kinoko_map_manager_clear((KinokoMapManager *)g_retdec_map_manager_state); return 0; }

void kinoko_game_split_path(const char *path, char *directory) {
    kinoko_path_split(path, directory, NULL);
}

alignas(8) unsigned char g_retdec_input_manager_state[0x600] = {};
