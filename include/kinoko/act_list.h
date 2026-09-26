#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_act_make_list(int32_t* slot);
int32_t kinoko_act_append_list(int32_t slot,int32_t value);
// Release owned payloads through ISerializable; retain list nodes until member teardown.
void kinoko_act_list_dispose_payloads(int32_t head);
void kinoko_act_list_drop_storage(int32_t head);
#ifdef __cplusplus
}
#endif
