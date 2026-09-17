#ifndef KINOKO_SQUIRREL_COMPILE_BRIDGE_H
#define KINOKO_SQUIRREL_COMPILE_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The caller frees the bytecode buffer. No C++ VM objects cross this boundary. */
int32_t retdec_squirrel_compile_source(const char *source, int32_t length,
    const char *name, unsigned char **bytecode, int32_t *bytecode_size);

int32_t kinoko_sq_compile_proto(int32_t vm, int32_t reader, int32_t context,
    const char *name, int32_t out[2], int32_t raiseerror, int32_t lineinfo);
int32_t kinoko_sq_compile_reader(int32_t vm, int32_t reader, int32_t context,
    const char *name, int32_t raiseerror);
int32_t kinoko_sq_compile_buffer(int32_t vm, const char *text, int32_t length,
    const char *name, int32_t raiseerror);
int32_t kinoko_sq_compilestring(int32_t vm);
int32_t kinoko_sq_source_table_vtable(void);

#ifdef __cplusplus
}
#endif
#endif
