#pragma once
#include <windows.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_frame_timer_initialize(void)
#ifdef __cplusplus
    noexcept(false)
#endif
    ;
HANDLE kinoko_frame_timer_register(void);
int32_t kinoko_frame_timer_unregister(HANDLE event);
void kinoko_frame_timer_wait(HANDLE event);
#ifdef __cplusplus
}
#endif
