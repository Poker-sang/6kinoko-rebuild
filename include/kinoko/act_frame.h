#pragma once
#include <stdint.h>
#include "kinoko/act_types.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoBlitSprite KinokoBlitSprite;
typedef struct KinokoActCommandOwner KinokoActCommandOwner;
typedef struct KinokoActSpriteOwner KinokoActSpriteOwner;
typedef struct KinokoActCommandStorage { KinokoActCommandOwner* owner; uint32_t reserved[2]; } KinokoActCommandStorage;
typedef struct KinokoActSpriteStorage { KinokoActSpriteOwner* owner; uint32_t reserved[2]; } KinokoActSpriteStorage;
/* Borrowed contiguous byte views. Mutating owners may invalidate them. */
typedef struct KinokoDrawSpan { unsigned char *begin,*end,*capacity; } KinokoDrawSpan;
KinokoDrawSpan kinoko_act_command_span(KinokoActRuntime* resource);
KinokoDrawSpan kinoko_act_sprite_span(KinokoActRuntime* resource);
void kinoko_act_commands_clear(KinokoActRuntime* resource);
void kinoko_act_draw_storage_destroy(KinokoActRuntime* resource);
/* Original 451640, 4522F0, 4525D0; update, preparation and drawing are
   separate passes. Receivers are borrowed runtime pointers. */
int32_t kinoko_act_update_frame(KinokoActRuntime* resource);
int32_t kinoko_act_layer_update(KinokoActLayer *layer);
int32_t kinoko_act_prepare_draw(KinokoActRuntime* resource);
int32_t kinoko_act_draw(KinokoActRuntime* resource, float x, float y);
int32_t kinoko_act_resize_sprites(KinokoActSpriteStorage* storage, uint32_t requested);
KinokoBlitSprite* kinoko_act_copy_blit_sprites(const KinokoBlitSprite* first, const KinokoBlitSprite* last, KinokoBlitSprite* output);
unsigned char* kinoko_act_clear_sprites(KinokoActSpriteStorage* storage);
int32_t kinoko_act_append_blit(KinokoActRuntime* resource, int32_t x, int32_t y,
    int32_t width, int32_t height, KinokoActResource* texture_resource, int32_t sx, int32_t sy,
    int32_t blend, float alpha);
#ifdef __cplusplus
}
#endif
