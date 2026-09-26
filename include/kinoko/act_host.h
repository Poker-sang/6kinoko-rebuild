#include "kinoko/act_types.h"
#include "kinoko/angle_math.h"
#include "kinoko/script_file.h"
#include "kinoko/file_io_legacy.h"
#include "kinoko/graphics_device.h"
#pragma once
#include "kinoko/legacy_string.h"
#include <stdint.h>
#include <stddef.h>

/* Temporary host ports. These preserve original object identities and I/O;
 * the C++ ACT implementation does not choose another resource directory. */
struct KinokoActHostSymbols {
    const void* script_vtable;
    const void* layer_ref_vtable;
    const void* layer_vtable;
    const void* layer_layout_vtable;
    const void* key_vtable;
    const void* act_vtable;
    const void* layout_vtable;
    const void* layout_sprite_vtable;
    const void* chip_resource_vtable;
    const void* map_layout_vtable;
    const void* map_view_vtable;
    const void* texture_resource_vtable;
    const void* sq_object_vtable;
    const void* sq_root_vtable;
    const void* sprite_vtable;
    const void* color_vtable;
    const void* render_target_vtable;
    const void* chip_quad_vtable;
};

#ifdef __cplusplus
extern "C" {
#endif
const struct KinokoActHostSymbols* kinoko_act_host_symbols(void);
int32_t kinoko_act_script_output_compiled(void);
extern char kinoko_resource2d_class_published;
extern int32_t kinoko_acting_player_class_pair[2];
extern int32_t kinoko_resource2d_class_pair[2];
extern int32_t kinoko_layout_set_pair[2];
extern int32_t kinoko_layout_get_pair[2];
extern int32_t kinoko_layout_class_pair[2];
extern int32_t kinoko_layer_set_pair[2];
extern int32_t kinoko_layer_get_pair[2];
/* The established 28-byte host storage includes a 24-byte string record and
   one reserved word. It is not a modern std::string or an owning C++ wrapper. */
typedef struct KinokoScriptExtension {
    unsigned char characters[16];
    uint32_t length, capacity, reserved;
} KinokoScriptExtension;
extern KinokoScriptExtension kinoko_act_script_extension;
extern int32_t  kinoko_script_void_result_identity;
extern int32_t  kinoko_null_object_type;
extern int32_t  kinoko_null_object_value;
extern struct SQSharedState* kinoko_primary_shared_state;
extern int32_t kinoko_release_watch_data[8];
extern int32_t kinoko_release_watch_count;

int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);
int32_t __fastcall kinoko_act_layer_associate_method(KinokoActLayer* receiver, void* unused_edx);
int32_t kinoko_sqrat_call_integer0(struct SQVM* a1);
int32_t kinoko_sqrat_call_integer1(struct SQVM* a1);
int32_t kinoko_is_release_watch_data(int32_t data);
int32_t kinoko_load_act_texture(const char *texture_name);
__declspec(noinline) void kinoko_trace_i32(const char *label,
                                                  int32_t value);
void kinoko_trace_ref_watch(const char *label, int32_t shared_state,
                                   int32_t type, int32_t data);
void kinoko_trace_squirrel_name(const char *label, int32_t name_ptr);
#ifdef __cplusplus
}
#endif
