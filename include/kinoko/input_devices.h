#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_input_devices_construct(int32_t manager);
void kinoko_input_devices_destroy(int32_t manager);
void kinoko_input_devices_resize(int32_t manager, uint32_t count);
void kinoko_input_devices_assign(int32_t destination, int32_t source);
int32_t kinoko_input_devices_begin(int32_t manager);
int32_t kinoko_input_devices_end(int32_t manager);
#ifdef __cplusplus
}
#endif
