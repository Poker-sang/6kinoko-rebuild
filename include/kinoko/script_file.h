#pragma once
#include <stdint.h>
#ifdef __cplusplus
#define KINOKO_SCRIPT_MAY_THROW noexcept(false)
extern "C" {
#else
#define KINOKO_SCRIPT_MAY_THROW
#endif
/* environment is a borrowed 12-byte SqPlus ObjectStorage, or NULL. */
int32_t kinoko_script_load_file(const char* path, const void* environment) KINOKO_SCRIPT_MAY_THROW;
void* kinoko_script_initialize_root(void) KINOKO_SCRIPT_MAY_THROW;
void* kinoko_script_root(void);
int32_t kinoko_script_close_vm(void);
int32_t kinoko_script_show_call_stack(void) KINOKO_SCRIPT_MAY_THROW;
/* Original x86 by-value Sqrat Object argument, including its ownership flag. */
int32_t kinoko_script_compile_file_argument(int32_t path, int32_t vtable,
    int32_t vm, int32_t type, int32_t data, char owns_reference) KINOKO_SCRIPT_MAY_THROW;
#ifdef __cplusplus
}
#endif

#undef KINOKO_SCRIPT_MAY_THROW
