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
int32_t kinoko_sq_pair_assign(int32_t destination, int32_t source);
void kinoko_sq_pair_destroy(int32_t object);
void kinoko_sq_closure_destroy(int32_t closure);
void kinoko_sq_class_destroy(int32_t klass);
int32_t kinoko_sq_get_vararg(int32_t vm, int32_t target, int32_t index, int32_t call_info);
int32_t kinoko_sq_clone(int32_t vm, int32_t source, int32_t target);
int32_t kinoko_sq_foreach(int32_t vm, int32_t object, int32_t key, int32_t value,
                        int32_t iterator, int32_t arg2, int32_t exitpos, int32_t *jump);
int32_t kinoko_sq_generator_yield(int32_t generator, int32_t vm);
int32_t kinoko_sq_generator_resume(int32_t generator, int32_t vm, int32_t target);
void kinoko_sq_generator_kill(int32_t generator);
int32_t kinoko_sq_array_remove(int32_t vm);
int32_t kinoko_sq_array_pop_api(int32_t vm, int32_t index, int32_t push_value);
int32_t kinoko_sq_array_pop(int32_t vm);
int32_t kinoko_sq_array_top(int32_t vm);

#ifdef __cplusplus
}
#endif
