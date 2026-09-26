#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_act_make_list(void* slot);
int32_t kinoko_act_append_list(void* slot,void* value);
// Release owned payloads through ISerializable; retain list nodes until member teardown.
void kinoko_act_list_dispose_payloads(void* head);
void kinoko_act_list_drop_storage(void* head);
#ifdef __cplusplus
}
#endif
