#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Borrowed addresses from the reconstructed x86 VM. These operations use
   Squirrel 2.2.2 SQObjectPtr and dispatch through each object's own vtable. */
void kinoko_sq_set_error_string(int32_t vm, int32_t interned_string);
void kinoko_sq_set_error_value(int32_t vm, const int32_t value[2]);
void kinoko_sq_reset_error(int32_t vm);
void kinoko_sq_assign_integer(int32_t object, int32_t value);
void kinoko_sq_assign_float(int32_t object, float value);
int32_t kinoko_sq_generator_yield(int32_t generator, int32_t vm);
int32_t kinoko_sq_generator_resume(int32_t generator, int32_t vm, int32_t target);
void kinoko_sq_generator_kill(int32_t generator);
int32_t kinoko_sq_array_remove(int32_t vm);

#ifdef __cplusplus
}
#endif
