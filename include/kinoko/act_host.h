#pragma once
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
};

#ifdef __cplusplus
extern "C" {
#endif
const struct KinokoActHostSymbols* kinoko_act_host_symbols(void);
extern char  g1037;
extern int32_t  g1049;
extern int32_t  g1050;
extern int32_t  g1079;
extern int32_t  g1080;
extern int32_t  g1141;
extern int32_t  g1142;
extern int32_t  g1143;
extern int32_t  g1144;
extern int32_t  g1145;
extern int32_t  g1151;
extern int32_t  g1152;
extern int32_t  g1153;
extern int32_t  g1154;
extern int32_t  g1224;
extern int32_t  g483;
extern int32_t  g484;
extern int32_t  g678;
extern int32_t  g765;
extern int32_t retdec_primary_shared_state;
extern int32_t retdec_release_watch_data[8];
extern int32_t retdec_release_watch_count;

int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);
int32_t function_402d40(char * a1, int32_t a2);
float function_4040d0(long double a1);
float function_404130(long double a1);
int32_t function_407370(int32_t reader_slot_address, const char *file_name);
int32_t function_41ef50(int32_t a1, int32_t a2, int32_t a3);
int32_t function_4252e0(void);
int32_t function_445530(int32_t a1);
int32_t function_4455e0(int32_t a1);
int32_t function_445650(int32_t a1);
int32_t function_4517c0(int32_t a1);
int32_t function_451b70(int32_t a1);
int32_t function_451f30(int32_t a1);
int32_t function_451f80(int32_t a1, int32_t a2);
int32_t function_452010(int32_t a1);
int32_t function_452150(int32_t lpFileName);
int32_t function_452220(void);
int32_t function_452270(void);
int32_t function_4522c0(void);
int32_t function_455330(int32_t a1);
int32_t function_455390(int32_t a1);
int32_t function_4554b0(int32_t a1);
int32_t function_455520(int32_t a1);
int32_t function_4556c0(int32_t a1);
int32_t function_455730(int32_t a1);
int32_t function_458330(int32_t a1, int32_t a2, int32_t result);
void retdec_destroy_reader(int32_t *reader);
int32_t retdec_is_release_watch_data(int32_t data);
int32_t retdec_layout_submit_impl(int32_t vertex_buffer,
                                          float x, float y);
int32_t retdec_load_act_texture(const char *texture_name);
int32_t retdec_reader_read_exact(int32_t reader_ptr, void *buffer,
                                        uint32_t size);
int32_t retdec_reader_seek_relative(int32_t reader_ptr,
                                            uint32_t offset);
int32_t retdec_set_texture_stage(int32_t stage, int32_t handle);
int32_t retdec_string_assign_cstr(int32_t *this_ptr,
                                         const char *source);
int32_t retdec_string_assign_n(int32_t *this_ptr,
                                      const char *source,
                                      uint32_t size);
__declspec(noinline) void retdec_trace_i32(const char *label,
                                                  int32_t value);
void retdec_trace_ref_watch(const char *label, int32_t shared_state,
                                   int32_t type, int32_t data);
void retdec_trace_squirrel_name(const char *label, int32_t name_ptr);
#ifdef __cplusplus
}
#endif
