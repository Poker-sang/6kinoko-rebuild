struct KinokoCamera;
#pragma once
#include <stdint.h>
typedef struct KinokoRenderLayer KinokoRenderLayer;
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_initialize_render_queue(void);
/* Return a borrowed queue node identity; appending never owns the layer. */
void *kinoko_render_queue_append(KinokoRenderLayer *layer);
int32_t kinoko_clear_render_queue(void);
int32_t kinoko_render_queue_identity(void);
int32_t kinoko_render_queue_first(void);
int32_t kinoko_render_queue_size(void);
void kinoko_draw_render_queue(struct KinokoCamera* camera);
#ifdef __cplusplus
}
#endif
