#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Word-aligned flat game records owned by std::vector<uint32_t>.
// begin/end are borrowed; word three stores the opaque owner.
int32_t kinoko_native_buffer_resize(int32_t slot,uint32_t bytes);
void kinoko_native_buffer_replace(int32_t slot,const void* data,uint32_t bytes);
void kinoko_native_buffer_destroy(int32_t slot);
#ifdef __cplusplus
}
#endif
