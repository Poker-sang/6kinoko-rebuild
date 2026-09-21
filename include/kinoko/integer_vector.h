#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_integer_vector_construct(int32_t slot);
void kinoko_integer_vector_destroy(int32_t slot);
void kinoko_integer_vector_clear(int32_t slot);
uint32_t kinoko_integer_vector_size(int32_t slot);
int32_t kinoko_integer_vector_data(int32_t slot);
void kinoko_integer_vector_append(int32_t slot,int32_t value);
#ifdef __cplusplus
}
#endif
