#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_construct_string_layout(int32_t object);
void kinoko_clear_string_layout(int32_t object);
int32_t __fastcall kinoko_method_delete_string_layout(int32_t object, void *unused, unsigned char flags);
#ifdef __cplusplus
}
#endif
