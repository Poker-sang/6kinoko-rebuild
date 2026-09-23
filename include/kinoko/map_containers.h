#pragma once
#include <stdint.h>
#include "kinoko/act_types.h"
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_map_containers_construct(KinokoMapManager* manager);
void kinoko_map_containers_clear(KinokoMapManager* manager);
void kinoko_map_containers_destroy(KinokoMapManager* manager);
void kinoko_map_containers_assign(KinokoMapManager* destination, KinokoMapManager* source);
KinokoRenderLayer* kinoko_map_append_render(KinokoMapManager* manager, KinokoActLayout* layout);
uint32_t kinoko_map_render_count(KinokoMapManager* manager);
KinokoRenderLayer* kinoko_map_render_at(KinokoMapManager* manager, uint32_t index);
void kinoko_map_append_event(KinokoMapManager* manager, KinokoActLayout* layout);
uint32_t kinoko_map_event_count(KinokoMapManager* manager);
KinokoActLayout* kinoko_map_event_at(KinokoMapManager* manager, uint32_t index);
uint32_t kinoko_map_event_capacity(KinokoMapManager* manager);
#ifdef __cplusplus
}
#endif
