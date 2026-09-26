struct SQVM;
#pragma once
#include "kinoko/legacy_string.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered host entry points. Values crossing this C ABI keep their Win32
   bit patterns; implementation uses the vendored Squirrel 2.2.2 source. */
void* kinoko_cact_layer_property_offset(struct SQVM* vm,
    int32_t *offset);
int32_t kinoko_cact_layer_get_int(struct SQVM* vm);
int32_t kinoko_cact_layer_set_int(struct SQVM* vm);
int32_t kinoko_cact_layer_get_bool(struct SQVM* vm);
int32_t kinoko_cact_layer_set_bool(struct SQVM* vm);
int32_t kinoko_cact_layer_get_float(struct SQVM* vm);
int32_t kinoko_cact_layer_set_float(struct SQVM* vm);
int32_t kinoko_cact_layer_get_pointer_float(struct SQVM* vm);
int32_t kinoko_cact_layer_set_pointer_float(struct SQVM* vm);
int32_t kinoko_cact_layer_get_pointer_int(struct SQVM* vm);
int32_t kinoko_cact_layer_set_pointer_int(struct SQVM* vm);
int32_t kinoko_cact_layer_get_string(struct SQVM* vm);
int32_t kinoko_cact_layer_set_string(struct SQVM* vm);
void* kinoko_c2dlayout_property_offset(struct SQVM* vm,
    int32_t *offset);
int32_t kinoko_c2dlayout_get_int(struct SQVM* vm);
int32_t kinoko_c2dlayout_get_float(struct SQVM* vm);
int32_t kinoko_c2dlayout_set_int(struct SQVM* vm);
int32_t kinoko_c2dlayout_set_float(struct SQVM* vm);
int32_t kinoko_c2dlayout_set_color(struct SQVM* vm);
void* kinoko_acting_player_property(struct SQVM* vm, int32_t *offset);
int32_t kinoko_acting_player_get_property(struct SQVM* vm);
int32_t kinoko_acting_player_set_property(struct SQVM* vm);
int32_t kinoko_native_view_get_short(struct SQVM* vm);
int32_t kinoko_native_view_set_short(struct SQVM* vm);

#ifdef __cplusplus
}
#endif
