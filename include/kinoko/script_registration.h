#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t *kinoko_input_binding_type(void);
int32_t *kinoko_camera_binding_type(void);
int32_t *kinoko_map_binding_type(void);
int32_t kinoko_register_input_class(void);
int32_t function_473010(void);
void kinoko_register_global_methods(int32_t root_table);
#ifdef __cplusplus
}
#endif
