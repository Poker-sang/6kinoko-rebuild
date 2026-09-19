#pragma once

#include <stdint.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include <sqstdmath.h>
#include <sqstdstring.h>

/* Representation conversions at the remaining generated Win32 C boundary.
 * These functions neither own objects nor implement VM operations. Call the
 * vendored Squirrel API directly after decoding a recovered address/value.
 * Do not replace internal SQObjectPtr ownership with external sq_addref.
 */
static inline void* kinoko_pointer(int32_t address) {
    return (void*)(uintptr_t)(uint32_t)address;
}
static inline HSQUIRRELVM kinoko_vm(int32_t address) {
    return (HSQUIRRELVM)kinoko_pointer(address);
}
static inline HSQOBJECT kinoko_borrowed_object(int32_t type, int32_t bits) {
    HSQOBJECT value;
    value._type = (SQObjectType)type;
    memcpy(&value._unVal, &bits, sizeof(bits));
    return value;
}
static inline SQFloat kinoko_float_bits(int32_t bits) {
    SQFloat value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

#ifdef __cplusplus
static_assert(sizeof(void*) == 4 && sizeof(HSQOBJECT) == 8,
    "Recovered addresses are the original Win32 ABI, not portable pointers.");
#endif
