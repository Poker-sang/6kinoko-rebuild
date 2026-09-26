struct KinokoActor;
struct KinokoActorPool;
struct KinokoActorManager;
struct KinokoCamera;
#include "kinoko/file_io.h"
#include "kinoko/act_types.h"
struct SQVM;
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
int32_t __fastcall kinoko_method_register_texture_resource(int32_t receiver, void* unused_edx, struct SQVM* vm);
int32_t __fastcall kinoko_method_register_render_target(int32_t receiver, void* unused_edx, struct SQVM* vm);
int32_t __fastcall kinoko_method_load_resource_texture(KinokoActResource* receiver, void* unused_edx, const char* prefix);
int32_t __fastcall kinoko_method_unload_resource_texture(KinokoActResource* receiver, void* unused_edx);
int32_t __fastcall kinoko_method_load_chip_resource(KinokoActResource* receiver, void* unused_edx, const char* prefix);

int32_t __fastcall kinoko_method_resource_42f6c0(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
int32_t __fastcall kinoko_method_resource_42f800(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
int32_t __fastcall kinoko_method_resource_4467e0(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
int32_t __fastcall kinoko_method_resource_446920(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
int32_t __fastcall kinoko_method_resource_449860(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);
int32_t __fastcall kinoko_method_resource_4499a0(KinokoActResource* receiver, void* unused_edx, void* object, const char* name);

int32_t __fastcall kinoko_method_register_layout(KinokoActLayout* receiver, void* unused_edx);
int32_t __fastcall kinoko_method_register_map_layout(KinokoActLayout* receiver, void* unused_edx);

int32_t __fastcall kinoko_method_delete_act_script(void* receiver, void* unused_edx);
int32_t __fastcall kinoko_method_register_act_layer(KinokoActLayer* receiver, void* unused_edx, void* parent, int32_t flags);
int32_t __fastcall kinoko_method_read_act_script(void* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_act_script(void* receiver, void* unused_edx, KinokoArchiveReader* writer);
int32_t __fastcall kinoko_method_write_act_layer(KinokoActLayer* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_write_act_key(KinokoActKey* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_write_string_layout(KinokoStringLayout* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_read_texture_resource(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_texture_resource(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_read_render_target(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_render_target(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_read_chip_resource(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_chip_resource(KinokoActResource* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_read_layout_properties(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_read_act_layer(KinokoActLayer* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_read_act_key(KinokoActKey* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_read_act(KinokoActDocument* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_act(KinokoActDocument* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_query_serializable(void* receiver, void* unused_edx, const void* type, void* output);
int32_t __fastcall kinoko_method_destroy_serializable(void* receiver, void* unused_edx);
void* __fastcall kinoko_method_delete_act_key(KinokoActKey* receiver, void* unused_edx, unsigned char flags);
void* __fastcall kinoko_method_delete_act_layer(KinokoActLayer* receiver, void* unused_edx, unsigned char flags);
void* __fastcall kinoko_method_delete_act_resource(KinokoActResource* receiver, void* unused_edx, unsigned char flags);
void* __fastcall kinoko_method_destroy_layout(KinokoActLayout* receiver, void* unused_edx);
int32_t __fastcall kinoko_method_write_layout_properties(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_read_map_layout(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_write_map_layout(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader*  writer);
int32_t __fastcall kinoko_method_map_set_layer(KinokoActLayout* receiver, void* unused_edx, KinokoActLayer* layer);
KinokoActLayout* __fastcall kinoko_method_clone_c2d_layout(KinokoActLayout* receiver, void* unused_edx);
KinokoActKey* __fastcall kinoko_method_clone_act_key(KinokoActKey* receiver, void* unused_edx);
KinokoActLayer* __fastcall kinoko_method_clone_act_layer(KinokoActLayer* receiver, void* unused_edx);
KinokoActResource* __fastcall kinoko_method_clone_chip_resource(KinokoActResource* receiver, void* unused_edx);
KinokoActResource* __fastcall kinoko_method_clone_texture_resource(KinokoActResource* receiver, void* unused_edx);
KinokoActResource* __fastcall kinoko_method_clone_render_target(KinokoActResource* receiver, void* unused_edx);

// kinoko_register_chip_resource_class
int32_t __fastcall kinoko_method_register_chip_resource(int32_t receiver, void* unused_edx, struct SQVM* vm);

// retdec_cact_destructor_bridge
void* __fastcall kinoko_method_destroy_act(KinokoActDocument* receiver, void* unused_edx, unsigned char flags);
// function_42bcc0
int32_t __fastcall kinoko_method_layout_set_layer(KinokoActLayout* receiver, void* unused_edx, KinokoActLayer* layer);
// function_42c100
int32_t __fastcall kinoko_method_layout_update(KinokoActLayout* receiver, void* unused_edx);
// function_42c300
int32_t __fastcall kinoko_method_layout_draw(KinokoActLayout* receiver, void* unused_edx, float x, float y);
// function_43c860
int32_t __fastcall kinoko_method_layout3d_assign(KinokoActLayout* receiver, void* unused_edx, KinokoArchiveReader** source,
    int32_t mode);
// function_450950
int32_t __fastcall kinoko_method_begin_stage(KinokoActRuntime* receiver, void* unused_edx, int32_t stage);
// function_450e30
int32_t __fastcall kinoko_method_root_table_construct(KinokoActRuntime* receiver, void* unused_edx, struct SQVM* vm,
    void* output);
// function_4514a0
int32_t __fastcall kinoko_method_act_bitblt(KinokoActRuntime* receiver, void* unused_edx, int32_t x, int32_t y,
    int32_t width, int32_t height, KinokoActResource* resource, int32_t source_x, int32_t source_y, int32_t blend,
    float alpha);
// function_457a10
int32_t __fastcall kinoko_method_update_children(void* receiver, void* unused_edx, int32_t argument);

// function_45dbd0
int32_t __fastcall kinoko_method_actor_move(struct KinokoActor* receiver, void* unused_edx, float dx, float dy);
// function_45eb00
int32_t __fastcall kinoko_actor_reset_method(struct KinokoActor* receiver, void* unused_edx);
// function_466490
int32_t __fastcall kinoko_method_class_type(int32_t receiver, void* unused_edx);
// function_469620
int32_t __fastcall kinoko_method_render_layer_update(void* receiver, void* unused_edx, struct KinokoCamera* argument);
// function_46a6f0
int32_t __fastcall kinoko_method_actor_manager_remove(struct KinokoActorPool* receiver, void* unused_edx, uint32_t handle);
// retdec_actor_manager_vtable_push
int32_t __fastcall kinoko_method_actor_manager_push(struct KinokoActorManager* receiver, void* unused_edx);
// function_46ab10
int32_t __fastcall kinoko_method_actor_manager_top(struct KinokoActorPool* receiver, void* unused_edx, uint32_t* output);

#ifdef __cplusplus
}
#endif
