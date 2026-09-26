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
#include <windows.h>
#include <intrin.h>
#include <d3d9.h>
#include <dinput.h>
#include <zlib.h>
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

#define kinoko_compile_environment_vtable kinoko_sqrat_object_methods_storage
#define kinoko_compile_environment_type kinoko_null_object_type
#define kinoko_compile_environment_slot kinoko_null_object_value

#define RETDEC_ACT_TEXTURE_SLOT_COUNT KINOKO_TEXTURE_CAPACITY
#define g_kinoko_act_texture_slots kinoko_texture_slots

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t kinoko_script_assets_packed(void);

extern int32_t kinoko_primary_shared_state;

extern int32_t kinoko_release_watch_data[8];

extern int32_t kinoko_release_watch_count;

struct kinoko_mcd_data;

void kinoko_trace_star_state(const char *phase, int32_t actor);

uint32_t timeGetTime(void);

typedef float float32_t;

struct SquirrelObjectMethods {
    int32_t (__fastcall *destroy)(int32_t object, void *unused,
                                                int32_t flags);
};

struct ActorMethods {
    KinokoActor * (__fastcall *destroy)(KinokoActor *actor, void* unused_edx, unsigned char flags);
};

struct ActorRenderLayerMethods {
    int32_t (__fastcall *update)(int32_t receiver, void* unused_edx, int32_t argument);
};

struct ActorPoolMethods {
    int32_t (__fastcall *destroy)(int32_t manager, void *unused, unsigned char flags);
    int32_t (__fastcall *top)(int32_t receiver, void* unused_edx, int32_t output);
    int32_t (__fastcall *remove)(int32_t receiver, void* unused_edx, uint32_t handle);
    int32_t (__fastcall *lookup)(int32_t manager, void *unused, uint32_t handle);
    int32_t (__fastcall *count)(int32_t manager, void *unused);
};

struct ActorOwnerMethods {
    int32_t (__fastcall *destroy)(int32_t manager, void *unused, unsigned char flags);
    int32_t (__fastcall *push)(int32_t receiver, void* unused_edx);
};

struct MapRenderLayerMethods {
    int32_t (__fastcall *update)(int32_t layer, void *unused,
    int32_t camera);
};

struct SqratObjectMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};

struct SqratRootMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};

struct RendererMethods {
    int32_t (__fastcall *before_reset)(KinokoRenderer *object, void *unused);
    int32_t (__fastcall *after_reset)(KinokoRenderer *object, void *unused);
};

struct ActScriptMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
};

struct ActLayerReferenceMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};

struct ActLayerMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *delete_object)(int32_t receiver, void* unused_edx, unsigned char flags);
    int32_t (__fastcall *clone)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *associate)(
    KinokoActLayer *layer, void *unused, KinokoActResource *resource);
    KinokoActLayer * (__fastcall *world_position)(KinokoActLayer *layer,
    void *unused, float *x, float *y, float *z);
    int32_t (__fastcall *register_class)(int32_t receiver, void* unused_edx, int32_t parent, int32_t flags);
};

struct ActLayerLayoutMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};

struct ActKeyMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *delete_object)(int32_t receiver, void* unused_edx, unsigned char flags);
    int32_t (__fastcall *clone)(int32_t receiver, void* unused_edx);
};

struct ActDocumentMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *delete_object)(int32_t receiver, void* unused_edx, unsigned char flags);
    KinokoActDocument * (__fastcall *clone)(KinokoActDocument *source, void *unused);
    int32_t (__fastcall *load_resources)(KinokoActDocument *document, void *unused, const char *prefix);
    int32_t (__fastcall *suspend_resources)(KinokoActDocument *document, void *unused);
    int32_t (__fastcall *resume_resources)(KinokoActDocument *document, void *unused);
};

struct ActLayoutMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t * (*type)(void);
    int32_t (__fastcall *clone)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *associate)(int32_t receiver, void* unused_edx, int32_t layer);
    int32_t (__fastcall *update)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *draw)(int32_t receiver, void* unused_edx, float x, float y);
    int32_t (__fastcall *register_class)(int32_t receiver, void* unused_edx);
};

struct ChipResourceMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *delete_object)(int32_t receiver, void* unused_edx, unsigned char flags);
    int32_t * (*type)(void);
    int32_t (__fastcall *register_class)(int32_t receiver, void* unused_edx, int32_t vm);
    int32_t (__fastcall *resource_42f800)(int32_t receiver, void* unused_edx, int32_t object, const char* name);
    int32_t (__fastcall *resource_42f6c0)(int32_t receiver, void* unused_edx, int32_t object, const char* name);
    int32_t (__fastcall *clone)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *load)(int32_t receiver, void* unused_edx, const char* prefix);
};

struct MapLayoutMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t * (*type)(void);
    int32_t (__fastcall *clone)(int32_t source, void *unused);
    int32_t (__fastcall *associate)(int32_t receiver, void* unused_edx, int32_t layer);
    int32_t (__fastcall *update)(int32_t layout, void *unused);
    int32_t (__fastcall *draw)(int32_t layout, void *unused,
    float x, float y);
    int32_t (__fastcall *register_class)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *update_visible)(int32_t layout, void *unused,
    int32_t left, int32_t top, int32_t right, int32_t bottom);
};

struct QuadColorMethods {
    int32_t (__fastcall *destroy)(int32_t sprite, void *unused, int32_t flags);
    uint32_t (__fastcall *set_color)(KinokoColoredQuad *quad, void *unused, uint32_t color);
    uint32_t (__fastcall *set_vertex_colors)(KinokoColoredQuad *quad, void *unused, const uint32_t *colors);
    uint32_t (__fastcall *modulate_color)(KinokoColoredQuad *quad, void *unused, uint32_t color);
};

struct TextureResourceMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *delete_object)(int32_t receiver, void* unused_edx, unsigned char flags);
    int32_t * (*type)(void);
    int32_t (__fastcall *register_class)(int32_t receiver, void* unused_edx, int32_t vm);
    int32_t (__fastcall *resource_446920)(int32_t receiver, void* unused_edx, int32_t object, const char* name);
    int32_t (__fastcall *resource_4467e0)(int32_t receiver, void* unused_edx, int32_t object, const char* name);
    int32_t (__fastcall *clone)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *load)(int32_t receiver, void* unused_edx, const char* prefix);
    int32_t (__fastcall *unload)(int32_t receiver, void* unused_edx);
};

struct RenderTargetMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *delete_object)(int32_t receiver, void* unused_edx, unsigned char flags);
    int32_t * (*type)(void);
    int32_t (__fastcall *register_class)(int32_t receiver, void* unused_edx, int32_t vm);
    int32_t (__fastcall *resource_4499a0)(int32_t receiver, void* unused_edx, int32_t object, const char* name);
    int32_t (__fastcall *resource_449860)(int32_t receiver, void* unused_edx, int32_t object, const char* name);
    int32_t (__fastcall *clone)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *load)(int32_t receiver, void* unused_edx, const char* prefix);
    int32_t (__fastcall *unload)(int32_t receiver, void* unused_edx);
    int32_t (__fastcall *create)(KinokoActResource *resource, void *unused,
                                                      int32_t width, int32_t height);
};

struct SpriteMethods {
    int32_t (__fastcall *destroy)(int32_t receiver, void* unused_edx, char flags);
    uint32_t (__fastcall *set_color)(KinokoColoredQuad *quad, void *unused, uint32_t color);
    uint32_t (__fastcall *set_vertex_colors)(KinokoColoredQuad *quad, void *unused, const uint32_t *colors);
    uint32_t (__fastcall *modulate_color)(KinokoColoredQuad *quad, void *unused, uint32_t color);
    int32_t (__fastcall *set_rect_pivot)(KinokoSprite *sprite, void *unused,
    int32_t texture, int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t pivot_x, int32_t pivot_y);
    int32_t (__fastcall *set_rect)(KinokoSprite *sprite, void *unused,
    int32_t texture, int32_t x, int32_t y, int32_t width, int32_t height);
    int32_t (__fastcall *draw_bounds)(KinokoSprite *sprite, void *unused,
    float left, float top, float right, float bottom);
    int32_t (__fastcall *draw_404770)(KinokoSprite *sprite, void *unused, float x, float y);
    int32_t (__fastcall *draw_404bc0)(KinokoSprite *sprite, void *unused, float x, float y);
    int32_t (__fastcall *draw_4049c0)(KinokoSprite *sprite, void *unused, float x, float y);
};

void kinoko_host_free_allocation(int32_t * a1);

int32_t kinoko_host_register_act_script_abi(int32_t a1, int32_t a2);

int32_t kinoko_host_construct_layer_abi(int32_t a1);

int32_t kinoko_register_cact_layer_class(struct SQVM* a1);

int32_t *kinoko_c2d_layout_type(void);

int32_t kinoko_register_c2dlayout_class(struct SQVM* a1);

int32_t *kinoko_chip_resource_type(void);

int32_t kinoko_register_chip_resource_class(struct SQVM* a1);

int32_t *kinoko_map_layout_type(void);

int32_t kinoko_register_map_layout_class(struct SQVM* a1);

int32_t kinoko_script_dprint_noop(void);

int32_t *kinoko_texture_resource_type(void);

int32_t kinoko_register_texture_resource_class(struct SQVM* a1);

int32_t *kinoko_render_target_type(void);

int32_t kinoko_register_render_target_class(struct SQVM* a1);

int32_t __fastcall kinoko_color_destroy(int32_t receiver, void* unused_edx, char flags);

int32_t kinoko_actor_register_script_class(void);

int32_t kinoko_host_clear_stages(void);

int32_t kinoko_host_initialize_camera(void);

int32_t kinoko_camera_class_copy_abi(int32_t a1, int32_t a2);

int32_t kinoko_register_camera_binding(void);

int32_t kinoko_host_register_collision_map_abi(int32_t a1);

int32_t kinoko_host_append_render_item(int32_t * a1);

int32_t kinoko_host_find_map_layout_abi(int32_t a1);

int32_t kinoko_register_map_binding(void);

int32_t kinoko_host_create_map_layer_abi(int32_t a1);

int32_t kinoko_host_clear_sound(void);

int32_t kinoko_host_show_message_abi(int32_t a1);

int32_t kinoko_host_sleep_abi(int32_t dwMilliseconds);

int32_t kinoko_host_milliseconds(void);

int32_t kinoko_host_close_window(void);

int32_t kinoko_script_bind_root_value(int32_t * a1, int32_t * a2, char * a3, int32_t a4);

int32_t kinoko_script_bind_root_integer(int32_t * a1, int32_t a2, char * a3);

int32_t function_472c90(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);

int32_t function_472e50(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);

int32_t kinoko_host_open_vm_abi(int32_t a1);

extern int32_t kinoko_host_get_delegate_abi(int32_t source_ptr, int32_t *target_ptr);

int32_t kinoko_host_create_native_instance_abi(int32_t a1, int32_t a2, int32_t a3, int32_t a4);

int32_t kinoko_host_destroy_global_callback(void);

extern const char * kinoko_application_error_text;

extern const char * kinoko_application_title_text;

extern const char * kinoko_audio_error_text;

extern struct QuadColorMethods kinoko_layout_color_methods_storage;

struct StringLayoutMethods {
    int32_t (__fastcall *write)(int32_t receiver, void* unused_edx, int32_t writer);
    int32_t (__fastcall *read)(int32_t object, void *unused, int32_t holder, int32_t version);
    int32_t (__fastcall *query)(int32_t receiver, void* unused_edx, int32_t type, int32_t output);
    int32_t (__fastcall *destroy)(int32_t object, void *unused);
    int32_t (__fastcall *type)(int32_t object, void *unused);
    int32_t (__fastcall *clone)(int32_t object, void *unused);
    int32_t (__fastcall *set_layer)(int32_t object, void *unused, int32_t layer);
    int32_t (__fastcall *update)(int32_t object, void *unused);
    int32_t (__fastcall *draw)(int32_t object, void *unused, float x, float y);
    int32_t (__fastcall *register_class)(int32_t object, void *unused);
    int32_t (__fastcall *delete_object)(int32_t object, void *unused, unsigned char flags);
};
extern struct StringLayoutMethods kinoko_string_layout_methods_storage;

extern int32_t kinoko_null_object_type;

extern int32_t kinoko_null_object_value;

extern int32_t kinoko_ime_context_slot;

extern int32_t kinoko_ime_default_window_slot;

extern int32_t kinoko_ime_text_limit;

extern int32_t kinoko_ime_commit_pending;

extern char kinoko_ime_text_changed;

extern char kinoko_ime_composition_changed;

extern char kinoko_ime_enabled;

extern int32_t kinoko_ime_cursor;



extern char kinoko_sqrat_trace_enabled;

extern int32_t kinoko_actor_user_key_storage[3];

extern int32_t kinoko_actor_step_key_storage[3];

extern int32_t kinoko_actor_class_storage[3];

extern int32_t kinoko_stage_list_slot;

extern int32_t kinoko_stage_count;

extern int32_t kinoko_camera_class_storage[3];

extern int32_t kinoko_render_layer_owner_slot;

extern int32_t kinoko_input_class_storage[3];

extern int32_t kinoko_map_class_storage[3];

extern int32_t kinoko_active_bgm_slot;

extern KinokoIntegerMap* kinoko_sound_lookup;

extern int32_t kinoko_sound_lookup_count;

extern char kinoko_skip_vm_owner_reset;

extern int32_t kinoko_newest_shared_state;

extern __declspec(align(4096)) struct SQVM *kinoko_primary_vm;

extern int32_t kinoko_cached_root_slot;

extern int32_t kinoko_vm_thread_wrapper[3];

extern int32_t kinoko_act_vm_abi_slot;

extern char kinoko_compile_act_output;

extern KinokoRenderer kinoko_renderer;

extern KinokoGraphics kinoko_graphics;

extern KinokoCriticalSection kinoko_graphics_lock;

extern int32_t kinoko_script_root_storage[3];

extern char * kinoko_game_window_slot;

extern unsigned char kinoko_keyboard_state[256];

extern char kinoko_packed_assets;

extern int32_t kinoko_audio_primary_device_slot;

extern char * kinoko_audio_device_slot;

extern int32_t kinoko_audio_listener_slot;

extern int32_t kinoko_layout_type_identity;

extern int32_t kinoko_chip_type_identity;

extern int32_t kinoko_map_type_identity;

extern int32_t kinoko_string_layout_type_identity;

extern int32_t kinoko_texture_type_identity;

extern int32_t kinoko_render_target_type_identity;

extern int32_t kinoko_mesh_manager_slot;

extern char kinoko_resource2d_class_published;

extern int32_t kinoko_acting_player_class_pair[2];

extern int32_t kinoko_resource2d_class_pair[2];

extern int32_t kinoko_layout_set_pair[2];

extern int32_t kinoko_layout_get_pair[2];

extern int32_t kinoko_layout_class_pair[2];

extern int32_t kinoko_layer_set_pair[2];

extern int32_t kinoko_layer_get_pair[2];

extern int32_t kinoko_script_void_result_identity;

extern struct SquirrelObjectMethods kinoko_squirrel_object_methods_storage;

extern struct ActorMethods kinoko_actor_methods_storage;

extern struct ActorRenderLayerMethods kinoko_actor_render_layer_methods_storage;

extern struct ActorPoolMethods kinoko_actor_pool_methods_storage;

extern struct ActorOwnerMethods kinoko_actor_owner_methods_storage;

extern struct MapRenderLayerMethods kinoko_map_render_layer_methods_storage;

extern struct SqratObjectMethods kinoko_sqrat_object_methods_storage;

extern struct SqratRootMethods kinoko_sqrat_root_methods_storage;

extern struct RendererMethods kinoko_renderer_methods_storage;

extern struct ActScriptMethods kinoko_act_script_methods_storage;

extern struct ActLayerReferenceMethods kinoko_act_layer_reference_methods_storage;

extern struct ActLayerMethods kinoko_act_layer_methods_storage;

extern struct ActLayerLayoutMethods kinoko_act_layer_layout_methods_storage;

extern struct ActKeyMethods kinoko_act_key_methods_storage;

extern struct ActDocumentMethods kinoko_act_document_methods_storage;

extern struct ActLayoutMethods kinoko_act_layout_methods_storage;

extern struct ChipResourceMethods kinoko_chip_resource_methods_storage;

extern struct MapLayoutMethods kinoko_map_layout_methods_storage;

extern struct QuadColorMethods kinoko_map_color_methods_storage;

extern struct TextureResourceMethods kinoko_texture_resource_methods_storage;

extern struct RenderTargetMethods kinoko_render_target_methods_storage;

extern struct SpriteMethods kinoko_sprite_methods_storage;

int32_t kinoko_host_register_act_script_abi(int32_t script, int32_t environment);

int32_t kinoko_host_construct_layer_abi(int32_t storage);

int32_t kinoko_collision_refresh_abi(int32_t this_ptr);

int32_t kinoko_collision_reset_abi(int32_t this_ptr, int32_t actor_ptr);

int32_t kinoko_host_register_collision_map_abi(int32_t layout);

int32_t kinoko_host_find_map_layout_abi(int32_t name_ptr);

int32_t kinoko_host_create_map_layer_abi(int32_t name_ptr);

int32_t kinoko_compile_file_native(int32_t vm);

int32_t kinoko_script_bind_root_value(int32_t *object, int32_t *value, char *name, int32_t flags);

int32_t kinoko_script_bind_root_integer(int32_t *object, int32_t value, char *name);

__declspec(noinline) int32_t kinoko_stack_vm(void);

int32_t kinoko_host_get_delegate_abi(int32_t source_ptr, int32_t *target_ptr);

int32_t kinoko_native_void_type(void);

int32_t kinoko_host_create_native_instance_abi(int32_t vm, int32_t class_name,
                        int32_t native_pointer, int32_t release_hook);

int32_t kinoko_csv_load_bytes(const char *path, char **bytes);

extern int32_t (__fastcall *kinoko_color_methods_storage)(int32_t, void*, char);

extern int32_t (__fastcall *kinoko_chip_quad_methods_storage)(int32_t, void*, char);

extern int32_t (__fastcall *kinoko_actor_pool_base_methods_storage)(int32_t, void*, unsigned char);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_host_explicit_vm(void);
#ifdef __cplusplus
}
#endif
