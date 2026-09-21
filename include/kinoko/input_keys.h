#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_input_keys_construct(int32_t tracker);
void kinoko_input_keys_destroy(int32_t tracker);
void kinoko_input_keys_clear(int32_t tracker);
void kinoko_input_keys_add(int32_t tracker, uint8_t scan);
void kinoko_input_keys_assign(int32_t destination, int32_t source);
uint32_t kinoko_input_keys_size(int32_t tracker);
uint8_t kinoko_input_keys_at(int32_t tracker, uint32_t index);
#ifdef __cplusplus
}
#endif
