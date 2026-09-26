#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Typed C boundary for recovered REP operations and the FPR scratch bank.
 * Nonpositive counts perform no access. Counts for MOVSD/STOSD are DWORDs,
 * not bytes. The scratch bank retains the original unsigned modulo mapping. */
int64_t __asm_rep_movsb_memcpy(void *destination, const void *source, int32_t count);
int64_t __asm_rep_movsd_memcpy(void *destination, const void *source, int32_t count);
int64_t __asm_rep_stosb_memset(void *destination, int32_t value, int32_t count);
int64_t __asm_rep_stosd_memset(void *destination, int32_t value, int32_t count);
long double __frontend_reg_load_fpr(int32_t reg);
void __frontend_reg_store_fpr(int32_t reg, long double value);

#ifdef __cplusplus
}
#endif
