// Original Win32 virtual-table identities with exact target signatures.
// Pointer initializers remain link-time relocations; no integer-address initialization.
#include "runtime_host_internal.h"

int32_t (__fastcall *kinoko_color_methods_storage)(int32_t, void*, char) = kinoko_color_destroy;

int32_t (__fastcall *kinoko_chip_quad_methods_storage)(int32_t, void*, char) = kinoko_color_destroy;

int32_t (__fastcall *kinoko_actor_pool_base_methods_storage)(int32_t, void*, unsigned char) = kinoko_method_actor_pool_base_delete;

struct QuadColorMethods kinoko_layout_color_methods_storage = {
    kinoko_delete_layout_sprite,
    kinoko_quad_set_color,
    kinoko_quad_set_vertex_colors,
    kinoko_quad_modulate_color
};

struct StringLayoutMethods kinoko_string_layout_methods_storage = {
    kinoko_method_write_string_layout,
    kinoko_method_read_string_layout,
    kinoko_method_query_serializable,
    kinoko_method_destroy_string_layout,
    kinoko_method_string_layout_type,
    kinoko_method_clone_string_layout,
    kinoko_method_set_string_layer,
    kinoko_method_update_string_layout,
    kinoko_method_draw_string_layout,
    kinoko_method_register_string_layout,
    kinoko_method_delete_string_layout
};

struct SquirrelObjectMethods kinoko_squirrel_object_methods_storage = {
    kinoko_squirrel_object_delete
};

struct ActorMethods kinoko_actor_methods_storage = {
    kinoko_actor_delete_method
};

struct ActorRenderLayerMethods kinoko_actor_render_layer_methods_storage = {
    kinoko_method_render_layer_update
};

struct ActorPoolMethods kinoko_actor_pool_methods_storage = {
    kinoko_method_actor_pool_delete,
    kinoko_method_actor_manager_top,
    kinoko_method_actor_manager_remove,
    kinoko_method_lookup_actor,
    kinoko_method_actor_pool_count
};

struct ActorOwnerMethods kinoko_actor_owner_methods_storage = {
    kinoko_method_actor_owner_delete,
    kinoko_method_actor_manager_push
};

struct MapRenderLayerMethods kinoko_map_render_layer_methods_storage = {
    kinoko_map_render_layer_entry
};

struct SqratObjectMethods kinoko_sqrat_object_methods_storage = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct SqratRootMethods kinoko_sqrat_root_methods_storage = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct RendererMethods kinoko_renderer_methods_storage = {
    kinoko_renderer_before_reset,
    kinoko_renderer_after_reset
};

struct ActScriptMethods kinoko_act_script_methods_storage = {
    kinoko_method_write_act_script,
    kinoko_method_read_act_script,
    kinoko_method_query_serializable,
    kinoko_method_delete_act_script
};

struct ActLayerReferenceMethods kinoko_act_layer_reference_methods_storage = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct ActLayerMethods kinoko_act_layer_methods_storage = {
    kinoko_method_write_act_layer,
    kinoko_method_read_act_layer,
    kinoko_method_query_serializable,
    kinoko_method_destroy_serializable,
    kinoko_method_delete_act_layer,
    kinoko_method_clone_act_layer,
    kinoko_act_layer_set_resource,
    kinoko_act_layer_world_position,
    kinoko_method_register_act_layer
};

struct ActLayerLayoutMethods kinoko_act_layer_layout_methods_storage = {
    kinoko_sqrat_delete_object,
    kinoko_sqrat_object_reference,
    kinoko_sqrat_copy_object
};

struct ActKeyMethods kinoko_act_key_methods_storage = {
    kinoko_method_write_act_key,
    kinoko_method_read_act_key,
    kinoko_method_query_serializable,
    kinoko_method_destroy_serializable,
    kinoko_method_delete_act_key,
    kinoko_method_clone_act_key
};

struct ActDocumentMethods kinoko_act_document_methods_storage = {
    kinoko_method_write_act,
    kinoko_method_read_act,
    kinoko_method_query_serializable,
    kinoko_method_destroy_serializable,
    kinoko_method_destroy_act,
    kinoko_act_clone,
    kinoko_method_load_act_resources,
    kinoko_method_suspend_act_resources,
    kinoko_method_resume_act_resources
};

struct ActLayoutMethods kinoko_act_layout_methods_storage = {
    kinoko_method_write_layout_properties,
    kinoko_method_read_layout_properties,
    kinoko_method_query_serializable,
    kinoko_method_destroy_layout,
    kinoko_c2d_layout_type,
    kinoko_method_clone_c2d_layout,
    kinoko_method_layout_set_layer,
    kinoko_method_layout_update,
    kinoko_method_layout_draw,
    kinoko_method_register_layout
};

struct ChipResourceMethods kinoko_chip_resource_methods_storage = {
    kinoko_method_write_chip_resource,
    kinoko_method_read_chip_resource,
    kinoko_method_query_serializable,
    kinoko_method_destroy_serializable,
    kinoko_method_delete_act_resource,
    kinoko_chip_resource_type,
    kinoko_method_register_chip_resource,
    kinoko_method_resource_42f800,
    kinoko_method_resource_42f6c0,
    kinoko_method_clone_chip_resource,
    kinoko_method_load_chip_resource
};

struct MapLayoutMethods kinoko_map_layout_methods_storage = {
    kinoko_method_write_map_layout,
    kinoko_method_read_map_layout,
    kinoko_method_query_serializable,
    kinoko_method_destroy_layout,
    kinoko_map_layout_type,
    kinoko_clone_map_layout,
    kinoko_method_map_set_layer,
    kinoko_map_update_all_entry,
    kinoko_map_draw_entry,
    kinoko_method_register_map_layout,
    kinoko_map_update_visible_entry
};

struct QuadColorMethods kinoko_map_color_methods_storage = {
    kinoko_delete_map_sprite,
    kinoko_quad_set_color,
    kinoko_quad_set_vertex_colors,
    kinoko_quad_modulate_color
};

struct TextureResourceMethods kinoko_texture_resource_methods_storage = {
    kinoko_method_write_texture_resource,
    kinoko_method_read_texture_resource,
    kinoko_method_query_serializable,
    kinoko_method_destroy_serializable,
    kinoko_method_delete_act_resource,
    kinoko_texture_resource_type,
    kinoko_method_register_texture_resource,
    kinoko_method_resource_446920,
    kinoko_method_resource_4467e0,
    kinoko_method_clone_texture_resource,
    kinoko_method_load_resource_texture,
    kinoko_method_unload_resource_texture
};

struct RenderTargetMethods kinoko_render_target_methods_storage = {
    kinoko_method_write_render_target,
    kinoko_method_read_render_target,
    kinoko_method_query_serializable,
    kinoko_method_destroy_serializable,
    kinoko_method_delete_act_resource,
    kinoko_render_target_type,
    kinoko_method_register_render_target,
    kinoko_method_resource_4499a0,
    kinoko_method_resource_449860,
    kinoko_method_clone_render_target,
    kinoko_method_load_resource_texture,
    kinoko_method_unload_resource_texture,
    kinoko_method_create_render_target
};

struct SpriteMethods kinoko_sprite_methods_storage = {
    kinoko_color_destroy,
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

const void *kinoko_string_layout_methods(void) { return &kinoko_string_layout_methods_storage; }

const void *kinoko_actor_pool_methods(void) { return &kinoko_actor_pool_methods_storage; }

const void *kinoko_actor_pool_base_methods(void) { return &kinoko_actor_pool_base_methods_storage; }
static_assert(sizeof(QuadColorMethods) == 4 * sizeof(void*));
static_assert(sizeof(StringLayoutMethods) == 11 * sizeof(void*));
static_assert(sizeof(SquirrelObjectMethods) == 1 * sizeof(void*));
static_assert(sizeof(ActorMethods) == 1 * sizeof(void*));
static_assert(sizeof(ActorRenderLayerMethods) == 1 * sizeof(void*));
static_assert(sizeof(ActorPoolMethods) == 5 * sizeof(void*));
static_assert(sizeof(ActorOwnerMethods) == 2 * sizeof(void*));
static_assert(sizeof(MapRenderLayerMethods) == 1 * sizeof(void*));
static_assert(sizeof(SqratObjectMethods) == 3 * sizeof(void*));
static_assert(sizeof(SqratRootMethods) == 3 * sizeof(void*));
static_assert(sizeof(RendererMethods) == 2 * sizeof(void*));
static_assert(sizeof(ActScriptMethods) == 4 * sizeof(void*));
static_assert(sizeof(ActLayerReferenceMethods) == 3 * sizeof(void*));
static_assert(sizeof(ActLayerMethods) == 9 * sizeof(void*));
static_assert(sizeof(ActLayerLayoutMethods) == 3 * sizeof(void*));
static_assert(sizeof(ActKeyMethods) == 6 * sizeof(void*));
static_assert(sizeof(ActDocumentMethods) == 9 * sizeof(void*));
static_assert(sizeof(ActLayoutMethods) == 10 * sizeof(void*));
static_assert(sizeof(ChipResourceMethods) == 11 * sizeof(void*));
static_assert(sizeof(MapLayoutMethods) == 11 * sizeof(void*));
static_assert(sizeof(QuadColorMethods) == 4 * sizeof(void*));
static_assert(sizeof(TextureResourceMethods) == 12 * sizeof(void*));
static_assert(sizeof(RenderTargetMethods) == 13 * sizeof(void*));
static_assert(sizeof(SpriteMethods) == 10 * sizeof(void*));
