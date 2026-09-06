#ifndef KINOKO_SQUIRREL_COMPILE_BRIDGE_H
#define KINOKO_SQUIRREL_COMPILE_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The caller frees the bytecode buffer. No C++ VM objects cross this boundary. */
int32_t retdec_squirrel_compile_source(const char *source, int32_t length,
    const char *name, unsigned char **bytecode, int32_t *bytecode_size);

#ifdef __cplusplus
}
#endif
#endif
