#pragma once
#include <stdint.h>
struct SQVM;

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered host entry points. Values crossing this C ABI keep their Win32
   bit patterns; implementation uses the vendored Squirrel 2.2.2 source. */
const void* kinoko_sqrat_object_vtable(void);
const void* kinoko_sqrat_root_vtable(void);
/* Original thiscall virtual slots: ECX receiver, callee pops output/flags.
   The unused EDX parameter lets MSVC express these entries without assembly. */
void * __fastcall kinoko_sqrat_copy_object(void * receiver, void *unused, void * output);
void * __fastcall kinoko_sqrat_object_reference(void * receiver, void *unused);
void * __fastcall kinoko_sqrat_delete_object(void * receiver, void *unused, int32_t flags);
void  kinoko_sqrat_trim_stack(struct SQVM * vm, int32_t base);
void * kinoko_sqrat_root_construct(void * object_ptr, struct SQVM * vm);
void  kinoko_sqrat_object_release(void * object_ptr);
int32_t  kinoko_sqrat_get(void * object_ptr, const char *name, void * out_ptr);
void  kinoko_sqrat_retain_pair(struct SQVM * vm, const int32_t pair[2]);
void  kinoko_sqrat_assign_pair(struct SQVM * vm, int32_t* destination, const int32_t* source);
void  kinoko_sqrat_release_pair(struct SQVM * vm, int32_t pair[2]);
int32_t  kinoko_sqrat_set_pair(struct SQVM * vm, const int32_t *object_pair, const char *name, const int32_t *value_pair);
int32_t  kinoko_sqrat_raw_set_pair(struct SQVM * vm, const int32_t *object_pair, const char *name, const int32_t *value_pair);
int32_t  kinoko_sqrat_raw_set_int(struct SQVM * vm, const int32_t *object_pair, const char *name, int32_t value);
int32_t  kinoko_sqrat_raw_set_float(struct SQVM * vm, const int32_t *object_pair, const char *name, float value);
int32_t  kinoko_sqrat_raw_set_bool(struct SQVM * vm, const int32_t *object_pair, const char *name, int32_t value);
int32_t  kinoko_sqrat_raw_set_string(struct SQVM * vm, const int32_t *object_pair, const char *name, const char *value);
int32_t  kinoko_sqrat_set_native_closure(struct SQVM * vm, const int32_t *object_pair, const char *name, void * native_function, const int32_t *free_pair, int32_t free_count);
int32_t  kinoko_sqrat_set_offset_closure(struct SQVM * vm, const int32_t *table_pair, const char *name, int32_t offset, void * callback);
int32_t  kinoko_sqrat_bind_int(struct SQVM * vm, const int32_t *object_pair, const char *name, int32_t value);
int32_t  kinoko_sqrat_bind_bool(struct SQVM * vm, const int32_t *object_pair, const char *name, int32_t value);
int32_t  kinoko_sqrat_bind_string(struct SQVM * vm, const int32_t *object_pair, const char *name, const char *value);
int32_t  kinoko_sqrat_no_constructor(struct SQVM * vm);
int32_t  kinoko_sqrat_initialize_class(struct SQVM * vm, const int32_t* type, const int32_t* set_table, const int32_t* get_table, void * constructor, void * setter, void * getter, void * weakref);
int32_t  kinoko_sqrat_new_class(struct SQVM * vm, int32_t* output);
int32_t  kinoko_sqrat_new_table(struct SQVM * vm, int32_t out_pair[2]);
int32_t  kinoko_sqrat_set_delegate(struct SQVM * vm, const int32_t *object_pair, const int32_t *delegate_pair);
struct SQVM * kinoko_sqrat_bind_object_function(void * this_ptr, const char * name, const void * src, int32_t size, void * native_function, int32_t static_slot);
int32_t  kinoko_sqrat_invoke_callback(const void * self_ptr);

#ifdef __cplusplus
}
#endif
