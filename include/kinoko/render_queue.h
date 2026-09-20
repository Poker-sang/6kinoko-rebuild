#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_initialize_render_queue(void);
int32_t kinoko_clear_render_queue(void);
int32_t kinoko_render_queue_identity(void);
int32_t kinoko_render_queue_first(void);
int32_t kinoko_render_queue_size(void);
int32_t kinoko_append_render_queue(int32_t object);
void kinoko_draw_render_queue(int32_t camera);
#ifdef __cplusplus
}
#endif
