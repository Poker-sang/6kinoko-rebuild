#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 455880 borrows the document. 455E40 returns a separately owned runtime;
   it does not consume the holder or document. */
KinokoActSourceHolder *kinoko_act_source_initialize(
    KinokoActSourceHolder *holder, KinokoActDocument *document);
int32_t kinoko_act_source_layer_count(const KinokoActSourceHolder *holder);
KinokoActRuntime *kinoko_act_source_create_runtime(KinokoActSourceHolder *holder);
#ifdef __cplusplus
}
#endif
