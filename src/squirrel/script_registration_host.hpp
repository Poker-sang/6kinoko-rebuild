#include "kinoko/game_runtime.h"
#pragma once
#include "kinoko/squirrel_native_calls.h"
#include "kinoko/audio_runtime.h"
extern "C" {
int32_t function_402aa0(void);
int32_t function_402af0(void);
int32_t function_402d30(void);
int32_t function_402d40(char * a1, int32_t a2);
int32_t function_403000(int32_t path, int32_t vtable, int32_t type, int32_t data);
int32_t function_43e100(void);
int32_t function_460e00(void);
int32_t function_4669d0(void);
int32_t function_4696b0(int32_t a1);
int32_t function_469700(void);
int32_t function_469710(float a1, float a2);
int32_t function_469820(int32_t a1);
int32_t function_469840(int32_t a1);
int32_t function_469860(void);
int32_t function_469870(void);
int32_t function_469880(int32_t a1);
int32_t function_469a20(int32_t a1, int32_t a2, int32_t a3,
                        int32_t a4, int32_t a5, int32_t a6);
int32_t function_469b40(
    int32_t result,
    int32_t first_vtable, int32_t first_type, int32_t first_data,
    float x, float y, float z,
    int32_t second_vtable, int32_t second_type, int32_t second_data);
int32_t function_469d10(int32_t name, int32_t environment_vtable,
                         int32_t environment_type, int32_t environment_data);
int32_t function_469dd0(int32_t name,
    int32_t closure_vtable, int32_t closure_type, int32_t closure_data,
    int32_t environment_vtable, int32_t environment_type, int32_t environment_data);
int32_t function_46a1d0(void);
int32_t function_46b7c0(int32_t this_ptr, int32_t lpFileName);
int32_t function_46b880(int32_t this_ptr, int32_t lpFileName);
int32_t function_46bbe0(int32_t this_ptr, int32_t device,
                        int32_t field, int32_t value);
int32_t function_46bc90(int32_t this_ptr, int32_t device, int32_t field);
int32_t function_46be40(int32_t this_ptr, int32_t device, int32_t field);
int32_t function_46d950(void);
int32_t function_46fac0(void);
int32_t function_470f60(int32_t a1);
int32_t function_470f80(int32_t dwMilliseconds);
int32_t function_470f90(void);
int32_t function_470fa0(int32_t id,
    int32_t closure_vtable, int32_t closure_type, int32_t closure_data,
    int32_t environment_vtable, int32_t environment_type, int32_t environment_data);
int32_t function_471080(void);
int32_t function_471b30(int32_t path, int32_t object_vtable, int32_t vm,
                       int32_t type, int32_t data, char owns_reference);
int32_t function_471bc0(int32_t a1);
int32_t function_471c10(int32_t a1);
int32_t function_471c70(int32_t a1);
int32_t function_471d90(int32_t a1);
int32_t function_471df0(int32_t a1);
int32_t function_471eb0(int32_t a1);
int32_t function_471f10(int32_t a1);
int32_t function_471fd0(int32_t a1);
int32_t function_472080(int32_t a1);
int32_t function_4720e0(int32_t a1);
int32_t function_472140(int32_t a1);
int32_t function_4721a0(int32_t * a1, int32_t * a2, char * a3, int32_t a4);
int32_t function_472240(int32_t * a1, int32_t a2, char * a3);
int32_t function_472c90(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);
int32_t function_472e50(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data);
int32_t retdec_compile_file_native(int32_t vm);
int32_t retdec_create_render_layer_fixed(int32_t name);
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
extern int32_t g629[3], g664;
}
