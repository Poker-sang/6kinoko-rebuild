#pragma once
#include <stdint.h>
// Native path is borrowed. The three-word SqPlus argument transfers one
// externally retained reference to the callee, including on file failure.
// Only the four original registration aliases keep integer address arguments.
int32_t kinoko_savedata_load_file_entry(const char* path, int32_t vtable, int32_t type, int32_t data);
int32_t kinoko_savedata_save_file_entry(const char* path, int32_t vtable, int32_t type, int32_t data);
