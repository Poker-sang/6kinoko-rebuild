#pragma once
#include <stdint.h>
#include "kinoko/act_types.h"
#include "kinoko/camera.h"
#include "kinoko/render_queue.h"
#ifdef __cplusplus
extern "C" {
#endif
struct kinoko_mcd_data;
int32_t kinoko_map_update_visible(KinokoActLayout *layout, int32_t left, int32_t top,
    int32_t right, int32_t bottom);
int32_t kinoko_map_draw_visible(KinokoActLayout *layout, float x, float y);
/* 463E60/46FD70: consult the owning layer, without touching render caches. */
struct kinoko_mcd_data *kinoko_map_layer_chip_data(KinokoActLayout *layout);
/* Inspect only: never binds, retains, or releases a resource. */
struct kinoko_mcd_data *kinoko_map_cached_chip_data(KinokoActLayout *layout);
/* 435220: query consumers lazily bind an empty layout resource cache. */
struct kinoko_mcd_data *kinoko_map_query_chip_data(KinokoActLayout *layout);
void kinoko_clear_map_layout(KinokoActLayout* layout);
KinokoActLayout* __fastcall kinoko_clone_map_layout(KinokoActLayout* source, void *unused);
void* __fastcall kinoko_delete_map_sprite(void* sprite, void *unused, int32_t flags);
/* Resolve the active ActingPlayer layout, never the source ACT's template. */
KinokoActLayout *kinoko_map_lookup_layout(KinokoMapManager *manager, const char *name);
/* Compatibility boundary for remaining integer-slot callers. */
KinokoRenderLayer *kinoko_map_make_render_layer(KinokoMapManager *manager, const char *name);
int32_t __fastcall kinoko_map_update_all_entry(KinokoActLayout* layout, void *unused);
int32_t __fastcall kinoko_map_update_visible_entry(KinokoActLayout* layout, void *unused,
    int32_t left, int32_t top, int32_t right, int32_t bottom);
int32_t __fastcall kinoko_map_draw_entry(KinokoActLayout* layout, void *unused,
    float x, float y);
int32_t __fastcall kinoko_map_render_layer_entry(KinokoRenderLayer* layer, void *unused,
    KinokoCamera* camera);
#ifdef __cplusplus
}
#endif
