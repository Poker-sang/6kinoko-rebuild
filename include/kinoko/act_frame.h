#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoDrawSpan { uint32_t begin,end,capacity; } KinokoDrawSpan;
KinokoDrawSpan kinoko_act_command_span(int32_t resource);
KinokoDrawSpan kinoko_act_sprite_span(int32_t resource);
void kinoko_act_commands_clear(int32_t resource);
void kinoko_act_draw_storage_destroy(int32_t resource);
int32_t function_451640(int32_t resource);
int32_t function_41efb0(int32_t layer);
int32_t function_4522f0(int32_t resource);
int32_t function_4525d0(int32_t resource, float x, float y);
int32_t function_452c20(int32_t vector, uint32_t requested);
int32_t function_455230(int32_t first, int32_t last, int32_t output);
int32_t retdec_act_clear_layout_vector(int32_t vector);
int32_t retdec_act_bitblt_this(int32_t resource, int32_t x, int32_t y,
    int32_t width, int32_t height, int32_t texture_resource, int32_t sx, int32_t sy,
    int32_t blend, float alpha);
#ifdef __cplusplus
}
#endif
