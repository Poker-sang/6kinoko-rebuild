#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* These records borrow the control-owned allocation and its native control.
   APIs accept byte storage so unaligned legacy fixtures remain valid. */
typedef struct KinokoNativeReference { void* allocation; void* control; } KinokoNativeReference;
void* kinoko_native_control_create(void* holder, void* allocation);
void* kinoko_native_weak_pair_lock(const void* pair, void* output);
void kinoko_native_add_strong(void* control);
void kinoko_native_add_weak(void* control);
void kinoko_native_release_weak(void* control);
void kinoko_native_release_strong(void* control);
#ifdef __cplusplus
}
#endif
