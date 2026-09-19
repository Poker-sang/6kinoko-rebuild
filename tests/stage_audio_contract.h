#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Test-only entry points; no audio state is exported by the production host. */
int kinoko_test_sound_cleanup(int32_t (*clear_all)(void));
int kinoko_test_bgm_preserves_game_math(void);
int kinoko_test_sound_module(void);
#ifdef __cplusplus
}
#endif
