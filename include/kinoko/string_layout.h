#include "kinoko/file_io.h"
struct SQVM;
#pragma once
#include <stdint.h>
#include "kinoko/act_types.h"
typedef struct KinokoStringLayout KinokoStringLayout;
#ifdef __cplusplus
extern "C" {
#endif
const void *kinoko_string_layout_methods(void);
KinokoStringLayout* __fastcall kinoko_method_clone_string_layout(KinokoStringLayout* object, void *unused);
KinokoStringLayout* __fastcall kinoko_method_destroy_string_layout(KinokoStringLayout* object, void *unused);
int32_t __fastcall kinoko_method_string_layout_type(KinokoStringLayout* object, void *unused);
int32_t __fastcall kinoko_method_register_string_layout(int32_t object, void *unused);
void kinoko_string_queue_construct(KinokoStringLayout* object);
void kinoko_string_queue_destroy(KinokoStringLayout* object);
uint32_t kinoko_string_queue_size(KinokoStringLayout* object);
void* kinoko_string_queue_at(KinokoStringLayout* object, uint32_t index);
void kinoko_string_drop_queue_storage(KinokoStringLayout* object);
void* kinoko_string_append_glyph(KinokoStringLayout* object);
uint32_t kinoko_string_atlas_size(KinokoStringLayout* layout);
void* kinoko_string_atlas_at(KinokoStringLayout* layout, uint32_t index);
void* kinoko_string_append_atlas(KinokoStringLayout* object);
int32_t kinoko_string_add_character(KinokoStringLayout* object, const char *character);
int32_t __fastcall kinoko_method_update_string_layout(KinokoStringLayout* object, void *unused);
int32_t __fastcall kinoko_method_draw_string_layout(KinokoStringLayout* object, void *unused, float x, float y);
int32_t __fastcall kinoko_method_set_string_layer(KinokoStringLayout* object, void *unused, KinokoActLayer* layer);
KinokoStringLayout* kinoko_construct_string_layout(KinokoStringLayout* object);
void kinoko_clear_string_layout(KinokoStringLayout* object);
int32_t kinoko_string_push_back(KinokoStringLayout* object, const char *text);
int32_t kinoko_string_clear(KinokoStringLayout* object);
int32_t kinoko_string_pop(KinokoStringLayout* object, int32_t count, int32_t front);
int32_t kinoko_string_replicate(KinokoStringLayout* object, KinokoStringLayout* source);
int32_t kinoko_string_character_bytes(const char *text);
int32_t kinoko_string_mark_rebuild(KinokoStringLayout* object);
int32_t kinoko_string_prune_atlases(KinokoStringLayout *layout);
int32_t kinoko_string_rebuild_queue(KinokoStringLayout *layout);
int32_t kinoko_publish_string_layout_class(struct SQVM* vm, void* root, int32_t *class_pair);
int32_t kinoko_string_read_properties(KinokoStringLayout *layout, KinokoArchiveReader** reader_holder, int32_t version);
int32_t __fastcall kinoko_method_read_string_layout(KinokoStringLayout* object, void *unused, KinokoArchiveReader** holder, int32_t version);
KinokoStringLayout* __fastcall kinoko_method_delete_string_layout(KinokoStringLayout* object, void *unused, unsigned char flags);
#ifdef __cplusplus
}
#endif
