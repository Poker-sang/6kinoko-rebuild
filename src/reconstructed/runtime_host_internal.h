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

int32_t *kinoko_c2d_layout_type(void);
int32_t *kinoko_chip_resource_type(void);
int32_t *kinoko_map_layout_type(void);
int32_t *kinoko_texture_resource_type(void);
int32_t *kinoko_render_target_type(void);
void* __fastcall kinoko_color_destroy(void* receiver, void* unused_edx, char flags);

#ifdef __cplusplus
struct SquirrelObjectMethods {
    decltype(&kinoko_squirrel_object_delete) destroy;
};
#else
struct SquirrelObjectMethods {
    void* (__fastcall *destroy)(void* object, void *unused,
                                                int32_t flags);
};
#endif

#ifdef __cplusplus
struct ActorMethods {
    decltype(&kinoko_actor_delete_method) destroy;
};
#else
struct ActorMethods {
    KinokoActor * (__fastcall *destroy)(KinokoActor *actor, void* unused_edx, unsigned char flags);
};
#endif

#ifdef __cplusplus
struct ActorRenderLayerMethods {
    decltype(&kinoko_method_render_layer_update) update;
};
#else
struct ActorRenderLayerMethods {
    int32_t (__fastcall *update)(void* receiver, void* unused_edx, struct KinokoCamera* argument);
};
#endif

#ifdef __cplusplus
struct ActorPoolMethods {
    decltype(&kinoko_method_actor_pool_delete) destroy;
    decltype(&kinoko_method_actor_manager_top) top;
    decltype(&kinoko_method_actor_manager_remove) remove;
    decltype(&kinoko_method_lookup_actor) lookup;
    decltype(&kinoko_method_actor_pool_count) count;
};
#else
struct ActorPoolMethods {
    KinokoActorPool* (__fastcall *destroy)(KinokoActorPool* manager, void *unused, unsigned char flags);
    struct KinokoActor* (__fastcall *top)(struct KinokoActorPool* receiver, void* unused_edx, uint32_t* output);
    int32_t (__fastcall *remove)(struct KinokoActorPool* receiver, void* unused_edx, uint32_t handle);
    KinokoActor* (__fastcall *lookup)(KinokoActorPool* manager, void *unused, uint32_t handle);
    int32_t (__fastcall *count)(KinokoActorPool* manager, void *unused);
};
#endif

#ifdef __cplusplus
struct ActorOwnerMethods {
    decltype(&kinoko_method_actor_owner_delete) destroy;
    decltype(&kinoko_method_actor_manager_push) push;
};
#else
struct ActorOwnerMethods {
    KinokoActorManager* (__fastcall *destroy)(KinokoActorManager* manager, void *unused, unsigned char flags);
    struct KinokoActor* (__fastcall *push)(struct KinokoActorManager* receiver, void* unused_edx);
};
#endif

#ifdef __cplusplus
struct MapRenderLayerMethods {
    decltype(&kinoko_map_render_layer_entry) update;
};
#else
struct MapRenderLayerMethods {
    int32_t (__fastcall *update)(KinokoRenderLayer* layer, void *unused,
    KinokoCamera* camera);
};
#endif

#ifdef __cplusplus
struct SqratObjectMethods {
    decltype(&kinoko_sqrat_delete_object) destroy;
    decltype(&kinoko_sqrat_object_reference) reference;
    decltype(&kinoko_sqrat_copy_object) copy;
};
#else
struct SqratObjectMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};
#endif

#ifdef __cplusplus
struct SqratRootMethods {
    decltype(&kinoko_sqrat_delete_object) destroy;
    decltype(&kinoko_sqrat_object_reference) reference;
    decltype(&kinoko_sqrat_copy_object) copy;
};
#else
struct SqratRootMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};
#endif

#ifdef __cplusplus
struct RendererMethods {
    decltype(&kinoko_renderer_before_reset) before_reset;
    decltype(&kinoko_renderer_after_reset) after_reset;
};
#else
struct RendererMethods {
    int32_t (__fastcall *before_reset)(KinokoRenderer *object, void *unused);
    int32_t (__fastcall *after_reset)(KinokoRenderer *object, void *unused);
};
#endif

#ifdef __cplusplus
struct ActScriptMethods {
    decltype(&kinoko_method_write_act_script) write;
    decltype(&kinoko_method_read_act_script) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_delete_act_script) destroy;
};
#else
struct ActScriptMethods {
    int32_t (__fastcall *write)(void* receiver, void* unused_edx, KinokoArchiveReader* writer);
    int32_t (__fastcall *read)(void* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    int32_t (__fastcall *destroy)(void* receiver, void* unused_edx);
};
#endif

#ifdef __cplusplus
struct ActLayerReferenceMethods {
    decltype(&kinoko_sqrat_delete_object) destroy;
    decltype(&kinoko_sqrat_object_reference) reference;
    decltype(&kinoko_sqrat_copy_object) copy;
};
#else
struct ActLayerReferenceMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};
#endif

#ifdef __cplusplus
struct ActLayerMethods {
    decltype(&kinoko_method_write_act_layer) write;
    decltype(&kinoko_method_read_act_layer) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_serializable) destroy;
    decltype(&kinoko_method_delete_act_layer) delete_object;
    decltype(&kinoko_method_clone_act_layer) clone;
    decltype(&kinoko_act_layer_set_resource) associate;
    decltype(&kinoko_act_layer_world_position) world_position;
    decltype(&kinoko_method_register_act_layer) register_class;
};
#else
struct ActLayerMethods {
    int32_t (__fastcall *write)(KinokoActLayer* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActLayer* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    int32_t (__fastcall *destroy)(void* receiver, void* unused_edx);
    void* (__fastcall *delete_object)(KinokoActLayer* receiver, void* unused_edx, unsigned char flags);
    KinokoActLayer* (__fastcall *clone)(KinokoActLayer* receiver, void* unused_edx);
    int32_t (__fastcall *associate)(
    KinokoActLayer *layer, void *unused, KinokoActResource *resource);
    KinokoActLayer * (__fastcall *world_position)(KinokoActLayer *layer,
    void *unused, float *x, float *y, float *z);
    int32_t (__fastcall *register_class)(KinokoActLayer* receiver, void* unused_edx, void* parent, int32_t flags);
};
#endif

#ifdef __cplusplus
struct ActLayerLayoutMethods {
    decltype(&kinoko_sqrat_delete_object) destroy;
    decltype(&kinoko_sqrat_object_reference) reference;
    decltype(&kinoko_sqrat_copy_object) copy;
};
#else
struct ActLayerLayoutMethods {
    void * (__fastcall *destroy)(void * receiver, void *unused, int32_t flags);
    void * (__fastcall *reference)(void * receiver, void *unused);
    void * (__fastcall *copy)(void * receiver, void *unused, void * output);
};
#endif

#ifdef __cplusplus
struct ActKeyMethods {
    decltype(&kinoko_method_write_act_key) write;
    decltype(&kinoko_method_read_act_key) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_serializable) destroy;
    decltype(&kinoko_method_delete_act_key) delete_object;
    decltype(&kinoko_method_clone_act_key) clone;
};
#else
struct ActKeyMethods {
    int32_t (__fastcall *write)(KinokoActKey* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActKey* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    int32_t (__fastcall *destroy)(void* receiver, void* unused_edx);
    void* (__fastcall *delete_object)(KinokoActKey* receiver, void* unused_edx, unsigned char flags);
    KinokoActKey* (__fastcall *clone)(KinokoActKey* receiver, void* unused_edx);
};
#endif

#ifdef __cplusplus
struct ActDocumentMethods {
    decltype(&kinoko_method_write_act) write;
    decltype(&kinoko_method_read_act) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_serializable) destroy;
    decltype(&kinoko_method_destroy_act) delete_object;
    decltype(&kinoko_act_clone) clone;
    decltype(&kinoko_method_load_act_resources) load_resources;
    decltype(&kinoko_method_suspend_act_resources) suspend_resources;
    decltype(&kinoko_method_resume_act_resources) resume_resources;
};
#else
struct ActDocumentMethods {
    int32_t (__fastcall *write)(KinokoActDocument* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActDocument* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    int32_t (__fastcall *destroy)(void* receiver, void* unused_edx);
    void* (__fastcall *delete_object)(KinokoActDocument* receiver, void* unused_edx, unsigned char flags);
    KinokoActDocument * (__fastcall *clone)(KinokoActDocument *source, void *unused);
    int32_t (__fastcall *load_resources)(KinokoActDocument *document, void *unused, const char *prefix);
    int32_t (__fastcall *suspend_resources)(KinokoActDocument *document, void *unused);
    int32_t (__fastcall *resume_resources)(KinokoActDocument *document, void *unused);
};
#endif

#ifdef __cplusplus
struct ActLayoutMethods {
    decltype(&kinoko_method_write_layout_properties) write;
    decltype(&kinoko_method_read_layout_properties) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_layout) destroy;
    decltype(&kinoko_c2d_layout_type) type;
    decltype(&kinoko_method_clone_c2d_layout) clone;
    decltype(&kinoko_method_layout_set_layer) associate;
    decltype(&kinoko_method_layout_update) update;
    decltype(&kinoko_method_layout_draw) draw;
    decltype(&kinoko_method_register_layout) register_class;
};
#else
struct ActLayoutMethods {
    int32_t (__fastcall *write)(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    void* (__fastcall *destroy)(KinokoActLayout* receiver, void* unused_edx);
    int32_t * (*type)(void);
    KinokoActLayout* (__fastcall *clone)(KinokoActLayout* receiver, void* unused_edx);
    int32_t (__fastcall *associate)(KinokoActLayout* receiver, void* unused_edx, KinokoActLayer* layer);
    int32_t (__fastcall *update)(KinokoActLayout* receiver, void* unused_edx);
    int32_t (__fastcall *draw)(KinokoActLayout* receiver, void* unused_edx, float x, float y);
    int32_t (__fastcall *register_class)(KinokoActLayout* receiver, void* unused_edx);
};
#endif

#ifdef __cplusplus
struct ChipResourceMethods {
    decltype(&kinoko_method_write_chip_resource) write;
    decltype(&kinoko_method_read_chip_resource) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_serializable) destroy;
    decltype(&kinoko_method_delete_act_resource) delete_object;
    decltype(&kinoko_chip_resource_type) type;
    decltype(&kinoko_method_register_chip_resource) register_class;
    decltype(&kinoko_method_resource_42f800) resource_42f800;
    decltype(&kinoko_method_resource_42f6c0) resource_42f6c0;
    decltype(&kinoko_method_clone_chip_resource) clone;
    decltype(&kinoko_method_load_chip_resource) load;
};
#else
struct ChipResourceMethods {
    int32_t (__fastcall *write)(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    int32_t (__fastcall *destroy)(void* receiver, void* unused_edx);
    void* (__fastcall *delete_object)(KinokoActResource* receiver, void* unused_edx, unsigned char flags);
    int32_t * (*type)(void);
    int32_t (__fastcall *register_class)(void* receiver, void* unused_edx, struct SQVM* vm);
    int32_t (__fastcall *resource_42f800)(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
    int32_t (__fastcall *resource_42f6c0)(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
    KinokoActResource* (__fastcall *clone)(KinokoActResource* receiver, void* unused_edx);
    int32_t (__fastcall *load)(KinokoActResource* receiver, void* unused_edx, const char* prefix);
};
#endif

#ifdef __cplusplus
struct MapLayoutMethods {
    decltype(&kinoko_method_write_map_layout) write;
    decltype(&kinoko_method_read_map_layout) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_layout) destroy;
    decltype(&kinoko_map_layout_type) type;
    decltype(&kinoko_clone_map_layout) clone;
    decltype(&kinoko_method_map_set_layer) associate;
    decltype(&kinoko_map_update_all_entry) update;
    decltype(&kinoko_map_draw_entry) draw;
    decltype(&kinoko_method_register_map_layout) register_class;
    decltype(&kinoko_map_update_visible_entry) update_visible;
};
#else
struct MapLayoutMethods {
    int32_t (__fastcall *write)(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    void* (__fastcall *destroy)(KinokoActLayout* receiver, void* unused_edx);
    int32_t * (*type)(void);
    KinokoActLayout* (__fastcall *clone)(KinokoActLayout* source, void *unused);
    int32_t (__fastcall *associate)(KinokoActLayout* receiver, void* unused_edx, KinokoActLayer* layer);
    int32_t (__fastcall *update)(KinokoActLayout* layout, void *unused);
    int32_t (__fastcall *draw)(KinokoActLayout* layout, void *unused,
    float x, float y);
    int32_t (__fastcall *register_class)(KinokoActLayout* receiver, void* unused_edx);
    int32_t (__fastcall *update_visible)(KinokoActLayout* layout, void *unused,
    int32_t left, int32_t top, int32_t right, int32_t bottom);
};
#endif

#ifdef __cplusplus
struct QuadColorMethods {
    decltype(&kinoko_delete_layout_sprite) destroy;
    decltype(&kinoko_quad_set_color) set_color;
    decltype(&kinoko_quad_set_vertex_colors) set_vertex_colors;
    decltype(&kinoko_quad_modulate_color) modulate_color;
};
#else
struct QuadColorMethods {
    void* (__fastcall *destroy)(void* sprite, void *unused, int32_t flags);
    uint32_t (__fastcall *set_color)(KinokoColoredQuad *quad, void *unused, uint32_t color);
    uint32_t (__fastcall *set_vertex_colors)(KinokoColoredQuad *quad, void *unused, const uint32_t *colors);
    uint32_t (__fastcall *modulate_color)(KinokoColoredQuad *quad, void *unused, uint32_t color);
};
#endif

#ifdef __cplusplus
struct TextureResourceMethods {
    decltype(&kinoko_method_write_texture_resource) write;
    decltype(&kinoko_method_read_texture_resource) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_serializable) destroy;
    decltype(&kinoko_method_delete_act_resource) delete_object;
    decltype(&kinoko_texture_resource_type) type;
    decltype(&kinoko_method_register_texture_resource) register_class;
    decltype(&kinoko_method_resource_446920) resource_446920;
    decltype(&kinoko_method_resource_4467e0) resource_4467e0;
    decltype(&kinoko_method_clone_texture_resource) clone;
    decltype(&kinoko_method_load_resource_texture) load;
    decltype(&kinoko_method_unload_resource_texture) unload;
};
#else
struct TextureResourceMethods {
    int32_t (__fastcall *write)(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    int32_t (__fastcall *destroy)(void* receiver, void* unused_edx);
    void* (__fastcall *delete_object)(KinokoActResource* receiver, void* unused_edx, unsigned char flags);
    int32_t * (*type)(void);
    int32_t (__fastcall *register_class)(void* receiver, void* unused_edx, struct SQVM* vm);
    int32_t (__fastcall *resource_446920)(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
    int32_t (__fastcall *resource_4467e0)(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
    KinokoActResource* (__fastcall *clone)(KinokoActResource* receiver, void* unused_edx);
    int32_t (__fastcall *load)(KinokoActResource* receiver, void* unused_edx, const char* prefix);
    int32_t (__fastcall *unload)(KinokoActResource* receiver, void* unused_edx);
};
#endif

#ifdef __cplusplus
struct RenderTargetMethods {
    decltype(&kinoko_method_write_render_target) write;
    decltype(&kinoko_method_read_render_target) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_serializable) destroy;
    decltype(&kinoko_method_delete_act_resource) delete_object;
    decltype(&kinoko_render_target_type) type;
    decltype(&kinoko_method_register_render_target) register_class;
    decltype(&kinoko_method_resource_4499a0) resource_4499a0;
    decltype(&kinoko_method_resource_449860) resource_449860;
    decltype(&kinoko_method_clone_render_target) clone;
    decltype(&kinoko_method_load_resource_texture) load;
    decltype(&kinoko_method_unload_resource_texture) unload;
    decltype(&kinoko_method_create_render_target) create;
};
#else
struct RenderTargetMethods {
    int32_t (__fastcall *write)(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    int32_t (__fastcall *destroy)(void* receiver, void* unused_edx);
    void* (__fastcall *delete_object)(KinokoActResource* receiver, void* unused_edx, unsigned char flags);
    int32_t * (*type)(void);
    int32_t (__fastcall *register_class)(void* receiver, void* unused_edx, struct SQVM* vm);
    int32_t (__fastcall *resource_4499a0)(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
    int32_t (__fastcall *resource_449860)(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
    KinokoActResource* (__fastcall *clone)(KinokoActResource* receiver, void* unused_edx);
    int32_t (__fastcall *load)(KinokoActResource* receiver, void* unused_edx, const char* prefix);
    int32_t (__fastcall *unload)(KinokoActResource* receiver, void* unused_edx);
    int32_t (__fastcall *create)(KinokoActResource *resource, void *unused,
                                                      int32_t width, int32_t height);
};
#endif

#ifdef __cplusplus
struct SpriteMethods {
    decltype(&kinoko_color_destroy) destroy;
    decltype(&kinoko_quad_set_color) set_color;
    decltype(&kinoko_quad_set_vertex_colors) set_vertex_colors;
    decltype(&kinoko_quad_modulate_color) modulate_color;
    decltype(&kinoko_sprite_set_rect_pivot) set_rect_pivot;
    decltype(&kinoko_sprite_set_rect) set_rect;
    decltype(&kinoko_sprite_draw_bounds) draw_bounds;
    decltype(&kinoko_sprite_draw_404770) draw_404770;
    decltype(&kinoko_sprite_draw_404bc0) draw_404bc0;
    decltype(&kinoko_sprite_draw_4049c0) draw_4049c0;
};
#else
struct SpriteMethods {
    void* (__fastcall *destroy)(void* receiver, void* unused_edx, char flags);
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
#endif

void kinoko_host_free_allocation(int32_t * a1);



int32_t kinoko_register_cact_layer_class(struct SQVM* a1);



int32_t kinoko_register_c2dlayout_class(struct SQVM* a1);



int32_t kinoko_register_chip_resource_class(struct SQVM* a1);



int32_t kinoko_register_map_layout_class(struct SQVM* a1);

int32_t kinoko_script_dprint_noop(void);



int32_t kinoko_register_texture_resource_class(struct SQVM* a1);



int32_t kinoko_register_render_target_class(struct SQVM* a1);



int32_t kinoko_actor_register_script_class(void);

int32_t kinoko_host_clear_stages(void);

int32_t kinoko_host_initialize_camera(void);

void kinoko_camera_class_copy(void* a1, void* a2);

int32_t kinoko_register_camera_binding(void);


int32_t kinoko_host_append_render_item(int32_t * a1);


int32_t kinoko_register_map_binding(void);


int32_t kinoko_host_clear_sound(void);

int32_t kinoko_host_show_message_abi(int32_t a1);

int32_t kinoko_host_sleep_abi(int32_t dwMilliseconds);

int32_t kinoko_host_milliseconds(void);

int32_t kinoko_host_close_window(void);

int32_t kinoko_script_bind_root_value(int32_t * a1, int32_t * a2, char * a3, int32_t a4);

int32_t kinoko_script_bind_root_integer(int32_t * a1, int32_t a2, char * a3);





int32_t kinoko_host_create_native_instance(struct SQVM* a1, const char* a2, void* a3, SQRELEASEHOOK a4);

int32_t kinoko_host_destroy_global_callback(void);

extern const char * kinoko_application_error_text;

extern const char * kinoko_application_title_text;

extern const char * kinoko_audio_error_text;

extern struct QuadColorMethods kinoko_layout_color_methods_storage;

#ifdef __cplusplus
struct StringLayoutMethods {
    decltype(&kinoko_method_write_string_layout) write;
    decltype(&kinoko_method_read_string_layout) read;
    decltype(&kinoko_method_query_serializable) query;
    decltype(&kinoko_method_destroy_string_layout) destroy;
    decltype(&kinoko_method_string_layout_type) type;
    decltype(&kinoko_method_clone_string_layout) clone;
    decltype(&kinoko_method_set_string_layer) set_layer;
    decltype(&kinoko_method_update_string_layout) update;
    decltype(&kinoko_method_draw_string_layout) draw;
    decltype(&kinoko_method_register_string_layout) register_class;
    decltype(&kinoko_method_delete_string_layout) delete_object;
};
#else
struct StringLayoutMethods {
    int32_t (__fastcall *write)(KinokoStringLayout* receiver, void* unused_edx, KinokoArchiveReader*  writer);
    int32_t (__fastcall *read)(KinokoStringLayout* object, void *unused, KinokoArchiveReader** holder, int32_t version);
    int32_t (__fastcall *query)(void* receiver, void* unused_edx, const void* type, void* output);
    KinokoStringLayout* (__fastcall *destroy)(KinokoStringLayout* object, void *unused);
    int32_t (__fastcall *type)(KinokoStringLayout* object, void *unused);
    KinokoStringLayout* (__fastcall *clone)(KinokoStringLayout* object, void *unused);
    int32_t (__fastcall *set_layer)(KinokoStringLayout* object, void *unused, KinokoActLayer* layer);
    int32_t (__fastcall *update)(KinokoStringLayout* object, void *unused);
    int32_t (__fastcall *draw)(KinokoStringLayout* object, void *unused, float x, float y);
    int32_t (__fastcall *register_class)(KinokoStringLayout* object, void *unused);
    KinokoStringLayout* (__fastcall *delete_object)(KinokoStringLayout* object, void *unused, unsigned char flags);
};
#endif
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








int32_t kinoko_compile_file_native(struct SQVM* vm);

int32_t kinoko_script_bind_root_value(int32_t *object, int32_t *value, char *name, int32_t flags);

int32_t kinoko_script_bind_root_integer(int32_t *object, int32_t value, char *name);

__declspec(noinline) int32_t kinoko_stack_vm(void);


int32_t kinoko_native_void_type(void);

int32_t kinoko_host_create_native_instance(struct SQVM* vm, const char* class_name,
                        void* native_pointer, SQRELEASEHOOK release_hook);

int32_t kinoko_csv_load_bytes(const char *path, char **bytes);

extern void* (__fastcall *kinoko_color_methods_storage)(void*, void*, char);

extern void* (__fastcall *kinoko_chip_quad_methods_storage)(void*, void*, char);

extern KinokoActorPool* (__fastcall *kinoko_actor_pool_base_methods_storage)(KinokoActorPool*, void*, unsigned char);

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
