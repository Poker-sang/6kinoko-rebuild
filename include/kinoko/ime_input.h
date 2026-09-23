#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_ime_initialize(void);
int32_t kinoko_ime_dispatch(int32_t window, uint32_t message, uint32_t key, int32_t parameter);
#ifdef __cplusplus
}
#endif
