#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original 466050 / 466090 / 4660C0 / 466100. The load result is borrowed
   after publication; stage cleanup owns published payloads. Without an
   initialized stage list the returned owner is transferred to the caller. */
int32_t kinoko_stages_update(void);
int32_t kinoko_stages_prepare_draw(void);
int32_t kinoko_stages_draw(void);
KinokoStageOwner *kinoko_stage_load(const char *file_name);
#ifdef __cplusplus
}
#endif
