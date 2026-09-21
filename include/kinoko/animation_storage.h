#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_animation_list_construct(int32_t list);
void kinoko_animation_list_destroy(int32_t list);
int32_t kinoko_animation_create(uint32_t frames);
void kinoko_animation_discard(int32_t animation);
void kinoko_animation_adopt(int32_t list,int32_t animation);
int32_t kinoko_clear_animation_list(int32_t list);
#ifdef __cplusplus
}
#endif
