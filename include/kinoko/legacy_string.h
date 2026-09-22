#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The game's 24-byte Win32 string record is not today's std::string.
   These functions borrow caller-owned, possibly unaligned record storage. */
void kinoko_string_destroy(int32_t object);
const char* retdec_std_string_data(int32_t object);
uint32_t retdec_safe_c_string_length(const char* source);
int32_t retdec_string_assign_n(int32_t* object, const char* source, uint32_t size);
int32_t retdec_string_assign_cstr(int32_t* object, const char* source);
int32_t kinoko_string_assign_substring(int32_t object, int32_t source,
    uint32_t position, uint32_t size);

/* Native ownership stays inside StringView; returns borrow the record/buffer. */
void* kinoko_string_append_n(void* object, const char* source, uint32_t size);
int32_t kinoko_string_reserve(void* object, uint32_t capacity, int32_t shrink);
void* kinoko_string_append_substring(void* object, const void* source, uint32_t position, uint32_t size);
char* kinoko_string_grow(void* object, uint32_t capacity, uint32_t old_length);

#ifdef __cplusplus
}
#endif
