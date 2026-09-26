#pragma once
#include <stdint.h>
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif

/* Legacy game callbacks; all VM objects and captures are read by source APIs. */
int32_t kinoko_native_property_set_callback(struct SQVM* vm);
int32_t kinoko_native_property_get_callback(struct SQVM* vm);
int32_t kinoko_native_class_weakref_callback(struct SQVM* vm);
int32_t kinoko_native_integer_member_callback(struct SQVM* vm);
int32_t kinoko_native_nullary_member_callback(struct SQVM* vm);
int32_t kinoko_native_draw_member_callback(struct SQVM* vm);

/* Explicit receiver + cdecl target (not the x86 thiscall method family). */
int32_t kinoko_native_string_callback_entry(void* self, void* target, struct SQVM* vm, int32_t first);
int32_t kinoko_native_three_integer_callback_entry(void* self, void* target, struct SQVM* vm, int32_t first);
int32_t kinoko_native_bool_two_integer_callback_entry(void* self, void* target, struct SQVM* vm, int32_t first);
int32_t kinoko_native_integer_two_integer_callback_entry(void* self, void* target, struct SQVM* vm, int32_t first);
void kinoko_native_capture_receiver_pair(struct SQVM* vm, void* pair[2]);
int32_t kinoko_input_save_entry(struct SQVM* vm);
int32_t kinoko_input_assign_entry(struct SQVM* vm);
int32_t kinoko_input_wait_entry(struct SQVM* vm);
int32_t kinoko_input_get_entry(struct SQVM* vm);

int32_t kinoko_native_truthy_entry(struct SQVM* vm, int32_t index);
int32_t kinoko_native_no_arguments_entry(struct SQVM* vm);
int32_t kinoko_native_string_object_entry(void* callback, struct SQVM* vm, int32_t index);
int32_t kinoko_native_string_bool_entry(void* callback, struct SQVM* vm, int32_t index);
int32_t kinoko_native_string_two_integer_truth_entry(void* callback, struct SQVM* vm, int32_t index);
int32_t kinoko_native_string_three_integer_truth_entry(void* callback, struct SQVM* vm, int32_t index);
int32_t kinoko_native_string_only_callback_entry(void* callback, struct SQVM* vm, int32_t index);
int32_t kinoko_native_two_integer_callback_entry(void* callback, struct SQVM* vm, int32_t index);
int32_t kinoko_native_string_pair_callback_entry(void* callback, struct SQVM* vm, int32_t index);
int32_t kinoko_native_create_event_callback(struct SQVM* vm);
int32_t kinoko_native_integer_pair_entry(struct SQVM* vm);
int32_t kinoko_native_void_entry(struct SQVM* vm);
int32_t kinoko_native_string_object_result_entry(struct SQVM* vm);
int32_t kinoko_native_string_bool_result_entry(struct SQVM* vm);
int32_t kinoko_native_two_floats_entry(struct SQVM* vm);
int32_t kinoko_native_string_entry(struct SQVM* vm);
int32_t kinoko_native_integer_entry(struct SQVM* vm);
int32_t kinoko_native_string_two_integer_truth_callback(struct SQVM* vm);
int32_t kinoko_native_string_three_integer_truth_callback(struct SQVM* vm);
int32_t kinoko_native_two_integer_entry(struct SQVM* vm);
int32_t kinoko_native_string_object_callback(struct SQVM* vm);
int32_t kinoko_native_integer_result_entry(struct SQVM* vm);
#ifdef __cplusplus
}
#endif
