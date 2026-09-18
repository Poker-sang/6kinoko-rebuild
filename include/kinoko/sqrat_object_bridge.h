#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recovered host entry points. Values crossing this C ABI keep their Win32
   bit patterns; implementation uses the vendored Squirrel 2.2.2 source. */
int32_t kinoko_sqrat_object_vtable(void);
int32_t kinoko_sqrat_root_vtable(void);
void retdec_sqrat_trim_stack(int32_t vm, int32_t base);
int32_t retdec_sqrat_root_construct(int32_t object_ptr, int32_t vm);
void retdec_sqrat_object_release(int32_t object_ptr);
int32_t retdec_sqrat_get(int32_t object_ptr, const char *name,
    int32_t out_ptr);
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
