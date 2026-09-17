#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_clear_global_stages(void);
int32_t kinoko_clear_global_sound(void);
void kinoko_initialize_render_queue(void);
#ifdef __cplusplus
}
#endif
