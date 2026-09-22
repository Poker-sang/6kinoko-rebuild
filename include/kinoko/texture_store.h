#pragma once

#include <stdint.h>
#include <windows.h>
#include <d3d9.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { KINOKO_TEXTURE_CAPACITY = 4096, KINOKO_TEXTURE_STAGE_COUNT = 8 };
typedef struct KinokoTextureSlot {
    IDirect3DBaseTexture9 *texture;
    uint32_t width;
    uint32_t height;
} KinokoTextureSlot;

/* The legacy renderer borrows these entries. Resource owners acquire/release. */
extern KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY];
int32_t kinoko_texture_acquire(const char *path);
/* Adopts one COM reference on success; on failure the caller still owns it.
   Slot pointers are borrowed. Only release(handle) consumes store ownership. */
int32_t kinoko_texture_register(IDirect3DBaseTexture9 *texture, uint32_t width, uint32_t height);
int32_t kinoko_texture_release(int32_t handle);
int32_t kinoko_texture_retain(int32_t handle);
/* 405E30: stage was carried in EDI. Cached nonzero returns the handle; zero
   always unbinds and returns zero. A changed handle returns SetTexture status
   and is cached even if that call fails, exactly as in the original. */
int32_t kinoko_texture_bind_stage(int32_t stage, int32_t handle);
void kinoko_initialize_texture_cache(void);
/* Final release invalidates borrowed cache entries; this does not own a COM
   reference and does not replace the store's actual-device unbind operation. */
void kinoko_texture_forget_bindings(int32_t handle);

#ifdef __cplusplus
}
#endif
