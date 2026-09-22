#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* environment is a borrowed 12-byte SqPlus ObjectStorage, or NULL. */
int32_t kinoko_script_load_file(const char* path, const void* environment);
void* kinoko_script_initialize_root(void);
void* kinoko_script_root(void);
int32_t kinoko_script_close_vm(void);
int32_t kinoko_script_show_call_stack(void);
/* Original x86 by-value Sqrat Object argument, including its ownership flag. */
int32_t kinoko_script_compile_file_argument(int32_t path, int32_t vtable,
    int32_t vm, int32_t type, int32_t data, char owns_reference);
#ifdef __cplusplus
}
#endif
