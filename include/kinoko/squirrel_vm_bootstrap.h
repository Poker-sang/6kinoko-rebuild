#pragma once
#include <stdint.h>
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif
int32_t  kinoko_sqplus_release_vm_wrappers(void);
void kinoko_sq_release_owned_states(void);
int32_t  kinoko_sqplus_print(struct SQVM * vm, const char* format, ...);
void * kinoko_sqplus_root_object(void);
int32_t  kinoko_sqplus_select_vm(struct SQVM * vm);
int32_t  kinoko_sqplus_new_instance_adapter(int32_t* object, int32_t* klass);
#ifdef __cplusplus
}
#endif
