#pragma once
// Private Win32 host/fixture boundary; public game ports remain in kinoko/*.h.
#include "kinoko/input_device.h"
#include "kinoko/base_utilities.h"
#include "kinoko/critical_section.h"
#include "kinoko/script_file.h"
#include "kinoko/diagnostics.h"
#include "kinoko/map_manager.h"
#include "kinoko/scene_operations.h"
#include "kinoko/game_script_api.h"
#include "kinoko/game_script_host.h"
#include "kinoko/game_runtime.h"
#include "kinoko/game_host.h"
#include "kinoko/file_io_layout.h"
#include "kinoko/file_io_legacy.h"
#include "kinoko/direct_input.h"
#include "kinoko/application.h"
#include "kinoko/renderer.h"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/camera.h"
#include "kinoko/act_layer_access.h"
#include "kinoko/pat_animation.h"
#include "kinoko/actor_render.h"
#include "kinoko/actor_manager.h"
#include "kinoko/act_document.h"
#include "kinoko/native_buffer.h"
#include "kinoko/stage_runtime.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/act_source.h"
#include "kinoko/integer_vector.h"
#include "kinoko/animation_storage.h"
#include "kinoko/integer_map.h"
#include "kinoko/actor_priority.h"
#include "kinoko/input_devices.h"
#include "kinoko/input_keys.h"
#include "kinoko/input_cluster.h"
#include "kinoko/map_containers.h"
#include "kinoko/actor_owner_list.h"
#include "kinoko/actor_pool.h"
#include "kinoko/render_queue.h"
#include "kinoko/ime_input.h"
#include "kinoko/render_target.h"
#include "kinoko/archive_random.h"
#include "kinoko/scene_queue.h"
#include "kinoko/timer_events.h"
#include "kinoko/string_layout.h"
#include "kinoko/boost_hash.h"
#include "kinoko/legacy_string.h"
#include "kinoko/act_frame.h"
#include "kinoko/audio_runtime.h"
#include "kinoko/audio_host.h"
#include "kinoko/squirrel_api_types.h"
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/native_property_bridge.h"
#include "kinoko/native_property_callbacks.h"
#include "kinoko/squirrel_binding.h"
#include "kinoko/script_registration.h"
#include "kinoko/squirrel_native_calls.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/squirrel_legacy_api.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_vm_lifecycle.h"
#include "kinoko/csv_bridge.h"
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>
#include <intrin.h>
#include <d3d9.h>
#include <dinput.h>
#include <zlib.h>
#include "retdec_asm_stubs.h"
#include "kinoko/squirrel_compile_bridge.h"
#include "kinoko/squirrel_vm_bootstrap.h"
#include "kinoko/squirrel_value_bridge.h"
#include "kinoko/actor_collision.h"
#include "kinoko/map_collision.h"
#include "kinoko/game_math.h"
#include "kinoko/audio_math.h"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/squirrel_gc_bridge.h"
#include "kinoko/act_resource.h"
#include "kinoko/act_clone.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_object.h"
#include "kinoko/sprite.h"
#include "resource.h"
#include "kinoko/texture_store.h"
#include "kinoko/map_render.h"
#include "kinoko/map_activation.h"

#define retdec_compile_environment_vtable g39
#define retdec_compile_environment_type g483
#define retdec_compile_environment_slot g484
#define retdec_game_callback_storage g612
#define RETDEC_ACT_TEXTURE_SLOT_COUNT KINOKO_TEXTURE_CAPACITY
#define g_retdec_act_texture_slots kinoko_texture_slots

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t kinoko_script_assets_packed(void);

void retdec_trace_ref_watch(const char *label, int32_t shared_state,
                                   int32_t type, int32_t data);

extern int32_t retdec_primary_shared_state;

extern int32_t retdec_release_watch_data[8];

extern int32_t retdec_release_watch_count;

int32_t retdec_is_release_watch_data(int32_t data);

struct retdec_mcd_data;

extern void retdec_mcd_free(struct retdec_mcd_data *data);

void retdec_act_free_map_records(int32_t layout);

int32_t retdec_act_load(int32_t this_ptr, int32_t reader_ptr,
                               int32_t version);

int32_t retdec_begin_stage_this(int32_t resource_ptr,
                                        int32_t stage);

extern int32_t retdec_publish_act_layers(int32_t vm, int32_t act,
                                          int32_t resource_ptr,
                                          int32_t *active_count);

int32_t retdec_publish_cact_resource2d_class(int32_t vm,
                                                     int32_t root_object);

extern int32_t retdec_load_act_texture(const char *texture_name);

int32_t retdec_layout_submit_impl(int32_t vertex_buffer,
                                          float x, float y);

int32_t retdec_c2dlayout_set_layer_impl(int32_t layout,
                                                int32_t layer);

int32_t retdec_c2dlayout_draw_impl(int32_t layout,
                                           float x, float y);

void retdec_trace_star_state(const char *phase, int32_t actor);

uint32_t timeGetTime(void);

uint32_t timeBeginPeriod(uint32_t period);

extern void retdec_trace(const char *message);

__declspec(noinline) void retdec_trace_i32(const char *label,
                                                  int32_t value);

void retdec_trace_squirrel_name(const char *label, int32_t name_ptr);

void retdec_trace_squirrel_table_entries(const char *label,
                                                int32_t table_ptr);

void retdec_destroy_cact_script(int32_t script_ptr);

void retdec_destroy_cact_object(int32_t object_ptr);

int32_t retdec_destroy_cact_with_flags(int32_t object_ptr,
                                               unsigned char flags);

void retdec_trace_hresult(const char *label, long value);

int _vsprintf_compat(char *buffer, const char *format, va_list args);

typedef float float32_t;

typedef long double float80_t;

struct retdec_LIST_ENTRY {
    struct retdec_LIST_ENTRY * e0;
    struct retdec_LIST_ENTRY * e1;
};

struct retdec_RTL_CRITICAL_SECTION {
    struct retdec_RTL_CRITICAL_SECTION_DEBUG * e0;
    int32_t e1;
    int32_t e2;
    int32_t * e3;
    int32_t * e4;
    int32_t e5;
};

struct retdec_RTL_CRITICAL_SECTION_DEBUG {
    int16_t e0;
    int16_t e1;
    struct retdec_RTL_CRITICAL_SECTION * e2;
    struct retdec_LIST_ENTRY e3;
    int32_t e4;
    int32_t e5;
    int32_t e6;
    int16_t e7;
    int16_t e8;
};

struct vtable_4d54a4_type {
    int32_t (*e0)(char);
};

struct vtable_4d54ac_type {
    int32_t (*e0)(char);
};

struct vtable_4d59bc_type {
    int32_t (*e0)(int32_t);
};

struct vtable_4d5a04_type {
    int32_t (__fastcall *e0)(int32_t, void *, unsigned char);
    int32_t (*e1)(int32_t);
    int32_t (*e2)(uint32_t);
    int32_t (__fastcall *e3)(int32_t, void *, uint32_t);
    int32_t (__fastcall *e4)(int32_t, void *);
};

struct vtable_4d5a5c_type {
    int32_t (__fastcall *e0)(int32_t, void *, unsigned char);
    int32_t (*e1)();
};

struct vtable_4d5ba0_type {
    int32_t (__fastcall *e0)(int32_t, void *, int32_t);
};

struct vtable_4d5c68_type {
    int32_t (__fastcall *e0)(int32_t, void *, int32_t);
    int32_t (__fastcall *e1)(int32_t, void *);
    int32_t (__fastcall *e2)(int32_t, void *, int32_t);
};

struct vtable_4d5c80_type {
    int32_t (__fastcall *e0)(int32_t, void *, int32_t);
    int32_t (__fastcall *e1)(int32_t, void *);
    int32_t (__fastcall *e2)(int32_t, void *, int32_t);
};

struct vtable_4eb20c_type {
    int32_t (__fastcall *e0)(KinokoRenderer *, void *);
    int32_t (__fastcall *e1)(KinokoRenderer *, void *);
};

struct vtable_4ebacc_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
};

struct vtable_4ebe58_type {
    int32_t (__fastcall *e0)(int32_t, void *, int32_t);
    int32_t (__fastcall *e1)(int32_t, void *);
    int32_t (__fastcall *e2)(int32_t, void *, int32_t);
};

struct vtable_4ebe68_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)(char);
    int32_t (*e5)();
    int32_t (*e6)(int32_t);
    int32_t (*e7)(int32_t, int32_t, int32_t);
    int32_t (*e8)(int32_t);
};

struct vtable_4ebe90_type {
    int32_t (__fastcall *e0)(int32_t, void *, int32_t);
    int32_t (__fastcall *e1)(int32_t, void *);
    int32_t (__fastcall *e2)(int32_t, void *, int32_t);
};

struct vtable_4ec0b0_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)(char);
    int32_t (*e5)();
};

struct vtable_4ec1d4_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)(char);
    KinokoActDocument *(__fastcall *e5)(KinokoActDocument *, void *);
    int32_t (*e6)(int32_t);
    int32_t (*e7)();
    int32_t (*e8)();
};

struct vtable_4ec358_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)();
    int32_t (*e5)();
    int32_t (*e6)(int32_t);
    int32_t (*e7)();
    int32_t (*e8)(float32_t, float32_t);
    int32_t (*e9)();
};

struct vtable_4ec548_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)(char);
    int32_t (*e5)();
    int32_t (*e6)(int32_t);
    int32_t (*e7)(int32_t, int32_t);
    int32_t (*e8)(int32_t, int32_t);
    int32_t (*e9)();
    int32_t (*e10)(int32_t);
};

struct vtable_4ec79c_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)();
    int32_t (__fastcall *e5)(int32_t, void *);
    int32_t (*e6)(int32_t);
    int32_t (__fastcall *e7)(int32_t, void *);
    int32_t (__fastcall *e8)(int32_t, void *, float32_t, float32_t);
    int32_t (*e9)();
    int32_t (__fastcall *e10)(int32_t, void *, int32_t, int32_t, int32_t, int32_t);
};

struct vtable_4ec7cc_type {
    int32_t (__fastcall *e0)(int32_t, void *, int32_t);
    uint32_t (__fastcall *e1)(KinokoColoredQuad *, void *, uint32_t);
    uint32_t (__fastcall *e2)(KinokoColoredQuad *, void *, const uint32_t *);
    uint32_t (__fastcall *e3)(KinokoColoredQuad *, void *, uint32_t);
};

struct vtable_4eccfc_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)(char);
    int32_t (*e5)();
    int32_t (*e6)(int32_t);
    int32_t (*e7)(int32_t, int32_t);
    int32_t (*e8)(int32_t, int32_t);
    int32_t (*e9)();
    int32_t (*e10)(int32_t);
    int32_t (*e11)();
};

struct vtable_4ece50_type {
    int32_t (*e0)(int32_t);
    int32_t (*e1)(int32_t, int32_t);
    int32_t (*e2)(int32_t, int32_t);
    int32_t (*e3)();
    int32_t (*e4)(char);
    int32_t (*e5)();
    int32_t (*e6)(int32_t);
    int32_t (*e7)(int32_t, int32_t);
    int32_t (*e8)(int32_t, int32_t);
    int32_t (*e9)();
    int32_t (*e10)(int32_t);
    int32_t (*e11)();
    int32_t (*e12)(int32_t, int32_t);
};

struct vtable_4ed2cc_type {
    int32_t (*e0)(char);
    uint32_t (__fastcall *e1)(KinokoColoredQuad *, void *, uint32_t);
    uint32_t (__fastcall *e2)(KinokoColoredQuad *, void *, const uint32_t *);
    uint32_t (__fastcall *e3)(KinokoColoredQuad *, void *, uint32_t);
    int32_t (__fastcall *e4)(KinokoSprite *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
    int32_t (__fastcall *e5)(KinokoSprite *, void *, int32_t, int32_t, int32_t, int32_t, int32_t);
    int32_t (__fastcall *e6)(KinokoSprite *, void *, float, float, float, float);
    int32_t (__fastcall *e7)(KinokoSprite *, void *, float32_t, float32_t);
    int32_t (__fastcall *e8)(KinokoSprite *, void *, float32_t, float32_t);
    int32_t (__fastcall *e9)(KinokoSprite *, void *, float32_t, float32_t);
};

void _3f__3f_3_40_YAXPAX_40_Z(int32_t * a1);

void* kinoko_script_initialize_root(void);

int32_t kinoko_script_close_vm(void);

int32_t kinoko_script_show_call_stack(void);

void* kinoko_script_root(void);

int32_t kinoko_script_load_file(const char* path, const void* environment);

int32_t function_415fd0(int32_t a1, int32_t a2);

int32_t function_41e390(int32_t a1);

int32_t function_41eff0(int32_t a1);

int32_t __fastcall kinoko_act_layer_associate_method(int32_t receiver, void* unused_edx);

int32_t *kinoko_c2d_layout_type(void);

int32_t function_42b6d0(int32_t a1);

int32_t *kinoko_chip_resource_type(void);

int32_t function_42f350(int32_t a1);

int32_t *kinoko_map_layout_type(void);

int32_t function_433c90(int32_t a1);

int32_t kinoko_script_dprint_noop(void);

int32_t *kinoko_texture_resource_type(void);

int32_t function_446520(int32_t a1);

int32_t *kinoko_render_target_type(void);

int32_t function_4495a0(int32_t a1);

int32_t __fastcall kinoko_color_destroy(int32_t receiver, void* unused_edx, char flags);

int32_t kinoko_clear_global_stages(void);

int32_t kinoko_clear_global_sound(void);

int32_t retdec_root_table_register_resource(int32_t root_object,
                                                    int32_t resource_ptr);

int32_t retdec_root_table_construct_this(int32_t resource_ptr,
                                                 int32_t vm,
                                                 int32_t output_ptr);

int32_t kinoko_actor_register_script_class(void);

int32_t function_465f70(void);

int32_t function_466270(void);

int32_t function_466540(int32_t a1, int32_t a2);

int32_t function_466770(int32_t * a1, int32_t a2, int32_t a3, int32_t a4);

int32_t kinoko_camera_update_entry(int32_t a1);

int32_t function_4669d0(void);

int32_t function_4693a0(int32_t a1);

int32_t function_46a210(int32_t * a1);

int32_t kinoko_register_input_class(void);

int32_t function_46f140(int32_t a1);

int32_t function_46f200(int32_t * a1, int32_t a2, int32_t a3, int32_t a4);

int32_t function_46fac0(void);

int32_t function_470030(int32_t a1);

int32_t function_470890(void);

int32_t function_470df0(int32_t a1, int32_t a2);

int32_t function_470ee0(int32_t a1);

int32_t function_470f60(int32_t a1);

int32_t function_470f80(int32_t dwMilliseconds);

int32_t function_470f90(void);

int32_t function_471080(void);

int32_t function_471160(int32_t callback_ptr, int32_t vm, int32_t index);

int32_t function_471330(int32_t callback_ptr, int32_t vm, int32_t index);

int32_t function_471880(int32_t callback_ptr, int32_t vm, int32_t index);

int32_t function_471960(int32_t callback_ptr, int32_t vm, int32_t index);

int32_t kinoko_script_compile_file_argument(int32_t path, int32_t object_vtable, int32_t vm,
                       int32_t type, int32_t data, char owns_reference);

int32_t function_471bc0(int32_t a1);

int32_t function_471c10(int32_t a1);

int32_t function_471d90(int32_t a1);

int32_t function_471eb0(int32_t a1);

int32_t function_471f10(int32_t a1);

int32_t function_471fd0(int32_t a1);

int32_t function_472080(int32_t a1);

int32_t function_4720e0(int32_t a1);

int32_t function_472140(int32_t a1);

int32_t kinoko_script_bind_root_value(int32_t * a1, int32_t * a2, char * a3, int32_t a4);

int32_t kinoko_script_bind_root_integer(int32_t * a1, int32_t a2, char * a3);

int32_t function_4722e0(int32_t *stream_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);

int32_t function_472820(int32_t stream_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);

int32_t function_472c90(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);

int32_t function_472e50(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);

int32_t function_473010(void);

int32_t function_48a170(int32_t a1);

int32_t  kinoko_sqplus_release_vm_wrappers(void);

int32_t  kinoko_sqplus_print(struct SQVM * vm, const char *format, ...);

void * kinoko_sqplus_root_object(void);

int32_t  kinoko_sqplus_select_vm(struct SQVM * a1);

extern int32_t function_4aa210(int32_t source_ptr, int32_t *target_ptr);

int32_t function_4ab170(int32_t a1, int32_t a2, int32_t a3, int32_t a4);

int32_t kinoko_register_stage_list_cleanup(void);

int32_t kinoko_register_render_queue_cleanup(void);

int32_t kinoko_register_sound_tree_cleanup(void);

int32_t function_4d47f0(void);

int32_t function_4d4860(void);

extern int32_t (__fastcall *g23)(int32_t, void*, char);

extern int32_t g25;

extern int32_t g28;

extern const char * g42;

extern const char * g43;

extern const char * g209;

extern struct vtable_4ec7cc_type g300;

extern int32_t g350[11];

extern int32_t g483;

extern int32_t g484;

extern int32_t g534;

extern int32_t g535;

extern int32_t g545;

extern int32_t g546;

extern char g547;

extern char g548;

extern char g549;

extern int32_t g550;

extern int32_t kinoko_act_script_extension[7];

extern char g560;

extern int32_t g600[3];

extern int32_t g601[3];

extern int32_t g602[3];

extern int32_t g603;

extern int32_t g604;

extern int32_t g611[3];

extern int32_t retdec_game_callback_storage[7];

extern int32_t g_514300_storage[30];

extern __declspec(align(8)) unsigned char g_retdec_actor_manager_state[0x200];

extern __declspec(align(8)) unsigned char g_retdec_map_manager_state[0x200];

extern __declspec(align(8)) unsigned char g_retdec_camera_state[0x200];

extern int32_t g617;

extern int32_t g629[3];

extern int32_t g636[3];

extern int32_t g637;

extern KinokoIntegerMap* g638;

extern int32_t g639;

extern char g642;

extern int32_t g643;

extern __declspec(align(4096)) char * g644;

extern int32_t g645;

extern int32_t unk_5149EC[3];

extern int32_t g664;

extern char g673;

extern KinokoRenderer kinoko_renderer;

extern KinokoGraphics kinoko_graphics;

extern KinokoCriticalSection kinoko_graphics_lock;

extern int32_t g722[3];

extern char * g767;

extern unsigned char g_retdec_keyboard_state[256];

extern char g874;

extern int32_t g876;

extern char * g877;

extern int32_t g878;

extern int32_t g910;

extern int32_t g914;

extern int32_t g918;

extern int32_t g926;

extern int32_t g930;

extern int32_t g934;

extern int32_t kinoko_mesh_manager_slot;

extern char kinoko_resource2d_class_published;

extern int32_t kinoko_acting_player_class_pair[2];

extern int32_t kinoko_resource2d_class_pair[2];

extern int32_t kinoko_layout_set_pair[2];

extern int32_t kinoko_layout_get_pair[2];

extern int32_t kinoko_layout_class_pair[2];

extern int32_t kinoko_layer_set_pair[2];

extern int32_t kinoko_layer_get_pair[2];

extern int32_t g1224;

extern struct vtable_4d54a4_type g16;

extern struct vtable_4d54ac_type g17;

extern struct vtable_4d59bc_type g27;

extern struct vtable_4d5a04_type g29;

extern struct vtable_4d5a5c_type g31;

extern struct vtable_4d5ba0_type g37;

extern struct vtable_4d5c68_type g39;

extern struct vtable_4d5c80_type g40;

extern struct vtable_4eb20c_type g184;

extern struct vtable_4ebacc_type g231;

extern struct vtable_4ebe58_type g251;

extern struct vtable_4ebe68_type g252;

extern struct vtable_4ebe90_type g253;

extern struct vtable_4ec0b0_type g277;

extern struct vtable_4ec1d4_type g285;

extern struct vtable_4ec358_type g299;

extern struct vtable_4ec548_type g313;

extern struct vtable_4ec79c_type g327;

extern struct vtable_4ec7cc_type g328;

extern struct vtable_4eccfc_type g365;

extern struct vtable_4ece50_type g379;

extern struct vtable_4ed2cc_type g407;

int32_t WINAPI D3DXCreateTexture(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8);

int32_t D3DXMatrixMultiply(void);

int32_t D3DXMatrixRotationYawPitchRoll(int32_t * a1, float80_t a2, float80_t a3, float80_t a4, int32_t a5, int32_t a6, int32_t a7);

int32_t D3DXMatrixScaling(int32_t * a1, float80_t a2, float80_t a3, float80_t a4);

int32_t D3DXMatrixTranslation(int32_t * a1, float80_t a2, float80_t a3, float80_t a4);

extern unsigned char g_retdec_input_manager_state[0x600];

int32_t retdec_layout_submit_impl(int32_t vertex_buffer,
                                          float32_t x, float32_t y);

int32_t function_415fd0(int32_t script, int32_t environment);

int32_t function_41e390(int32_t storage);

uint32_t kinoko_actor_motion_update_mask(void);

void kinoko_actor_render_set_blend(int32_t mode);

int32_t kinoko_actor_render_submit(KinokoAnimationFrame *frame);

const void *kinoko_pat_frame_methods(void);

int32_t function_468620_this(int32_t this_ptr);

int32_t function_468950_this(int32_t this_ptr, int32_t actor_ptr);

int32_t function_4693a0(int32_t layout);

int32_t function_46f140(int32_t name_ptr);

int32_t function_470030(int32_t name_ptr);

int32_t retdec_compile_file_native(int32_t vm);

int32_t kinoko_script_bind_root_value(int32_t *object, int32_t *value, char *name, int32_t flags);

int32_t kinoko_script_bind_root_integer(int32_t *object, int32_t value, char *name);

__declspec(noinline) int32_t retdec_stack_vm(void);

const KinokoSqplusVmSlots *kinoko_sqplus_vm_slots(void);

struct SQVM *kinoko_script_open_primary_vm(int32_t stack_size);

typedef int32_t (*retdec_stream_callback)(int32_t, int32_t, int32_t);

typedef int32_t (__cdecl *retdec_native_fn)(void);

typedef struct retdec_native_entry {
    const char *name;
    retdec_native_fn function_ptr;
    int32_t argument_size;
    const char *type_mask;
} retdec_native_entry;

int32_t kinoko_sqrat_object_vtable(void);

int32_t kinoko_sqrat_root_vtable(void);

int32_t kinoko_actor_vtable(void);

int32_t kinoko_actor_step_key(void);

int32_t kinoko_squirrel_object_vtable(void);

int32_t function_4aa210(int32_t source_ptr, int32_t *target_ptr);

int32_t *kinoko_native_binding_type(int32_t category);

int32_t kinoko_native_void_type(void);

int32_t function_4ab170(int32_t vm, int32_t class_name,
                        int32_t native_pointer, int32_t release_hook);

typedef void (__cdecl *retdec_sq_print_fn)(int32_t vm, const char *format, ...);

int32_t kinoko_csv_load_bytes(const char *path, char **bytes);

int32_t kinoko_act_script_output_compiled(void);

const struct KinokoActHostSymbols* kinoko_act_host_symbols(void);

const struct KinokoAudioHostSymbols* kinoko_audio_host_symbols(void);

const KinokoCameraMapScriptSymbols *kinoko_camera_map_script_symbols(void);

const KinokoInputScriptSymbols *kinoko_input_script_symbols(void);

int32_t *kinoko_input_binding_type(void);

int32_t *kinoko_camera_binding_type(void);

int32_t *kinoko_map_binding_type(void);

const void *kinoko_actor_owner_methods(void);

const void *kinoko_actor_render_layer_methods(void);

void *kinoko_actor_class_object(void);

void *kinoko_actor_user_key(void);

struct SQVM *kinoko_actor_default_vm(void);

void kinoko_actor_motion_host(KinokoActor *actor);

int32_t kinoko_actor_render_host(KinokoActor *actor, KinokoCamera *camera);

void kinoko_actor_manager_refresh_collision(void);

const KinokoRuntimeBootSymbols *kinoko_runtime_boot_symbols(void);

void kinoko_application_initialize_host(void);

const char *kinoko_application_title(void);

const char *kinoko_application_error(void);

void kinoko_application_set_archive_mode(int32_t enabled);

void kinoko_application_open_archives(void);

const KinokoGameObjects *kinoko_game_objects(void);

void kinoko_game_prepare_scripts(void);

void kinoko_game_register_scripts(void);

int32_t kinoko_game_initialize_input(KinokoInputManager *input);

int32_t kinoko_game_update_input(KinokoInputManager *input);

int32_t kinoko_game_load_boot_script(void);

void kinoko_game_update_callback(int32_t trace_index);

int32_t kinoko_game_update_map(KinokoMapManager *map);

void kinoko_game_prepare_map(KinokoMapManager *map, KinokoCamera *camera);

void kinoko_game_clear_map(KinokoMapManager *map);

void kinoko_game_clear_actors(KinokoActorManager *actors);

void kinoko_game_trace_map(KinokoMapManager *map, int32_t drawing);

void kinoko_game_release_script_reference(uint32_t index);

int32_t kinoko_game_close_vm(void);

KinokoScriptCallback *kinoko_game_global_callback(void);

KinokoCollisionState *kinoko_game_collision_state(void);

void kinoko_game_initialize_callback(KinokoScriptCallback *callback);

int32_t kinoko_game_load_map_file(const char *path);

int32_t kinoko_game_release_map_state(void);

void kinoko_game_split_path(const char *path, char *directory);

#ifdef __cplusplus
}
#endif
