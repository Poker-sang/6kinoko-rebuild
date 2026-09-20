#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
extern int32_t g350[11];
int32_t __fastcall kinoko_method_clone_string_layout(int32_t object, void *unused);
int32_t __fastcall kinoko_method_destroy_string_layout(int32_t object, void *unused);
int32_t __fastcall kinoko_method_string_layout_type(int32_t object, void *unused);
int32_t __fastcall kinoko_method_register_string_layout(int32_t object, void *unused);
int32_t kinoko_string_append_glyph(int32_t object);
int32_t kinoko_string_append_atlas(int32_t object);
int32_t kinoko_string_add_character(int32_t object, const char *character);
int32_t __fastcall kinoko_method_update_string_layout(int32_t object, void *unused);
int32_t __fastcall kinoko_method_draw_string_layout(int32_t object, void *unused, float x, float y);
int32_t __fastcall kinoko_method_set_string_layer(int32_t object, void *unused, int32_t layer);
int32_t kinoko_construct_string_layout(int32_t object);
void kinoko_clear_string_layout(int32_t object);
int32_t kinoko_string_push_back(int32_t object, const char *text);
int32_t kinoko_string_clear(int32_t object);
int32_t kinoko_string_pop(int32_t object, int32_t count, int32_t front);
int32_t kinoko_string_replicate(int32_t object, int32_t source);
int32_t kinoko_string_character_bytes(const char *text);
int32_t kinoko_string_mark_rebuild(int32_t object);
int32_t kinoko_publish_string_layout_class(int32_t vm, int32_t root, int32_t *class_pair);
int32_t __fastcall kinoko_method_read_string_layout(int32_t object, void *unused, int32_t holder, int32_t version);
int32_t __fastcall kinoko_method_delete_string_layout(int32_t object, void *unused, unsigned char flags);
#ifdef __cplusplus
}
#endif
