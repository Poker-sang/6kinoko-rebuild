#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The game's 24-byte Win32 string record is not today's std::string.
   These functions borrow caller-owned, possibly unaligned record storage. */
const char* retdec_std_string_data(int32_t object);
uint32_t retdec_safe_c_string_length(const char* source);
int32_t retdec_string_assign_n(int32_t* object, const char* source, uint32_t size);
int32_t retdec_string_assign_cstr(int32_t* object, const char* source);

/* Original-address exports retained only for the remaining C/ABI callers. */
int32_t function_4038c0(int32_t object, const char* source, uint32_t size);
int32_t function_4039e0(int32_t object, uint32_t capacity, int32_t shrink);
int32_t function_403bf0(int32_t object, int32_t source, uint32_t position, uint32_t size);
int32_t function_403ce0(int32_t object, uint32_t capacity, uint32_t old_length);

#ifdef __cplusplus
}
#endif
