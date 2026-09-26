#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Word-aligned flat game records owned by std::vector<uint32_t>.
// begin/end are borrowed; word three stores the opaque owner.
// Grow storage without changing the published logical end.
int32_t kinoko_native_buffer_ensure(void* slot,uint32_t bytes);
int32_t kinoko_native_buffer_resize(void* slot,uint32_t bytes);
void kinoko_native_buffer_replace(void* slot,const void* data,uint32_t bytes);
void kinoko_native_buffer_destroy(void* slot);
#ifdef __cplusplus
}
#endif
