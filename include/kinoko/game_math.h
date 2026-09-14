#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 40DC0A..40DC20: game-update thread rounds toward positive infinity.
   Set both x87 and SSE so migrated C++ follows the original x87 policy. */
#ifdef KINOKO_MATH_AUDIT
void kinoko_math_checkpoint(const char *phase, uint32_t detail);
#else
#define kinoko_math_checkpoint(phase, detail) ((void)0)
#endif
uint32_t kinoko_enter_game_math(void);
void kinoko_leave_game_math(uint32_t previous_rounding);
unsigned long kinoko_run_game_math(unsigned long (__stdcall *update)(void *), void *argument)
#ifdef __cplusplus
    noexcept(false)
#endif
    ;
#ifdef __cplusplus
}
#endif
