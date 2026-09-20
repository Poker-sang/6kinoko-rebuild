#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Fixed-layout embedding records stay at this C boundary, never in the VM. */
int32_t function_4029b0(int32_t vm, int32_t* object);
int32_t function_402a50(int32_t stream, int32_t destination, int32_t requested);
int32_t retdec_create_bound_instance(int32_t vm, const int32_t* parent_pair,
    const char* name, const int32_t* class_pair, int32_t native, int32_t output[2]);
int32_t retdec_create_unbound_instance(int32_t vm, const int32_t* class_pair,
    int32_t native, int32_t output[2]);
void retdec_release_act_callback(int32_t record);
void retdec_copy_act_callback(int32_t vm, int32_t script, int32_t offset,
    int32_t global_object, const char* name);
int32_t retdec_bind_act_resource_root(int32_t resource, int32_t vm, const int32_t* root_pair);
int32_t retdec_execute_act_file_bytecode(int32_t vm, int32_t script, const int32_t* environment);
int32_t retdec_execute_embedded_act_script(int32_t vm, int32_t script,
    const int32_t* environment_pair);
int32_t function_45e020_this(int32_t state, int32_t temporary, int32_t type, int32_t data);

int32_t retdec_squirrel_object_from_pair(int32_t* object, int32_t type, int32_t data);
int32_t retdec_squirrel_object_string(int32_t* object, const char** value);
int32_t retdec_squirrel_object_copy(int32_t* destination, const int32_t* source);
int32_t retdec_squirrel_object_from_string(int32_t* object, const char* data, uint32_t length);
#ifdef __cplusplus
}
#endif
