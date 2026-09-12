#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void kinoko_sq_gc_move_marked(int32_t object, int32_t type, int32_t *live_head);
int32_t kinoko_sq_gc_sweep(int32_t shared_state, int32_t live_head);
#ifdef __cplusplus
}
#endif
