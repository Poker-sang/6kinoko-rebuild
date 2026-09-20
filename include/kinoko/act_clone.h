#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t __fastcall kinoko_act_clone(int32_t source, void *unused);
/* Returns true only for uncounted data the caller must free directly.
   Counted data is destroyed by the upstream control's final strong release. */
int32_t kinoko_act_release_chip_data(int32_t resource);
/* Consume the native texture reference retained by a borrowed virtual clone.
   Returns true when this clone's reference was handled here. */
int32_t kinoko_act_release_cloned_texture(int32_t resource);
#ifdef __cplusplus
}
#endif
