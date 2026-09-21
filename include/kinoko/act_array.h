#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// begin/end are borrowed views; the third word owns an opaque std::vector.
int32_t kinoko_act_array_prepare(int32_t slot, uint32_t count);
void kinoko_act_array_clone(int32_t destination, int32_t source);
void kinoko_act_array_append(int32_t slot, int32_t value);
void kinoko_act_array_destroy(int32_t slot);
#ifdef __cplusplus
}
#endif
