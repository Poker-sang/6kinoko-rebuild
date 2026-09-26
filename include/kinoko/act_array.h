#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// begin/end are borrowed views; the third word owns an opaque std::vector.
int32_t kinoko_act_array_prepare(void* slot, uint32_t count);
void kinoko_act_array_clone(void* destination, const void* source);
void kinoko_act_array_append(void* slot, void* value);
void kinoko_act_array_destroy(void* slot);
#ifdef __cplusplus
}
#endif
