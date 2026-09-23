#pragma once
#include <stdint.h>
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif
struct SQVM *kinoko_script_open_primary_vm(int32_t stack_size);
int32_t  kinoko_sqplus_release_vm_wrappers(void);
void kinoko_sq_release_owned_states(void);
int32_t  kinoko_sqplus_print(struct SQVM * vm, const char* format, ...);
void * kinoko_sqplus_root_object(void);
int32_t  kinoko_sqplus_select_vm(struct SQVM * vm);
#ifdef __cplusplus
}
#endif
