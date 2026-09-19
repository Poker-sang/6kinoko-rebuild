#pragma once
#include <stdint.h>
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* C ABI entry points only; state and resources are owned by audio_runtime.cpp. */
void retdec_bgm_update_fade(void);
int32_t function_40a0f0(void);
int32_t function_40a3d0(void);
int32_t function_40a460(void);
int32_t function_40a5d0(int32_t result);
int32_t function_40a9f0(long double a1);
int32_t function_40aaa0(void);
int32_t function_40aac0(void);
int32_t function_40aae0(void);
int32_t function_40adb0(int32_t* storage);
int32_t function_40b3a0(void);
int32_t function_40b520(void);
int32_t function_40b8a0(long double a1);
int32_t function_411d80(HWND hwnd, int32_t options);
int32_t function_411f90(void);
int32_t function_4701e0(void);
int32_t function_470220(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t function_470290(int32_t a1, int32_t a2, int32_t a3, int32_t a4,
                        int32_t a5);
int32_t function_470300(void);
int32_t function_470320(int32_t a1, int32_t a2);
int32_t function_470360(void);
int32_t function_470980(int32_t id);
int32_t function_470ab0(int32_t a1);
#ifdef __cplusplus
}
#endif
