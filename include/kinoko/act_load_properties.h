#pragma once
#include "kinoko/act_types.h"
#include "kinoko/file_io.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Property stream readers borrow both record and active reader. */
int32_t kinoko_act_read_key_properties_typed(KinokoActKey *key,KinokoArchiveReader *reader);
int32_t kinoko_act_read_layer_properties_typed(KinokoActLayer *layer,KinokoArchiveReader *reader);
int32_t kinoko_act_read_map_properties_typed(KinokoActLayout *layout,KinokoArchiveReader *reader);
int32_t kinoko_act_read_document_properties_typed(KinokoActDocument *document,KinokoArchiveReader *reader);
#ifdef __cplusplus
}
#endif
