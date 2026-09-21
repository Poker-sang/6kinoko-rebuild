#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_stage_list_construct(void);
void kinoko_stage_list_destroy(void);
int32_t kinoko_stage_list_first(void);
int32_t kinoko_stage_list_next(int32_t token);
int32_t kinoko_stage_list_value(int32_t token);
int32_t kinoko_stage_list_append(int32_t owner);
int32_t kinoko_clear_global_stages(void);
int32_t kinoko_clear_global_sound(void);
void kinoko_initialize_render_queue(void);
#ifdef __cplusplus
}
#endif
