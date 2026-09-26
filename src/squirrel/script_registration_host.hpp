#include "kinoko/input_manager.h"
#include "kinoko/script_file.h"
#include "kinoko/game_script_api.h"
#include "kinoko/game_runtime.h"
#pragma once
#include "kinoko/squirrel_native_calls.h"
#include "kinoko/csv_bridge.h"
#include "kinoko/audio_runtime.h"
extern "C" {
int32_t kinoko_script_dprint_noop(void);
int32_t kinoko_actor_register_script_class(void);
int32_t kinoko_register_input_class(void);
int32_t kinoko_host_show_message_abi(int32_t a1);
int32_t kinoko_host_sleep_abi(int32_t dwMilliseconds);
int32_t kinoko_host_milliseconds(void);
int32_t kinoko_host_close_window(void);
int32_t kinoko_native_void_entry(struct SQVM* a1);
int32_t kinoko_native_string_object_result_entry(struct SQVM* a1);
int32_t kinoko_native_string_bool_result_entry(struct SQVM* a1);
int32_t kinoko_native_two_floats_entry(struct SQVM* a1);
int32_t kinoko_native_string_entry(struct SQVM* a1);
int32_t kinoko_native_integer_entry(struct SQVM* a1);
int32_t kinoko_native_string_two_integer_truth_callback(struct SQVM* a1);
int32_t kinoko_native_string_three_integer_truth_callback(struct SQVM* a1);
int32_t kinoko_native_two_integer_entry(struct SQVM* a1);
int32_t kinoko_script_bind_root_value(int32_t * a1, int32_t * a2, char * a3, int32_t a4);
int32_t kinoko_script_bind_root_integer(int32_t * a1, int32_t a2, char * a3);
int32_t function_472c90(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);
int32_t function_472e50(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);
int32_t kinoko_compile_file_native(int32_t vm);
void kinoko_trace(const char*);
void kinoko_trace_i32(const char*, int32_t);
extern int32_t kinoko_input_class_storage[3], kinoko_act_vm_abi_slot;
}
