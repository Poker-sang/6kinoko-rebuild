#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_actor_owner_list_construct(int32_t manager);
uint32_t kinoko_actor_owner_list_size(int32_t manager);
void kinoko_actor_owner_list_clear(int32_t manager);
int32_t function_46aa60_this(int32_t manager);
int32_t __fastcall kinoko_method_actor_owner_delete(int32_t manager, void *unused, unsigned char flags);
#ifdef __cplusplus
}
#endif
