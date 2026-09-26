#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Original 407210 char iterator range; production ABI and hash width are Win32. */
int32_t kinoko_boost_hash_range(const char* begin, const char* end);
#ifdef __cplusplus
}
#endif
