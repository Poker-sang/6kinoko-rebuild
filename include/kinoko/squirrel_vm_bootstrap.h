#pragma once
#include <stdint.h>
struct SQVM;
typedef struct KinokoSqplusVmSlots {
    char *skip_owner_reset;
    void **newest_shared_state;
    struct SQVM **current_vm;
    void **cached_root;
    int32_t *thread_wrapper;
} KinokoSqplusVmSlots;
#ifdef __cplusplus
extern "C" {
#endif
const KinokoSqplusVmSlots *kinoko_sqplus_vm_slots(void);
struct SQVM *kinoko_script_open_primary_vm(int32_t stack_size);
int32_t  kinoko_sqplus_release_vm_wrappers(void);
void kinoko_sq_release_owned_states(void);
int32_t  kinoko_sqplus_print(struct SQVM * vm, const char* format, ...);
void * kinoko_sqplus_root_object(void);
int32_t  kinoko_sqplus_select_vm(struct SQVM * vm);
#ifdef __cplusplus
}
#endif
