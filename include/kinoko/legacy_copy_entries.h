#pragma once
#include <stdint.h>
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error The recovered copy entries require MSVC Win32.
#endif
#ifdef __cplusplus
extern "C" {
#endif
int32_t __fastcall function_43d110(int32_t receiver, void* unused_edx, int32_t source);
int32_t __fastcall function_43cf20(int32_t receiver, void* unused_edx, int32_t source);
int32_t __fastcall function_43e860(int32_t receiver, void* unused_edx, int32_t source);
#ifdef __cplusplus
}
#endif
