#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_integer_map_create(void);
void kinoko_integer_map_destroy(int32_t map);
void kinoko_integer_map_clear(int32_t map);
uint32_t kinoko_integer_map_size(int32_t map);
int32_t kinoko_integer_map_put(int32_t map,int32_t key,int32_t value);
int32_t kinoko_integer_map_find(int32_t map,int32_t key);
int32_t function_4706c0_this(int32_t tree,int32_t* entry,int32_t* key);
#ifdef __cplusplus
}
#endif
