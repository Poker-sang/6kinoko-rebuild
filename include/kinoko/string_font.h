#pragma once
#include <stdint.h>
#include "kinoko/string_layout.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Internal CStringLayout atlas path. Input is one CharNextA character (or
   empty for atlas initialization), not the generic rich-text renderer. */
void kinoko_string_font_copy_pixels(void* destination, void* source);
void kinoko_string_font_destroy_pixels(void* renderer);
void kinoko_string_font_construct(void* renderer);
void kinoko_string_font_configure(void* renderer, KinokoStringLayout* layout);
void kinoko_string_font_rasterize(void* renderer, const char *character,
                                 int32_t *width, int32_t *height);
int32_t kinoko_string_font_texture(void* renderer);
void kinoko_string_font_upload(void* renderer, int32_t handle,
                              const char *character, int32_t x, int32_t y,
                              int32_t *width, int32_t *height);
#ifdef __cplusplus
}
#endif
