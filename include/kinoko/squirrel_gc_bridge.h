#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void kinoko_sq_gc_move_marked(int32_t object, int32_t type, int32_t *live_head);
int32_t kinoko_sq_gc_sweep(int32_t shared_state, int32_t live_head);
void __fastcall kinoko_sq_vm_release(int32_t vm, void *unused);
void __fastcall kinoko_sq_array_release(int32_t array, void *unused);
#ifdef __cplusplus
}
#endif
