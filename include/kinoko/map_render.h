#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_clear_map_layout(int32_t layout);
int32_t __fastcall kinoko_clone_map_layout(int32_t source, void *unused);
int32_t __fastcall kinoko_delete_map_sprite(int32_t sprite, void *unused, int32_t flags);
int32_t kinoko_map_update(int32_t layout, int32_t left, int32_t top,
                         int32_t right, int32_t bottom);
int32_t kinoko_map_draw(int32_t layout, float x, float y);
/* Resolve the active ActingPlayer layout, never the source ACT's template. */
int32_t kinoko_map_find_layout(int32_t manager, const char *name);
int32_t kinoko_map_create_render_layer(int32_t manager, const char *name);
int32_t __fastcall kinoko_map_entry_434b40(int32_t layout, void *unused);
int32_t __fastcall kinoko_map_entry_434b60(int32_t layout, void *unused,
    int32_t left, int32_t top, int32_t right, int32_t bottom);
int32_t __fastcall kinoko_map_entry_434f40(int32_t layout, void *unused,
    float x, float y);
int32_t __fastcall kinoko_map_entry_46eed0(int32_t layer, void *unused,
    int32_t camera);
#ifdef __cplusplus
}
#endif
