#pragma once
#include <windows.h>
#include <d3d9.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* On success transfers one texture COM reference to *texture. Width/height
   describe the source image, even when the device requires square allocation.
   On failure the output pointer is untouched; dimensions may be published. */
HRESULT kinoko_texture_load_image(const char *path, IDirect3DTexture9 **texture,
    uint32_t *width, uint32_t *height);
#ifdef __cplusplus
}
#endif
