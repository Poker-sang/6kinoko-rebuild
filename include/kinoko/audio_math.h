#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* BGM preparation uses the decoder's default floating environment without
   changing the game thread's original upward rounding on return. */
typedef int (*kinoko_prepare_audio_fn)(uint32_t, const char *, int, float);
int kinoko_prepare_audio(kinoko_prepare_audio_fn prepare, uint32_t handle,
                         const char *path, int looping, float volume)
#ifdef __cplusplus
    noexcept(false)
#endif
    ;
#ifdef __cplusplus
}
#endif
