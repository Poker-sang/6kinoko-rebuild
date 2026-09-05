#ifndef KINOKO_SQUIRREL_VM_BRIDGE_H
#define KINOKO_SQUIRREL_VM_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returned when the raw RetDec object cannot be safely passed to C++. */
#define RETDEC_SQUIRREL_CPP_UNSUPPORTED (-1)

int32_t retdec_squirrel_execute_cpp(
    int32_t vm,
    int32_t closure,
    int32_t target,
    int32_t nargs,
    int32_t stackbase,
    int32_t outres,
    int32_t raiseerror,
    int32_t execution_type);

#ifdef __cplusplus
}
#endif

#endif
