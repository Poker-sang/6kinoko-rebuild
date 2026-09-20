#pragma once
#include <stdint.h>

// Original Win32 virtual entry ABI: ECX is the receiver, EDX is unused,
// remaining parameters stay on the stack and are popped by the callee.
// The fastcall spelling deliberately reserves EDX; it must not be omitted.
// Legacy vtables store raw addresses with recovered C types. Invoke these
// entries through legacy_abi.h, never through those cdecl slot types.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error The recovered method entries require MSVC Win32.
#endif
#ifdef __cplusplus
extern "C" {
#endif

// Original 446520/4495A0/446BD0/447050 virtual resource entries.
int32_t __fastcall kinoko_method_register_texture_resource(int32_t receiver, void* unused_edx, int32_t vm);
int32_t __fastcall kinoko_method_register_render_target(int32_t receiver, void* unused_edx, int32_t vm);
int32_t __fastcall kinoko_method_load_resource_texture(int32_t receiver, void* unused_edx, const char* prefix);
int32_t __fastcall kinoko_method_unload_resource_texture(int32_t receiver, void* unused_edx);

int32_t __fastcall kinoko_method_resource_42f6c0(int32_t receiver, void* unused_edx, int32_t object, const char* name);
int32_t __fastcall kinoko_method_resource_42f800(int32_t receiver, void* unused_edx, int32_t object, const char* name);
int32_t __fastcall kinoko_method_resource_4467e0(int32_t receiver, void* unused_edx, int32_t object, const char* name);
int32_t __fastcall kinoko_method_resource_446920(int32_t receiver, void* unused_edx, int32_t object, const char* name);
int32_t __fastcall kinoko_method_resource_449860(int32_t receiver, void* unused_edx, int32_t object, const char* name);
int32_t __fastcall kinoko_method_resource_4499a0(int32_t receiver, void* unused_edx, int32_t object, const char* name);

int32_t __fastcall kinoko_method_register_layout(int32_t receiver, void* unused_edx);
int32_t __fastcall kinoko_method_register_map_layout(int32_t receiver, void* unused_edx);

int32_t __fastcall kinoko_method_delete_act_script(int32_t receiver, void* unused_edx);
int32_t __fastcall kinoko_method_delete_input_device(int32_t receiver, void* unused_edx, unsigned char flags);
int32_t __fastcall kinoko_method_register_act_layer(int32_t receiver, void* unused_edx, int32_t parent, int32_t flags);
int32_t __fastcall kinoko_method_read_act_script(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_act_script(int32_t receiver, void* unused_edx, int32_t writer);
int32_t __fastcall kinoko_method_read_texture_resource(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_texture_resource(int32_t receiver, void* unused_edx, int32_t writer);
int32_t __fastcall kinoko_method_read_render_target(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_render_target(int32_t receiver, void* unused_edx, int32_t writer);
int32_t __fastcall kinoko_method_read_chip_resource(int32_t receiver, void* unused_edx, int32_t reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_chip_resource(int32_t receiver, void* unused_edx, int32_t writer);

// function_42f350
int32_t __fastcall kinoko_method_register_chip_resource(int32_t receiver, void* unused_edx, int32_t vm);

// retdec_cact_destructor_bridge
int32_t __fastcall kinoko_method_destroy_act(int32_t receiver, void* unused_edx, unsigned char flags);
// function_42bcc0
int32_t __fastcall kinoko_method_layout_set_layer(int32_t receiver, void* unused_edx, int32_t layer);
// function_42c100
int32_t __fastcall kinoko_method_layout_update(int32_t receiver, void* unused_edx);
// function_42c300
int32_t __fastcall kinoko_method_layout_draw(int32_t receiver, void* unused_edx, float x, float y);
// function_43c860
int32_t __fastcall kinoko_method_layout3d_assign(int32_t receiver, void* unused_edx, int32_t source,
    int32_t mode);
// function_450950
int32_t __fastcall kinoko_method_begin_stage(int32_t receiver, void* unused_edx, int32_t stage);
// function_450e30
int32_t __fastcall kinoko_method_root_table_construct(int32_t receiver, void* unused_edx, int32_t vm,
    int32_t output);
// function_4514a0
int32_t __fastcall kinoko_method_act_bitblt(int32_t receiver, void* unused_edx, int32_t x, int32_t y,
    int32_t width, int32_t height, int32_t resource, int32_t source_x, int32_t source_y, int32_t blend,
    float alpha);
// function_457a10
int32_t __fastcall kinoko_method_update_children(int32_t receiver, void* unused_edx, int32_t argument);
// function_45d970_bridge
int32_t __fastcall kinoko_method_destroy_actor(int32_t receiver, void* unused_edx, char flags);
// function_45dbd0
int32_t __fastcall kinoko_method_actor_move(int32_t receiver, void* unused_edx, float dx, float dy);
// function_45eb00
int32_t __fastcall kinoko_method_actor_destroy_state(int32_t receiver, void* unused_edx);
// function_466490
int32_t __fastcall kinoko_method_class_type(int32_t receiver, void* unused_edx);
// function_469620
int32_t __fastcall kinoko_method_render_layer_update(int32_t receiver, void* unused_edx, int32_t argument);
// function_46a6f0
int32_t __fastcall kinoko_method_actor_manager_remove(int32_t receiver, void* unused_edx, uint32_t handle);
// retdec_actor_manager_vtable_push
int32_t __fastcall kinoko_method_actor_manager_push(int32_t receiver, void* unused_edx);
// function_46ab10
int32_t __fastcall kinoko_method_actor_manager_top(int32_t receiver, void* unused_edx, int32_t output);

#ifdef __cplusplus
}
#endif
