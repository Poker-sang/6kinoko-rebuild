#pragma once
#include <stddef.h>
#include <stdint.h>

/* CBitmapData: palette is borrowed; pixels owns the loaded byte array.
   The public record preserves the Win32 layout, not a C++ container overlay. */
typedef struct KinokoBitmap {
    const void *methods;
    uint8_t bit_depth;
    uint8_t reserved5[3];
    uint32_t width, height, row_width, encoded_size;
    const uint16_t *palette;
    uint8_t *pixels;
} KinokoBitmap;

#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_bitmap_load_cv2(KinokoBitmap *bitmap, const char *path);
void kinoko_bitmap_release_pixels(KinokoBitmap *bitmap);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoBitmap) == 32);
static_assert(offsetof(KinokoBitmap, bit_depth) == 4);
static_assert(offsetof(KinokoBitmap, width) == 8);
static_assert(offsetof(KinokoBitmap, height) == 12);
static_assert(offsetof(KinokoBitmap, row_width) == 16);
static_assert(offsetof(KinokoBitmap, encoded_size) == 20);
static_assert(offsetof(KinokoBitmap, palette) == 24);
static_assert(offsetof(KinokoBitmap, pixels) == 28);
#endif
