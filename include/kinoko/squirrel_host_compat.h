#pragma once
#include <stdint.h>
struct SQVM;

#ifdef __cplusplus
extern "C" {
#endif

/* SqPlus external-reference objects occupy 12 bytes (vtable + HSQOBJECT).
   Storage parameters are pointers to possibly unaligned host records, accessed
   via ObjectView/memcpy. Copy/assign retain before release; capture owns an
   external ref; raw setters borrow; destroy consumes the owned reference.
   Type/data words remain integers because the value union is not always a pointer.
   Object-returning APIs return the supplied storage except reset, which returns
   the embedded HSQOBJECT address as the original does. */
int32_t kinoko_squirrel_object_vtable(void);
void * kinoko_sqplus_object_initialize(void * object);
void * kinoko_sqplus_object_copy_construct(void * object, const void * source);
void * kinoko_sqplus_object_construct_value(void * object, int32_t type, int32_t data);
void * kinoko_sqplus_object_reset(void * object);
void * kinoko_sqplus_object_assign(void * object, const void * source);
struct SQVM * kinoko_sqplus_object_append(void * object, const void * source);
int32_t  kinoko_sqplus_object_capture(void * object, int32_t index);
int32_t  kinoko_sqplus_object_is_null(void * object);
int32_t  kinoko_sqplus_object_size(void * object);
int32_t  kinoko_sqplus_object_reverse(void * object);
int32_t  kinoko_sqplus_object_set_index_string(void * object, int32_t key, const char * text);
int32_t  kinoko_sqplus_object_raw_set_object(void * object, const void * key, const void * value);
int32_t  kinoko_sqplus_object_raw_set_name(void * object, const char* key, const void * value);
int32_t  kinoko_sqplus_object_new_userdata(void * object, const char * key, int32_t size, void * tag);
int32_t  kinoko_sqplus_object_type(void * object);
int32_t  kinoko_sqplus_object_get_index_integer(void * object, int32_t key);
const char * kinoko_sqplus_object_get_index_string(void * object, int32_t key);
void * kinoko_sqplus_object_get_index_userpointer(void * object, int32_t key);
void * kinoko_sqplus_object_instance(void * object, void * tag);
int32_t  kinoko_sqplus_object_set_instance(void * object, void * native_pointer);
int32_t  kinoko_sqplus_object_begin_iteration(void * object);
int32_t  kinoko_sqplus_object_next(int32_t* key, int32_t* value);
int32_t  kinoko_sqplus_object_typetag(void * object, int32_t* tag);
int32_t  kinoko_sqplus_object_end_iteration(void);
void*  kinoko_sqplus_object_destroy(void * object);
void * kinoko_sqplus_object_assign_thread(void * object, struct SQVM * thread);
int32_t  kinoko_sqplus_object_set_delegate(void * object, const void * delegate);
int32_t  kinoko_sqplus_object_get_userdata(void * object, const char * key, void * output, void * tag_output);
int32_t  kinoko_sqplus_object_raw_get_userdata(void * object, const char* key, int32_t* output, void * tag_output);
int32_t  kinoko_sqplus_object_exists(void * object, const char* key);
void * kinoko_sqplus_object_get_delegate(void * object, void * output);
void * kinoko_sqplus_object_get_value(void * object, void * output, const char* key);
void * kinoko_sqplus_object_new_instance(void * object, const void * klass);
void * kinoko_sqplus_object_new_table(void * object);
void * kinoko_sqplus_object_new_array(void * object, int32_t size);

#ifdef __cplusplus
}
#endif
