#pragma once
#include <stdint.h>
#include "kinoko/input_manager.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed identities in the original SqPlus host, filled by the C bridge. */
typedef struct KinokoInputScriptSymbols {
    const void *object_vtable;
    int32_t null_type, null_data;
    const void *input_class;
    void *root;
    void *vm;
    uint32_t manager_storage_bytes;
} KinokoInputScriptSymbols;
const KinokoInputScriptSymbols *kinoko_input_script_symbols(void);
typedef struct KinokoCameraMapScriptSymbols {
    void *camera_class;
    void *map_class;
} KinokoCameraMapScriptSymbols;
const KinokoCameraMapScriptSymbols *kinoko_camera_map_script_symbols(void);
int32_t kinoko_input_initialize_script_instance(KinokoInputManager *manager);
int32_t *kinoko_input_binding_type(void);
int32_t *kinoko_camera_binding_type(void);
int32_t *kinoko_map_binding_type(void);
int32_t kinoko_register_input_class(void);
int32_t kinoko_register_camera_binding(void);
int32_t kinoko_register_map_binding(void);
int32_t kinoko_register_root_bindings_entry(void);
void kinoko_register_global_methods(void* root_table);
#ifdef __cplusplus
}
#endif
