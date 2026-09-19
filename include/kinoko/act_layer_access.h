#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Original 455DC0/455F50: output owns a malloc-compatible one-word holder;
// the pointed-to layer/key remains borrowed from the ACT container.
int32_t kinoko_act_layer_holder(int32_t act_holder, int32_t index, int32_t output);
int32_t kinoko_act_key_holder(int32_t layer_holder, int32_t index, int32_t output);
// Borrowed first key / its layout. Neither result transfers ownership.
int32_t function_452040(int32_t runtime, int32_t index);
int32_t function_452020(int32_t runtime, int32_t index);
#ifdef __cplusplus
}
#endif
