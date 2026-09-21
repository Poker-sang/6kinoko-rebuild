#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original 466050 / 466090 / 4660C0 / 466100. Returned addresses remain
   borrowed Win32 ABI tokens; stage cleanup owns published payloads. */
int32_t kinoko_stages_update(void);
int32_t kinoko_stages_prepare_draw(void);
int32_t kinoko_stages_draw(void);
int32_t kinoko_stage_load(const char *file_name);
#ifdef __cplusplus
}
#endif
