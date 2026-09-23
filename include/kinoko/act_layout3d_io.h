#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 43C860 borrows a 3D layout and a holder of the active reader. It does
   not consume either object; only version 1 is accepted. */
int32_t kinoko_act_read_layout3d_properties(KinokoActLayout *layout,
                                             int32_t *reader_holder, int32_t version);
#ifdef __cplusplus
}
#endif
