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
};

#ifdef __cplusplus
extern "C" {
#endif
const struct KinokoActHostSymbols* kinoko_act_host_symbols(void);
extern char  g1037;
extern int32_t kinoko_acting_player_class_pair[2];
extern int32_t kinoko_resource2d_class_pair[2];
#define g1079 (kinoko_resource2d_class_pair[0])
#define g1080 (kinoko_resource2d_class_pair[1])
extern int32_t kinoko_layout_set_pair[2];
#define g1141 (kinoko_layout_set_pair[0])
#define g1142 (kinoko_layout_set_pair[1])
extern int32_t kinoko_layout_get_pair[2];
#define g1143 (kinoko_layout_get_pair[0])
#define g1144 (kinoko_layout_get_pair[1])
extern int32_t kinoko_layout_class_pair[2];
#define g1145 (kinoko_layout_class_pair[0])
#define g1146 (kinoko_layout_class_pair[1])
extern int32_t kinoko_layer_set_pair[2];
#define g1151 (kinoko_layer_set_pair[0])
#define g1152 (kinoko_layer_set_pair[1])
extern int32_t kinoko_layer_get_pair[2];
#define g1153 (kinoko_layer_get_pair[0])
#define g1154 (kinoko_layer_get_pair[1])
extern int32_t kinoko_act_script_extension[7];
#define g554 (kinoko_act_script_extension[0])
#define g555 (kinoko_act_script_extension[4])
#define g556 (kinoko_act_script_extension[5])
extern int32_t  g1224;
extern int32_t  g483;
extern int32_t  g484;
extern int32_t retdec_primary_shared_state;
extern int32_t retdec_release_watch_data[8];
extern int32_t retdec_release_watch_count;

int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);
int32_t __fastcall kinoko_act_layer_associate_method(int32_t receiver, void* unused_edx);
int32_t kinoko_sqrat_call_integer0(int32_t a1);
int32_t kinoko_sqrat_call_integer1(int32_t a1);
int32_t retdec_is_release_watch_data(int32_t data);
int32_t retdec_layout_submit_impl(int32_t vertex_buffer,
                                          float x, float y);
int32_t retdec_load_act_texture(const char *texture_name);
__declspec(noinline) void retdec_trace_i32(const char *label,
                                                  int32_t value);
void retdec_trace_ref_watch(const char *label, int32_t shared_state,
                                   int32_t type, int32_t data);
void retdec_trace_squirrel_name(const char *label, int32_t name_ptr);
#ifdef __cplusplus
}
#endif
