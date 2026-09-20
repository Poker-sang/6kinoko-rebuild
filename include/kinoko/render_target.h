#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_initialize_renderer_sets(void);
void kinoko_initialize_texture_cache(void);
int32_t __fastcall kinoko_method_create_render_target(int32_t resource, void *unused,
                                                      int32_t width, int32_t height);
#ifdef __cplusplus
}
#endif
