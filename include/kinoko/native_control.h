#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native Win32 control blocks, NOT Squirrel object references. Address words
   exist only at the remaining generated-C boundary. The allocation is the
   separately owned Actor* slot, never the borrowed Actor it points to. */
int32_t kinoko_native_control_create(int32_t holder_address, int32_t allocation_address);
int32_t* kinoko_native_weak_pair_lock(int32_t pair_address, int32_t* output_pair);
void kinoko_native_add_strong(int32_t control_address);
void kinoko_native_add_weak(int32_t control_address);
void kinoko_native_release_weak(int32_t control_address);
void kinoko_native_release_strong(int32_t control_address);
#ifdef __cplusplus
}
#endif
