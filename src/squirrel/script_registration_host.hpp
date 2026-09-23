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
int32_t function_470f60(int32_t a1);
int32_t function_470f80(int32_t dwMilliseconds);
int32_t function_470f90(void);
int32_t function_471080(void);
int32_t function_471bc0(int32_t a1);
int32_t function_471c10(int32_t a1);
int32_t function_471d90(int32_t a1);
int32_t function_471eb0(int32_t a1);
int32_t function_471f10(int32_t a1);
int32_t function_471fd0(int32_t a1);
int32_t function_472080(int32_t a1);
int32_t function_4720e0(int32_t a1);
int32_t function_472140(int32_t a1);
int32_t kinoko_script_bind_root_value(int32_t * a1, int32_t * a2, char * a3, int32_t a4);
int32_t kinoko_script_bind_root_integer(int32_t * a1, int32_t a2, char * a3);
int32_t function_472c90(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);
int32_t function_472e50(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);
int32_t retdec_compile_file_native(int32_t vm);
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
extern int32_t g629[3];
}
