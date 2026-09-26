#pragma once
#include <stdint.h>
struct SQVM;
#ifdef __cplusplus
extern "C" {
#endif

/* Fixed-layout embedding records stay at this C boundary, never in the VM. */
void* kinoko_push_script_object(struct SQVM* vm, void* object);
typedef struct KinokoScriptMemoryReader {
    const unsigned char* base;
    int32_t size;
    const unsigned char* cursor;
} KinokoScriptMemoryReader;
int32_t kinoko_script_read_memory(void* stream, void* destination, int32_t requested);
int32_t kinoko_create_bound_instance(struct SQVM* vm, const int32_t* parent_pair, const char* name, const int32_t* class_pair, void* native, int32_t output[2]);
int32_t kinoko_create_unbound_instance(struct SQVM* vm, const int32_t* class_pair, void* native, int32_t output[2]);
void kinoko_release_act_callback(void* record);
void kinoko_copy_act_callback(struct SQVM* vm, void* script, int32_t offset, void* global_object, const char* name);
int32_t kinoko_bind_act_resource_root(void* resource, struct SQVM* vm, const int32_t* root_pair);
int32_t kinoko_execute_act_file_bytecode(struct SQVM* vm, void* script, const int32_t* environment);
int32_t kinoko_execute_embedded_act_script(struct SQVM* vm, void* script, const int32_t* environment_pair);

int32_t kinoko_squirrel_object_from_pair(int32_t* object, int32_t type, int32_t data);
int32_t kinoko_squirrel_object_string(int32_t* object, const char** value);
int32_t kinoko_squirrel_object_copy(int32_t* destination, const int32_t* source);
int32_t kinoko_squirrel_object_from_string(int32_t* object, const char* data, uint32_t length);
#ifdef __cplusplus
}
#endif
