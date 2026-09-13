#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t __fastcall kinoko_act_clone(int32_t source, void *unused);
/* Returns true when the shared decoded chip data has no remaining owners. */
int32_t kinoko_act_release_chip_data(int32_t resource);
#ifdef __cplusplus
}
#endif
