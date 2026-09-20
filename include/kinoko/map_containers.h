#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_map_containers_construct(int32_t manager);
void kinoko_map_containers_clear(int32_t manager);
void kinoko_map_containers_destroy(int32_t manager);
void kinoko_map_containers_assign(int32_t destination, int32_t source);
int32_t kinoko_map_append_render(int32_t manager, int32_t layout);
uint32_t kinoko_map_render_count(int32_t manager);
int32_t kinoko_map_render_at(int32_t manager, uint32_t index);
void kinoko_map_append_event(int32_t manager, int32_t layout);
uint32_t kinoko_map_event_count(int32_t manager);
int32_t kinoko_map_event_at(int32_t manager, uint32_t index);
uint32_t kinoko_map_event_capacity(int32_t manager);
#ifdef __cplusplus
}
#endif
