#include "kinoko/file_io.h"
#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow resource and reader slot for a version-1 serialized property read.
   Construction, texture creation and ownership remain with the caller. */
int32_t kinoko_act_read_texture_properties(KinokoActResource *resource,KinokoArchiveReader** reader_holder,int32_t version);
int32_t kinoko_act_read_render_target_properties(KinokoActResource *resource,KinokoArchiveReader** reader_holder,int32_t version);
int32_t kinoko_act_read_chip_properties(KinokoActResource *resource,KinokoArchiveReader** reader_holder,int32_t version);
#ifdef __cplusplus
}
#endif
