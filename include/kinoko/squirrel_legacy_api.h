// Recovered address names are retained only at the C/game ABI boundary.
// Implementations live in squirrel_legacy_api.cpp and use vendored 2.2.2.
#pragma once
#include <stdint.h>
#include <squirrel.h>
typedef struct SQVM SQVM;
typedef struct SQSharedState SQSharedState;
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_vm_lifecycle.h"
#ifdef __cplusplus
struct SQObjectPtr; struct SQDelegable; struct SQTable; struct SQUserData;
#else
typedef struct SQObjectPtr SQObjectPtr;
typedef struct SQDelegable SQDelegable;
typedef struct SQTable SQTable;
typedef struct SQUserData SQUserData;
#endif
#ifdef __cplusplus
extern "C" {
#endif
HSQOBJECT* kinoko_sq_add_object_reference(SQVM* machine, HSQOBJECT* object);
SQUserData* kinoko_sq_create_user_data(SQSharedState* shared, int32_t bytes);
int32_t kinoko_sq_array_pop(SQVM* machine, int32_t index, int32_t push_value);
SQObjectPtr* kinoko_sq_assign_float(SQObjectPtr* destination, float value);
SQObjectPtr* kinoko_sq_assign_integer(SQObjectPtr* destination, int32_t value);
SQObjectPtr* kinoko_sq_assign_object(SQObjectPtr* destination, const SQObjectPtr* source);
int32_t kinoko_sq_collect_garbage(SQVM* machine, int32_t a2, int32_t a3);
int32_t kinoko_sq_compile_buffer(SQVM* machine, int32_t text, int32_t length, int32_t *name, int32_t raiseerror);
int32_t kinoko_sq_compile_lexed(SQVM* machine, int32_t reader, int32_t *context, int32_t name, int32_t raiseerror);
int32_t kinoko_sq_create_instance(SQVM* machine, int32_t a2);
SQObjectPtr* kinoko_sq_destroy_object(SQObjectPtr* value);
int32_t kinoko_sq_destroy_userdata_entry(int32_t object, void* unused, int32_t flags);
int32_t kinoko_sq_get_instance_pointer(SQVM* machine, int32_t index, void** output, void* tag);
int32_t kinoko_sq_get_integer(SQVM* machine, int32_t a2, int32_t * a3);
SQObjectPtr* kinoko_sq_get_last_error(SQVM* machine);
int32_t kinoko_sq_get_print_function(SQVM* machine);
int32_t kinoko_sq_get_slot(SQVM* machine, int32_t a2);
int32_t kinoko_sq_get_stack_object(SQVM* machine, int32_t index, HSQOBJECT* output);
int32_t kinoko_sq_get_stack_top(SQVM* machine);
int32_t kinoko_sq_get_type(SQVM* machine, int32_t a2);
int32_t kinoko_sq_get_user_pointer(SQVM* machine, int32_t index, void** output);
int32_t kinoko_sq_install_error_handlers(SQVM* machine);
SQObjectPtr* kinoko_sq_move_object(SQVM* machine, SQVM* source, int32_t count);
int32_t kinoko_sq_new_class(SQVM* machine, int32_t a2);
SQObjectPtr* kinoko_sq_new_closure(SQVM* machine, SQFUNCTION callback, int32_t free_variables);
SQObjectPtr* kinoko_sq_push_integer(SQVM* machine, int32_t a2);
SQObjectPtr* kinoko_sq_push_new_table(SQVM* machine);
SQObjectPtr* kinoko_sq_push_raw_object(SQVM* machine, int32_t a2, int32_t a3);
SQObjectPtr* kinoko_sq_push_root_table(SQVM* machine);
SQObjectPtr* kinoko_sq_push_string(SQVM* machine, const char* text, int32_t length);
SQObjectPtr* kinoko_sq_push_user_pointer(SQVM* machine, void* value);
SQVM* kinoko_sq_raise_object_error(SQVM* machine, SQObjectPtr* error_value);
int32_t kinoko_sq_raw_get_slot(SQVM* machine, int32_t a2);
int32_t kinoko_sq_read_closure(SQVM* machine, SQREADFUNC reader, SQUserPointer context);
int32_t kinoko_sq_register_blob_library(SQVM* machine);
int32_t kinoko_sq_register_io_library(SQVM* machine);
int32_t kinoko_sq_register_math_library(SQVM* machine);
int32_t kinoko_sq_register_string_library(SQVM* machine);
int32_t kinoko_sq_release_object_reference(SQVM* machine, HSQOBJECT* object);
SQVM* kinoko_sq_reset_error(SQVM* machine);
HSQOBJECT* kinoko_sq_reset_object(HSQOBJECT* object);
int32_t kinoko_sq_set_closure_name(SQVM* machine, int32_t index, const char* name);
SQVM* kinoko_sq_set_compiler_error_handler(SQVM* machine, SQCOMPILERERROR callback);
int32_t kinoko_sq_set_instance_pointer(SQVM* machine, int32_t index, void* instance);
int32_t kinoko_sq_set_object_delegate(SQDelegable* receiver, SQTable* delegate);
SQVM* kinoko_sq_set_print_function(SQVM* machine, SQPRINTFUNCTION callback);
SQVM* kinoko_sq_set_stack_top(SQVM* machine, uint32_t a2);
int32_t kinoko_sq_set_type_tag(SQVM* machine, int32_t index, void* tag);
int32_t kinoko_sq_throw_error(SQVM* machine, char * a2);
int32_t kinoko_sq_write_closure(SQVM* machine, SQWRITEFUNC writer, SQUserPointer context);
void kinoko_sq_finalize_userdata_entry(int32_t object, void* unused);

int32_t kinoko_sq_raise_formatted_error(int32_t this_ptr, const char *format, ...);
void kinoko_squirrel_addref(int32_t type, int32_t data);
void kinoko_squirrel_release(int32_t type, int32_t data);
void kinoko_squirrel_assign(int32_t *dst, const int32_t *src);
void kinoko_release_squirrel_value(int32_t *value_ptr);
int32_t kinoko_gc_object_type(int32_t object_ptr);
void kinoko_gc_mark_value(const int32_t *value, int32_t *chain_head);
void kinoko_gc_finalize_collectable(int32_t object_ptr,
                                            int32_t object_type);
#ifdef __cplusplus
}
#endif
