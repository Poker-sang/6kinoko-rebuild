#pragma once
#include <stdint.h>
typedef struct KinokoQuad KinokoQuad;
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_quad_submit(KinokoQuad *quad,float x,float y);
int32_t kinoko_render_set_blend(int32_t mode);
int32_t kinoko_render_set_depth(int32_t test_enabled,int32_t write_enabled);
int32_t kinoko_render_set_alpha(int32_t blend_enabled,int32_t test_enabled);
#ifdef __cplusplus
}
#endif
