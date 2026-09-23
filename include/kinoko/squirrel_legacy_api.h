// Recovered address names are retained only at the C/game ABI boundary.
// Implementations live in squirrel_legacy_api.cpp and use vendored 2.2.2.
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t function_48a230(int32_t a1, int32_t a2);
int32_t function_48a400(int32_t a1, int32_t a2);
int32_t function_48a430(int32_t a1, int32_t a2);
int32_t function_48a480(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48a4f0(int32_t a1, int32_t a2);
int32_t function_48a5c0(int32_t a1, int32_t a2);
int32_t function_48a600(int32_t a1);
int32_t function_48a670(int32_t a1);
int32_t function_48a6f0(int32_t a1, int32_t a2);
int32_t function_48a7d0(int32_t a1, int32_t a2, int32_t * a3);
int32_t function_48a9e0(int32_t a1, int32_t a2, int32_t * a3);
int32_t function_48aa20(int32_t a1);
int32_t function_48aa30(int32_t a1, int32_t a2);
int32_t function_48aa50(int32_t a1);
int32_t function_48ab40(int32_t a1, int32_t a2, int32_t * a3);
int32_t function_48ab90(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48abe0(int32_t result);
int32_t function_48ac00(int32_t a1, char * a2);
int32_t function_48ac70(int32_t a1);
int32_t function_48acc0(int32_t a1);
int32_t function_48ace0(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t function_48afa0(int32_t result, int32_t a2);
int32_t function_48afc0(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48b050(int32_t a1, int32_t a2, int32_t * a3);
int32_t function_48b180(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48b490(int32_t a1, int32_t a2);
int32_t function_48b870(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48b8b0(int32_t result, int32_t a2);
int32_t function_48b8d0(int32_t a1);
int32_t function_48c1f0(int32_t vm, int32_t reader, int32_t *context, int32_t name, int32_t raiseerror);
int32_t function_48c350(int32_t a1, int32_t a2);
int32_t function_48c580(int32_t vm, int32_t index, int32_t name);
int32_t function_48c780(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48c840(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48c890(int32_t a1, int32_t a2, int32_t * a3, int32_t a4);
int32_t function_48c910(int32_t a1, uint32_t a2);
int32_t function_48ce00(int32_t a1, int32_t a2);
int32_t function_48ce70(int32_t a1, int32_t a2);
int32_t function_48d0b0(int32_t vm, int32_t text, int32_t length, int32_t *name, int32_t raiseerror);
int32_t function_48d850(int32_t a1, int32_t a2, int32_t a3);
int32_t function_48dd10(int32_t vm, int32_t index, int32_t push_value);
int32_t function_48bec0(int32_t a1, int32_t a2);
int32_t function_4c7c90(int32_t a1);
int32_t function_4c73a0(int32_t a1);
int32_t function_4c6c20(int32_t a1);
int32_t function_4c6670(int32_t a1);
int32_t function_4c5c80(int32_t vm);
int32_t function_48e520_this(int32_t this_ptr, int32_t delegate_ptr);
int32_t function_491880_this(int32_t this_ptr, int32_t index);

int32_t function_489f30_this(int32_t this_ptr);
int32_t function_489f50_this(int32_t this_ptr, int32_t source_ptr);
int32_t function_48e0e0_this(int32_t this_ptr, int32_t value);
int32_t function_48e120_this(int32_t this_ptr, float value);
int32_t function_499b00(int32_t this_ptr, int32_t value_ptr);
int32_t kinoko_sq_raise_formatted_error(int32_t this_ptr, const char *format, ...);
int32_t function_49a520_this(int32_t shared_state, int32_t vm);
void retdec_squirrel_addref(int32_t type, int32_t data);
void retdec_squirrel_release(int32_t type, int32_t data);
void retdec_squirrel_assign(int32_t *dst, const int32_t *src);
void retdec_release_squirrel_value(int32_t *value_ptr);
int32_t retdec_gc_object_type(int32_t object_ptr);
void retdec_gc_mark_value(const int32_t *value, int32_t *chain_head);
void retdec_gc_finalize_collectable(int32_t object_ptr,
                                            int32_t object_type);
void __fastcall function_48be70(int32_t object, void* unused);
int32_t __fastcall function_48bf50(int32_t object, void* unused, int32_t flags);
#ifdef __cplusplus
}
#endif
