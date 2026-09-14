// Negative control only: reproduce the old BGM reset with no restoration.
#include "kinoko/audio_math.h"
#include <float.h>
extern "C" int kinoko_prepare_audio(kinoko_prepare_audio_fn prepare,
    uint32_t handle, const char *path, int looping, float volume) noexcept(false) {
    _fpreset();
    return prepare(handle, path, looping, volume);
}
