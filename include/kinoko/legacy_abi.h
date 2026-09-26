#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* C entry points for the original x86 virtual-method ABI. */
void kinoko_call_thiscall0(void *object, void *method);
void kinoko_call_thiscall1(void *object, void *method, int32_t argument);
int32_t kinoko_call_thiscall0_result(void *object, void *method);
int32_t kinoko_call_thiscall1_result(void *object, void *method, int32_t argument);
int32_t kinoko_call_thiscall2_result(void *object, void *method,
    int32_t argument1, int32_t argument2);
int32_t kinoko_call_thiscall3_result(void *object, void *method,
    int32_t argument1, int32_t argument2, int32_t argument3);
int32_t kinoko_call_thiscall4_result(void *object, void *method,
    int32_t argument1, int32_t argument2, int32_t argument3, int32_t argument4);
int32_t kinoko_call_draw_method(void *object, void *method,
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t resource,
    int32_t source_x, int32_t source_y, int32_t blend, float alpha);

#ifdef __cplusplus
}
#endif
