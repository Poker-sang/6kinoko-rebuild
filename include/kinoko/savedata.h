#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Native path is borrowed. The three-word SqPlus argument transfers one
// externally retained reference to the callee, including on file failure.
// The runtime registration calls these entries directly.
int32_t kinoko_savedata_load_file_entry(const char* path, int32_t vtable, int32_t type, int32_t data);
int32_t kinoko_savedata_save_file_entry(const char* path, int32_t vtable, int32_t type, int32_t data);

#ifdef __cplusplus
}
#endif
