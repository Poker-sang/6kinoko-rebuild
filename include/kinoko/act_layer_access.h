#pragma once
#include <stdint.h>
#include "kinoko/act_types.h"
#ifdef __cplusplus
extern "C" {
#endif
KinokoActLayer *__fastcall kinoko_act_layer_world_position(KinokoActLayer *layer,
    void *unused, float *x, float *y, float *z);
// 455DC0/455F50: output receives an owned, malloc-compatible one-word wrapper.
// Its layer/key is borrowed. Returns the output slot even for a missing item;
// a null output slot is rejected before accessing the holder.
KinokoActLayerHolder **kinoko_act_layer_holder(
    const KinokoActSourceHolder *source, int32_t index, KinokoActLayerHolder **output);
KinokoActKeyHolder **kinoko_act_key_holder(
    const KinokoActLayerHolder *layer, int32_t index, KinokoActKeyHolder **output);
// 452040/452020: borrowed first key and its layout; no ownership transfer.
KinokoActKey *kinoko_act_first_key(KinokoActRuntime *runtime, int32_t index);
KinokoActLayout *kinoko_act_layer_layout(KinokoActRuntime *runtime, int32_t index);
/* 41EF20: borrowed resource; updates resourceID only for a non-null resource. */
int32_t __fastcall kinoko_act_layer_set_resource(
    KinokoActLayer *layer, void *unused, KinokoActResource *resource);
#ifdef __cplusplus
}
#endif
