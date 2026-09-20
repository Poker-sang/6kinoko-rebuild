#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Internal CStringLayout atlas path. Input is one CharNextA character (or
   empty for atlas initialization), not the generic rich-text renderer. */
void kinoko_string_font_construct(int32_t renderer);
void kinoko_string_font_configure(int32_t renderer, int32_t layout);
void kinoko_string_font_rasterize(int32_t renderer, const char *character,
                                 int32_t *width, int32_t *height);
int32_t kinoko_string_font_texture(int32_t renderer);
void kinoko_string_font_upload(int32_t renderer, int32_t handle,
                              const char *character, int32_t x, int32_t y,
                              int32_t *width, int32_t *height);
#ifdef __cplusplus
}
#endif
