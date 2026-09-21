#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t retdec_act_make_list(int32_t* slot);
int32_t retdec_act_append_list(int32_t slot,int32_t value);
void kinoko_act_list_drop_storage(int32_t head);
#ifdef __cplusplus
}
#endif
