#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { KINOKO_TEXTURE_CAPACITY = 4096 };
typedef struct KinokoTextureSlot {
    void *texture;
    uint32_t width;
    uint32_t height;
} KinokoTextureSlot;

/* The legacy renderer borrows these entries. Resource owners acquire/release. */
extern KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY];
int32_t kinoko_texture_acquire(const char *path);
/* Adopts one COM reference on success; on failure the caller still owns it.
   Slot pointers are borrowed. Only release(handle) consumes store ownership. */
int32_t kinoko_texture_register(void *texture, uint32_t width, uint32_t height);
int32_t kinoko_texture_release(int32_t handle);
int32_t kinoko_texture_retain(int32_t handle);

#ifdef __cplusplus
}
#endif
