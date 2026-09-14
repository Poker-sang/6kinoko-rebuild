#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 40DC0A..40DC20: game-update thread rounds toward positive infinity.
   Set both x87 and SSE so migrated C++ follows the original x87 policy. */
uint32_t kinoko_enter_game_math(void);
void kinoko_leave_game_math(uint32_t previous_rounding);
uint32_t kinoko_run_game_math(uint32_t (__stdcall *update)(void *), void *argument);
#ifdef __cplusplus
}
#endif
