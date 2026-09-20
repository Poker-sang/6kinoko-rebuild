#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Original 407210 char iterator range; production ABI and hash width are Win32. */
int32_t kinoko_boost_hash_range(int32_t begin, int32_t end);
#ifdef __cplusplus
}
#endif
