#pragma once
#include "kinoko/legacy_string.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered host entry points. Values crossing this C ABI keep their Win32
   bit patterns; implementation uses the vendored Squirrel 2.2.2 source. */
int32_t kinoko_cact_layer_property_offset(int32_t vm,
    int32_t *offset);
int32_t kinoko_cact_layer_get_int(int32_t vm);
int32_t kinoko_cact_layer_set_int(int32_t vm);
int32_t kinoko_cact_layer_get_bool(int32_t vm);
int32_t kinoko_cact_layer_set_bool(int32_t vm);
int32_t kinoko_cact_layer_get_float(int32_t vm);
int32_t kinoko_cact_layer_set_float(int32_t vm);
int32_t kinoko_cact_layer_get_pointer_float(int32_t vm);
int32_t kinoko_cact_layer_set_pointer_float(int32_t vm);
int32_t kinoko_cact_layer_get_pointer_int(int32_t vm);
int32_t kinoko_cact_layer_set_pointer_int(int32_t vm);
int32_t kinoko_cact_layer_get_string(int32_t vm);
int32_t kinoko_cact_layer_set_string(int32_t vm);
int32_t kinoko_c2dlayout_property_offset(int32_t vm,
    int32_t *offset);
int32_t kinoko_c2dlayout_get_int(int32_t vm);
int32_t kinoko_c2dlayout_get_float(int32_t vm);
int32_t kinoko_c2dlayout_set_int(int32_t vm);
int32_t kinoko_c2dlayout_set_float(int32_t vm);
int32_t kinoko_c2dlayout_set_color(int32_t vm);
int32_t kinoko_acting_player_property(int32_t vm, int32_t *offset);
int32_t kinoko_acting_player_get_property(int32_t vm);
int32_t kinoko_acting_player_set_property(int32_t vm);
int32_t kinoko_native_view_get_short(int32_t vm);
int32_t kinoko_native_view_set_short(int32_t vm);

#ifdef __cplusplus
}
#endif
