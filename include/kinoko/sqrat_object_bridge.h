#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered host entry points. Values crossing this C ABI keep their Win32
   bit patterns; implementation uses the vendored Squirrel 2.2.2 source. */
int32_t kinoko_sqrat_object_vtable(void);
int32_t kinoko_sqrat_root_vtable(void);
/* Original thiscall virtual slots: ECX receiver, callee pops output/flags.
   The unused EDX parameter lets MSVC express these entries without assembly. */
int32_t __fastcall kinoko_sqrat_copy_object(int32_t receiver, void *unused, int32_t output);
int32_t __fastcall kinoko_sqrat_object_reference(int32_t receiver, void *unused);
int32_t __fastcall kinoko_sqrat_delete_object(int32_t receiver, void *unused, int32_t flags);
void retdec_sqrat_trim_stack(int32_t vm, int32_t base);
int32_t retdec_sqrat_root_construct(int32_t object_ptr, int32_t vm);
void retdec_sqrat_object_release(int32_t object_ptr);
int32_t retdec_sqrat_get(int32_t object_ptr, const char *name,
    int32_t out_ptr);
void retdec_sqrat_assign_pair(int32_t vm, int32_t* destination, const int32_t* source);
void retdec_sqrat_release_pair(int32_t vm, int32_t pair[2]);
int32_t retdec_sqrat_set_pair(int32_t vm, const int32_t *object_pair,
    const char *name,
    const int32_t *value_pair);
int32_t retdec_sqrat_raw_set_pair(int32_t vm,
    const int32_t *object_pair,
    const char *name,
    const int32_t *value_pair);
int32_t retdec_sqrat_raw_set_int(int32_t vm,
    const int32_t *object_pair,
    const char *name, int32_t value);
int32_t retdec_sqrat_raw_set_float(int32_t vm,
    const int32_t *object_pair,
    const char *name, float value);
int32_t retdec_sqrat_raw_set_bool(int32_t vm,
    const int32_t *object_pair,
    const char *name, int32_t value);
int32_t retdec_sqrat_raw_set_string(int32_t vm,
    const int32_t *object_pair,
    const char *name,
    const char *value);
int32_t retdec_sqrat_set_native_closure(
    int32_t vm, const int32_t *object_pair, const char *name,
    int32_t native_function, const int32_t *free_pair,
    int32_t free_count);
int32_t retdec_sqrat_set_offset_closure(
    int32_t vm, const int32_t *table_pair, const char *name,
    int32_t offset, int32_t callback);
int32_t retdec_sqrat_set_int(int32_t vm, const int32_t *object_pair,
    const char *name, int32_t value);
int32_t retdec_sqrat_set_bool(int32_t vm, const int32_t *object_pair,
    const char *name, int32_t value);
int32_t retdec_sqrat_set_string(int32_t vm, const int32_t *object_pair,
    const char *name, const char *value);
int32_t retdec_sqrat_no_constructor(int32_t vm);
int32_t retdec_sqrat_initialize_class(int32_t vm, const int32_t* type,
    const int32_t* set_table, const int32_t* get_table, int32_t constructor,
    int32_t setter, int32_t getter, int32_t weakref);
int32_t retdec_sqrat_new_class(int32_t vm, int32_t* output);
int32_t retdec_sqrat_new_table(int32_t vm, int32_t out_pair[2]);
int32_t retdec_sqrat_set_delegate(int32_t vm,
    const int32_t *object_pair,
    const int32_t *delegate_pair);
int32_t function_415550_this(int32_t this_ptr, int32_t name,
    int32_t src, int32_t size,
    int32_t native_function,
    int32_t static_slot);
int32_t function_415810_this(int32_t self_ptr);

#ifdef __cplusplus
}
#endif
