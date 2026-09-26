#include "kinoko/file_io.h"
#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 42C030 borrows the 2D layout and a holder of the active reader.
   A successful version-1 read invalidates its cached pivot transforms. */
int32_t kinoko_act_read_layout2d_properties(KinokoActLayout *layout,
                                             KinokoArchiveReader** reader_holder, int32_t version);
#ifdef __cplusplus
}
#endif
