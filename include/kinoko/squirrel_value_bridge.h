#include "kinoko/squirrel_native_types.h"
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Borrowed addresses from the reconstructed x86 VM. These operations use
   Squirrel 2.2.2 SQObjectPtr and dispatch through each object's own vtable. */
void kinoko_sq_set_error_string(SQVM* vm, SQString* interned_string);
void kinoko_sq_set_error_value(SQVM* vm, const int32_t value[2]);
void kinoko_sq_reset_error(SQVM* vm);
int32_t kinoko_sq_set_native_name(SQVM* vm, int32_t index, const char *name);
void kinoko_sq_assign_integer(SQObjectPtr* object, int32_t value);
void kinoko_sq_assign_float(SQObjectPtr* object, float value);
SQObjectPtr* kinoko_sq_pair_assign(SQObjectPtr* destination, SQObjectPtr* source);
void kinoko_sq_stack_remove(SQVM* vm, int32_t index);
void kinoko_sq_pair_destroy(SQObjectPtr* object);
void kinoko_sq_closure_destroy(SQClosure* closure);
void kinoko_sq_class_destroy(SQClass* klass);
int32_t kinoko_sq_get_vararg(SQVM* vm, SQObjectPtr* target, SQObjectPtr* index, void* call_info);
int32_t kinoko_sq_clone(SQVM* vm, SQObjectPtr* source, SQObjectPtr* target);
int32_t kinoko_sq_foreach(SQVM* vm, SQObjectPtr* object, SQObjectPtr* key, SQObjectPtr* value,
                        SQObjectPtr* iterator, int32_t arg2, int32_t exitpos, int32_t *jump);
int32_t kinoko_sq_generator_yield(SQGenerator* generator, SQVM* vm);
int32_t kinoko_sq_generator_resume(SQGenerator* generator, SQVM* vm, int32_t target);
void kinoko_sq_generator_kill(SQGenerator* generator);
int32_t kinoko_sq_array_remove(SQVM* vm);
int32_t kinoko_sq_array_pop_api(SQVM* vm, int32_t index, int32_t push_value);
int32_t kinoko_sq_array_pop(SQVM* vm);
int32_t kinoko_sq_array_top(SQVM* vm);
int32_t kinoko_sq_suspend(SQVM* vm);
int32_t kinoko_sq_wakeup(SQVM* vm, int32_t wakeupret, int32_t retval, int32_t raiseerror);
int32_t kinoko_sq_thread_call(SQVM* vm);
int32_t kinoko_sq_thread_wakeup(SQVM* vm);
int32_t kinoko_sq_thread_status(SQVM* vm);
int32_t kinoko_sq_newthread(SQVM* vm);

#ifdef __cplusplus
}
#endif
