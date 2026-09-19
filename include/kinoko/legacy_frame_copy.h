#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Temporary x86 register-ABI compatibility, NOT a recovered memcpy signature.
   Ordinary/new code must use memcpy with explicit operands instead. */
int32_t _memcpy2(void);
uintptr_t kinoko_legacy_caller_ebp(void);
int32_t kinoko_legacy_copy_from_frame(uintptr_t caller_frame);
#ifdef __cplusplus
}
#endif
