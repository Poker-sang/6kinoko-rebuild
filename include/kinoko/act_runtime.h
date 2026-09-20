#pragma once
#include <stdint.h>
#include <stddef.h>

/* Recovered ACT/MCD records shared by the loader, native renderer and Sqrat
 * bindings. Pointer-bearing records require the original Win32 ABI. */
struct retdec_act_property {
    char *name;
    uint32_t type;
    int32_t integer;
    float real;
    char *string;
    uint32_t string_length;
};

struct retdec_mcd_chip {
    uint32_t chip_id;
    unsigned char bytes[48];
};

struct retdec_mcd_texture {
    uint32_t texture_id;
    int32_t handle;
};

struct retdec_mcd_data {
    uint32_t chip_count;
    struct retdec_mcd_chip *chips;
    uint32_t texture_count;
    struct retdec_mcd_texture *textures;
};

struct retdec_native_view_property {
    const char *name;
    int32_t offset;
    int32_t kind;
};

#ifdef __cplusplus
extern "C" {
#endif

int32_t function_44e780(uint32_t count,
                        int32_t *vector,
                        int32_t position,
                        int32_t value);
int32_t function_458090(int32_t a1, int32_t result, int32_t a3, int32_t a4);
int32_t function_458200(int32_t a1);
int32_t function_4583a0(int32_t this_ptr, int32_t result);
int32_t kinoko_map_draw(int32_t layout,
                                             float x, float y);
int32_t kinoko_map_update(
    int32_t layout, int32_t view_left, int32_t view_top,
    int32_t view_right, int32_t view_bottom);
int32_t retdec_act_append_list(int32_t list_slot, int32_t value);
void retdec_act_apply_cact(int32_t object_ptr,
                                  struct retdec_act_property *properties,
                                  uint32_t count);
void retdec_act_apply_chip_resource(
    int32_t object_ptr, struct retdec_act_property *properties,
    uint32_t count);
void retdec_act_apply_layer(int32_t object_ptr,
                                   struct retdec_act_property *properties,
                                   uint32_t count);
void retdec_act_apply_layout(int32_t object_ptr,
                                    struct retdec_act_property *properties,
                                    uint32_t count);
void retdec_act_apply_map_layout(
    int32_t object_ptr, struct retdec_act_property *properties,
    uint32_t count);
void retdec_act_apply_resource(
    int32_t object_ptr, struct retdec_act_property *properties,
    uint32_t count);
void retdec_act_apply_script(int32_t object_ptr,
                                    struct retdec_act_property *properties,
                                    uint32_t count);
void retdec_act_assign_string(int32_t object_ptr, uint32_t offset,
                                     const struct retdec_act_property *property);
int32_t retdec_act_bind_layouts(int32_t act);
void retdec_act_free_map_records(int32_t layout);
void retdec_act_free_properties(struct retdec_act_property *properties,
                                        uint32_t count);
int32_t retdec_act_load(int32_t this_ptr, int32_t reader_ptr,
                               int32_t version);
int32_t retdec_act_load_key(int32_t key, int32_t reader_ptr,
                                   int32_t version);
int32_t retdec_act_load_layer(int32_t layer, int32_t reader_ptr,
                                     int32_t version);
int32_t retdec_act_load_mcd(int32_t resource,
                                   const char *file_name);
int32_t retdec_act_load_script(int32_t object_ptr, int32_t reader_ptr);
int32_t retdec_act_make_key(int32_t reader_ptr, int32_t version);
int32_t retdec_act_make_layer(void);
int32_t retdec_construct_cact_layer(int32_t layer, int32_t vm);
int32_t retdec_construct_c2dlayout(int32_t layout);
int32_t retdec_act_make_layout(int32_t reader_ptr);
int32_t retdec_act_make_list(int32_t *list_slot);
int32_t retdec_act_make_map_layout(int32_t reader_ptr);
int32_t retdec_act_make_resource(int32_t reader_ptr, uint32_t type);
int32_t retdec_act_prepare_vector(int32_t object_ptr,
                                         uint32_t begin_offset,
                                         uint32_t end_offset,
                                         uint32_t capacity_offset,
                                         uint32_t count);
float retdec_act_property_float(
    const struct retdec_act_property *property);
int32_t retdec_act_property_integer(
    const struct retdec_act_property *property);
int32_t retdec_act_read_map_records(int32_t layout,
                                           int32_t reader_ptr);
int32_t retdec_act_read_properties(
    int32_t reader_ptr, struct retdec_act_property **properties_out,
    uint32_t *count_out);
int32_t retdec_act_read_u32(int32_t reader_ptr, uint32_t *value);
int32_t retdec_act_read_u8(int32_t reader_ptr, uint8_t *value);
int32_t retdec_begin_stage_this(int32_t resource_ptr, int32_t stage);
int32_t retdec_bind_act_resource_object(int32_t resource_ptr);
int32_t retdec_c2dlayout_draw_impl(int32_t layout,
                                           float x, float y);
int32_t retdec_c2dlayout_set_layer_impl(int32_t layout,
                                                int32_t layer);
int32_t retdec_c2dlayout_update_faithful_impl(int32_t layout);
int32_t retdec_c2dlayout_update_impl(int32_t layout);
void retdec_c2dlayout_world_position(int32_t layer,
                                             float *x,
                                             float *y,
                                             float *z);
int32_t retdec_c2dmaplayout_set_layer_impl(int32_t layout,
                                                   int32_t layer);
int32_t retdec_cact_associate_resource(int32_t vm);
int32_t retdec_compare_bytes32(const unsigned char *left,
                                      const unsigned char *right,
                                      size_t length);
int32_t retdec_compare_strings32(int32_t left_object,
                                        int32_t right_object);
int32_t retdec_construct_cact_script(int32_t this_ptr);
void retdec_destroy_cact_layer(int32_t layer);
void retdec_destroy_cact_key(int32_t key);
void retdec_destroy_cact_list(int32_t *list_slot);
void retdec_destroy_cact_object(int32_t object_ptr);
void retdec_destroy_cact_resource(int32_t resource);
int32_t retdec_register_act_script(int32_t script, int32_t object);
void retdec_forget_act_script(int32_t script);
void retdec_destroy_cact_script(int32_t script_ptr);
int32_t retdec_destroy_cact_with_flags(int32_t object_ptr,
                                               unsigned char flags);
int32_t retdec_erase_node32(int32_t list_base,
                                   int32_t result,
                                   int32_t node);
int32_t retdec_execute_act_callback(int32_t script_ptr,
                                            int32_t offset,
                                            const char *trace_label);
int32_t retdec_execute_act_source_script(
    int32_t vm, int32_t script_ptr, const int32_t *environment_pair);
int32_t retdec_get_act_resource_class(int32_t vm, int32_t resource, int32_t out[2]);
int32_t retdec_map_chip_count(int32_t vm);
struct retdec_mcd_data *retdec_map_chip_data(int32_t layout);
int retdec_map_compare_records(const void *a, const void *b);
int32_t retdec_map_get_chip_by_position(int32_t vm);
int32_t retdec_map_get_chip_id(int32_t vm);
int32_t retdec_map_get_chip_layout(int32_t vm);
int32_t retdec_map_layout_argument(int32_t vm, int32_t *index);
int32_t retdec_map_prearrangement(int32_t vm);
int32_t retdec_map_record_at(int32_t layout, int32_t index);
int32_t retdec_map_set_chip_id(int32_t vm);
int32_t retdec_map_set_chip_layout(int32_t vm);
int32_t retdec_map_set_chip_rect(int32_t vm);
int32_t retdec_map_sprite_init(int32_t sprite, int32_t handle,
                                      const unsigned char *chip_bytes);
struct retdec_mcd_chip *retdec_mcd_find_chip(
    struct retdec_mcd_data *data, uint32_t chip_id);
struct retdec_mcd_texture *retdec_mcd_find_texture(
    struct retdec_mcd_data *data, uint32_t texture_id);
void retdec_mcd_free(struct retdec_mcd_data *data);
int16_t retdec_mcd_i16(const unsigned char *bytes);
uint32_t retdec_mcd_u32(const unsigned char *bytes);
int32_t retdec_prepare_cact_layer_objects(int32_t vm, int32_t layer,
                                                 int32_t script_pair[2]);
int32_t retdec_publish_act_layers(int32_t vm, int32_t act,
                                          int32_t resource_ptr,
                                          int32_t *active_count);
int32_t retdec_publish_act_resource_pairs(
    int32_t vm, const int32_t layer_pair[2],
    const int32_t script_pair[2], int32_t resource);

int32_t retdec_publish_act_script_constants(int32_t vm, const int32_t *environment);
int32_t retdec_publish_acting_player(int32_t vm,
                                             const int32_t *act_pair,
                                             const char *name,
                                             int32_t player_ptr,
                                             int32_t out_pair[2]);
int32_t retdec_publish_acting_player_class(int32_t vm,
                                                   int32_t root_object);
int32_t retdec_publish_acting_player_properties(int32_t vm,
                                                        const int32_t class_pair[2]);
int32_t retdec_publish_c2dlayout_class(int32_t vm, int32_t root_object);
int32_t retdec_publish_c2dlayout_properties(
    int32_t vm, const int32_t class_pair[2]);

int32_t retdec_publish_c2dmaplayout_class(int32_t vm, int32_t root,
                                                int32_t out[2]);
int32_t retdec_publish_cact_layer_class(int32_t vm, int32_t root_object);
int32_t retdec_publish_cact_layer_members(
    int32_t vm, const int32_t *class_pair);
int32_t retdec_publish_cact_layer_property(
    int32_t vm, const char *name, int32_t offset,
    int32_t getter, int32_t setter);
int32_t retdec_publish_cact_resource2d_class(int32_t vm,
                                                     int32_t root_object);
int32_t retdec_publish_map_view_class(int32_t vm, int32_t root,
    const char *name, const struct retdec_native_view_property *properties,
    int32_t property_count, int32_t is_map, int32_t out[2]);
int32_t retdec_register_runtime_act_script(int32_t vm, int32_t resource_ptr,
                                                 int32_t act);
int32_t retdec_resource_get_chip_info(int32_t vm);
int32_t retdec_root_table_construct_this(int32_t resource_ptr,
                                                 int32_t vm,
                                                 int32_t output_ptr);
int32_t retdec_root_table_register_resource(int32_t root_object,
                                                    int32_t resource_ptr);
void retdec_sprite_rotate_faithful(int32_t sprite,
                                           float angle_x,
                                           float angle_y,
                                           float angle_z,
                                           float pivot_x,
                                           float pivot_y,
                                           float pivot_z);
void retdec_sprite_rotate_xy(float *x, float *y,
                                    float pivot_x, float pivot_y,
                                    float angle);
float retdec_sprite_scale_about(float value,
                                           float pivot,
                                           float scale);
void retdec_sprite_scale_faithful(int32_t sprite,
                                          float scale_x,
                                          float pivot_x,
                                          float scale_y,
                                          float pivot_y,
                                          float scale_z,
                                          float pivot_z);
void retdec_sprite_translate_faithful(int32_t sprite,
                                              float x,
                                              float y,
                                              float z);
const unsigned char *retdec_string_data32(int32_t object);
uint32_t retdec_string_hash32(int32_t object);
uint32_t retdec_string_length32(int32_t object);
int32_t retdec_vector_insert32(uint32_t count,
                                      int32_t *vector,
                                      int32_t position,
                                      int32_t value);

#ifdef __cplusplus
}
#endif
