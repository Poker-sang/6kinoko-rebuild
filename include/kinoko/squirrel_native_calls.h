#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Legacy game callbacks; all VM objects and captures are read by source APIs. */
int32_t function_41e260(int32_t vm);
int32_t function_41e2c0(int32_t vm);
int32_t function_431650(int32_t vm);
int32_t function_445730(int32_t vm);
int32_t function_4552e0(int32_t vm);
int32_t function_4555a0(int32_t vm);

/* Explicit receiver + cdecl target (not the x86 thiscall method family). */
int32_t function_46b490(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t function_46b500(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t function_46b610(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t function_46b6f0(int32_t self, int32_t target, int32_t vm, int32_t first);
void function_46c6b0_pair(int32_t vm, int32_t pair[2]);
int32_t function_46ce70(int32_t vm);
int32_t function_46cec0(int32_t vm);
int32_t function_46cf10(int32_t vm);
int32_t function_46cf60(int32_t vm);

int32_t function_4716b0(int32_t callback, int32_t vm, int32_t index);
int32_t function_471a60(int32_t callback, int32_t vm, int32_t index);
int32_t function_471720(int32_t callback, int32_t vm, int32_t index);
int32_t function_471d30(int32_t vm);
int32_t function_471e50(int32_t vm);
int32_t function_471f70(int32_t vm);
int32_t function_472030(int32_t vm);
#ifdef __cplusplus
}
#endif
