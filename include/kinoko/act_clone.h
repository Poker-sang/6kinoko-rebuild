#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 427950: owns the returned document; source remains borrowed. */
KinokoActDocument *__fastcall kinoko_act_clone(KinokoActDocument *source, void *unused);
/* Returns true only for uncounted data the caller must free directly.
   Counted data is destroyed by the upstream control's final strong release. */
int32_t kinoko_act_release_chip_data((KinokoActResource*)(uintptr_t)(KinokoActResource* resource));
/* Consume the native texture reference retained by a borrowed virtual clone.
   Returns true when this clone's reference was handled here. */
int32_t kinoko_act_release_cloned_texture((KinokoActResource*)(uintptr_t)(KinokoActResource* resource));
#ifdef __cplusplus
}
#endif
