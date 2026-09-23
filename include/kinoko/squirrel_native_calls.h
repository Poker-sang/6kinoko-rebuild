#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Legacy game callbacks; all VM objects and captures are read by source APIs. */
int32_t kinoko_native_property_set_callback(int32_t vm);
int32_t kinoko_native_property_get_callback(int32_t vm);
int32_t kinoko_native_class_weakref_callback(int32_t vm);
int32_t function_41e260(int32_t vm);
int32_t function_41e2c0(int32_t vm);
int32_t function_431650(int32_t vm);
int32_t kinoko_native_integer_member_callback(int32_t vm);
int32_t kinoko_native_nullary_member_callback(int32_t vm);
int32_t kinoko_native_draw_member_callback(int32_t vm);
int32_t function_445730(int32_t vm);
int32_t function_4552e0(int32_t vm);
int32_t function_4555a0(int32_t vm);

/* Explicit receiver + cdecl target (not the x86 thiscall method family). */
int32_t kinoko_native_string_callback_entry(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t function_46b490(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t kinoko_native_three_integer_callback_entry(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t function_46b500(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t kinoko_native_bool_two_integer_callback_entry(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t function_46b610(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t kinoko_native_integer_two_integer_callback_entry(int32_t self, int32_t target, int32_t vm, int32_t first);
int32_t function_46b6f0(int32_t self, int32_t target, int32_t vm, int32_t first);
void kinoko_native_capture_receiver_pair(int32_t vm, int32_t pair[2]);
void function_46c6b0_pair(int32_t vm, int32_t pair[2]);
int32_t kinoko_input_save_entry(int32_t vm);
int32_t kinoko_input_assign_entry(int32_t vm);
int32_t kinoko_input_wait_entry(int32_t vm);
int32_t kinoko_input_get_entry(int32_t vm);

int32_t function_470df0(int32_t vm, int32_t index);
int32_t function_470ee0(int32_t vm);
int32_t function_471160(int32_t callback, int32_t vm, int32_t index);
int32_t function_471330(int32_t callback, int32_t vm, int32_t index);
int32_t function_471880(int32_t callback, int32_t vm, int32_t index);
int32_t function_471960(int32_t callback, int32_t vm, int32_t index);
int32_t kinoko_native_string_only_callback_entry(int32_t callback, int32_t vm, int32_t index);
int32_t function_4716b0(int32_t callback, int32_t vm, int32_t index);
int32_t kinoko_native_two_integer_callback_entry(int32_t callback, int32_t vm, int32_t index);
int32_t function_471a60(int32_t callback, int32_t vm, int32_t index);
int32_t kinoko_native_string_pair_callback_entry(int32_t callback, int32_t vm, int32_t index);
int32_t function_471720(int32_t callback, int32_t vm, int32_t index);
int32_t kinoko_native_create_event_callback(int32_t vm);
int32_t function_471d30(int32_t vm);
int32_t function_471bc0(int32_t vm);
int32_t function_471c10(int32_t vm);
int32_t function_471d90(int32_t vm);
int32_t function_471eb0(int32_t vm);
int32_t function_471f10(int32_t vm);
int32_t function_471fd0(int32_t vm);
int32_t function_472080(int32_t vm);
int32_t function_4720e0(int32_t vm);
int32_t function_472140(int32_t vm);
int32_t function_471e50(int32_t vm);
int32_t function_471f70(int32_t vm);
int32_t function_472030(int32_t vm);
#ifdef __cplusplus
}
#endif
