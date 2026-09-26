#include "kinoko/act_types.h"
#include "kinoko/file_io.h"
struct SQVM;
#pragma once
#include <stdint.h>
#include <stddef.h>

/* Recovered ACT/MCD records shared by the loader, native renderer and Sqrat
 * bindings. Pointer-bearing records require the original Win32 ABI. */
struct kinoko_mcd_chip {
    uint32_t chip_id;
    unsigned char bytes[48];
};

struct kinoko_mcd_texture {
    uint32_t texture_id;
    int32_t handle;
};

struct kinoko_mcd_data {
    uint32_t chip_count;
    struct kinoko_mcd_chip *chips;
    uint32_t texture_count;
    struct kinoko_mcd_texture *textures;
};

struct kinoko_native_view_property {
    const char *name;
    int32_t offset;
    int32_t kind;
};

#ifdef __cplusplus
extern "C" {
#endif

int32_t kinoko_act_append_list(void* list_slot, void* value);
int32_t __fastcall kinoko_delete_layout_sprite(int32_t sprite, void *unused, int32_t flags);
void kinoko_act_free_map_records(int32_t layout);
int32_t kinoko_act_load(KinokoActDocument* this_ptr, KinokoArchiveReader* reader_ptr,
                               int32_t version);
int32_t kinoko_act_load_key(KinokoActKey* key, KinokoArchiveReader* reader_ptr,
                                   int32_t version);
int32_t kinoko_act_read_script_properties(void* script, KinokoArchiveReader* reader);
const char* kinoko_act_serialized_type_name(const void* object);
int32_t kinoko_act_load_layer(KinokoActLayer* layer, KinokoArchiveReader* reader_ptr,
                                     int32_t version);
int32_t kinoko_act_load_mcd(KinokoActResource* resource,
                                   const char *file_name);
int32_t kinoko_act_load_script(void* object_ptr, KinokoArchiveReader* reader_ptr);
KinokoActKey* kinoko_act_make_key(KinokoArchiveReader* reader_ptr, int32_t version);
const void* kinoko_act_timeline_vtable(void);
KinokoActTimeline* kinoko_act_new_timeline(void);
int32_t kinoko_act_load_timeline(KinokoActTimeline* timeline, KinokoArchiveReader*  reader, int32_t version);
KinokoActLayer* kinoko_act_make_layer(void);
KinokoActLayout* kinoko_construct_c2dlayout(KinokoActLayout* layout);
KinokoActLayout* kinoko_act_make_layout(KinokoArchiveReader* reader_ptr);
int32_t kinoko_act_make_list(void* list_slot);
KinokoActLayout* kinoko_act_make_map_layout(KinokoArchiveReader* reader_ptr);
KinokoActResource* kinoko_act_make_resource(KinokoArchiveReader* reader_ptr, uint32_t type);
int32_t kinoko_act_prepare_vector(int32_t object_ptr,
                                         uint32_t begin_offset,
                                         uint32_t end_offset,
                                         uint32_t capacity_offset,
                                         uint32_t count);
int32_t kinoko_act_read_map_records(int32_t layout,
                                           KinokoArchiveReader* reader_ptr);
int32_t kinoko_act_read_u32(KinokoArchiveReader* reader_ptr, uint32_t *value);
int32_t kinoko_act_read_u8(KinokoArchiveReader* reader_ptr, uint8_t *value);
int32_t kinoko_begin_stage_this(KinokoActRuntime* resource_ptr, int32_t stage);
int32_t kinoko_bind_act_resource_object(KinokoActRuntime* resource_ptr);
int32_t kinoko_c2dlayout_draw_impl(int32_t layout,
                                           float x, float y);
int32_t kinoko_c2dlayout_set_layer_impl(int32_t layout,
                                                int32_t layer);
int32_t kinoko_c2dlayout_update_faithful_impl(int32_t layout);
int32_t kinoko_c2dlayout_update_impl(int32_t layout);
void kinoko_c2dlayout_world_position(int32_t layer,
                                             float *x,
                                             float *y,
                                             float *z);
int32_t kinoko_c2dmaplayout_set_layer_impl(int32_t layout,
                                                   int32_t layer);
int32_t kinoko_cact_associate_resource(struct SQVM* vm);
void* kinoko_construct_cact_script(void* this_ptr);
void kinoko_destroy_cact_key(void* key);
void kinoko_destroy_cact_list(void* list_slot);
void kinoko_destroy_cact_object(KinokoActDocument* object_ptr);
void kinoko_destroy_cact_resource(KinokoActResource* resource);
int32_t kinoko_register_act_script(void* script, void* object);
void kinoko_forget_act_script(void* script);
void kinoko_destroy_cact_script(void* script_ptr);
void* kinoko_destroy_cact_with_flags(KinokoActDocument* object_ptr,
                                               unsigned char flags);
int32_t kinoko_execute_act_callback(void* script_ptr,
                                            int32_t offset,
                                            const char *trace_label);
int32_t kinoko_execute_act_source_script(
    struct SQVM* vm, void* script_ptr, const int32_t *environment_pair);
int32_t kinoko_get_act_resource_class(struct SQVM* vm, int32_t resource, int32_t out[2]);
int32_t kinoko_map_chip_count(struct SQVM* vm);
struct kinoko_mcd_data *kinoko_map_chip_data(int32_t layout);
int kinoko_map_compare_records(const void *a, const void *b);
int32_t kinoko_map_get_chip_by_position(struct SQVM* vm);
int32_t kinoko_map_get_chip_id(struct SQVM* vm);
int32_t kinoko_map_get_chip_layout(struct SQVM* vm);
int32_t kinoko_map_layout_argument(struct SQVM* vm, int32_t *index);
int32_t kinoko_map_prearrangement(struct SQVM* vm);
int32_t kinoko_map_record_at(int32_t layout, int32_t index);
int32_t kinoko_map_set_chip_id(struct SQVM* vm);
int32_t kinoko_map_set_chip_layout(struct SQVM* vm);
int32_t kinoko_map_set_chip_rect(struct SQVM* vm);
int32_t kinoko_map_sprite_init(int32_t sprite, int32_t handle,
                                      const unsigned char *chip_bytes);
struct kinoko_mcd_chip *kinoko_mcd_find_chip(
    struct kinoko_mcd_data *data, uint32_t chip_id);
struct kinoko_mcd_texture *kinoko_mcd_find_texture(
    struct kinoko_mcd_data *data, uint32_t texture_id);
void kinoko_mcd_free(struct kinoko_mcd_data *data);
int16_t kinoko_mcd_i16(const unsigned char *bytes);
uint32_t kinoko_mcd_u32(const unsigned char *bytes);
int32_t kinoko_prepare_cact_layer_objects(struct SQVM* vm, KinokoActLayer* layer,
                                                 int32_t script_pair[2]);
int32_t kinoko_publish_act_layers(struct SQVM* vm, KinokoActDocument* act,
                                          KinokoActRuntime* resource_ptr,
                                          int32_t *active_count);
int32_t kinoko_publish_act_resource_pairs(
    struct SQVM* vm, const int32_t layer_pair[2],
    const int32_t script_pair[2], int32_t resource);

int32_t kinoko_publish_act_script_constants(struct SQVM* vm, const int32_t *environment);
int32_t kinoko_publish_acting_player(struct SQVM* vm,
                                             const int32_t *act_pair,
                                             const char *name,
                                             int32_t player_ptr,
                                             int32_t out_pair[2]);
int32_t kinoko_publish_acting_player_class(struct SQVM* vm,
                                                   void* root_object);
int32_t kinoko_publish_acting_player_properties(struct SQVM* vm,
                                                        const int32_t class_pair[2]);
int32_t kinoko_publish_c2dlayout_class(struct SQVM* vm, void* root_object);
int32_t kinoko_publish_c2dlayout_properties(
    struct SQVM* vm, const int32_t class_pair[2]);

int32_t kinoko_publish_c2dmaplayout_class(struct SQVM* vm, void* root,
                                                int32_t out[2]);
int32_t kinoko_publish_cact_layer_class(struct SQVM* vm, void* root_object);
int32_t kinoko_publish_cact_layer_members(
    struct SQVM* vm, const int32_t *class_pair);
int32_t kinoko_publish_cact_layer_property(
    struct SQVM* vm, const char *name, int32_t offset,
    int32_t getter, int32_t setter);
int32_t kinoko_publish_cact_resource2d_class(struct SQVM* vm,
                                                     void* root_object);
int32_t kinoko_publish_map_view_class(struct SQVM* vm, void* root,
    const char *name, const struct kinoko_native_view_property *properties,
    int32_t property_count, int32_t is_map, int32_t out[2]);
int32_t kinoko_register_runtime_act_script(struct SQVM* vm, KinokoActRuntime* resource_ptr,
                                                 KinokoActDocument* act);
int32_t kinoko_resource_get_chip_info(struct SQVM* vm);
int32_t kinoko_root_table_construct_this(KinokoActRuntime* resource_ptr,
                                                 struct SQVM* vm,
                                                 void* output_ptr);
int32_t kinoko_root_table_register_resource(void* root_object,
                                                    KinokoActRuntime* resource_ptr);
#ifdef __cplusplus
}
#endif
