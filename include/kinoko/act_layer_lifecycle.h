#pragma once
#include "kinoko/act_types.h"
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif
/* Caller owns allocation; clear releases members only. These APIs describe
   actual host pointers, not the fixed-width ACT file representation. */
KinokoActLayer* kinoko_act_layer_initialize(KinokoActLayer* layer, struct SQVM* vm);
void kinoko_act_layer_clear(KinokoActLayer* layer);
#ifdef __cplusplus
}
#endif
