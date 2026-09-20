#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_seed_random(uint32_t seed);
void kinoko_decode_archive_index(uint8_t *bytes, uint32_t size);
#ifdef __cplusplus
}
#endif
