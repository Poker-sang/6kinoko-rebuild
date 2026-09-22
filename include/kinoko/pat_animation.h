#pragma once
#include "kinoko/actor_render.h"
typedef struct KinokoAnimation KinokoAnimation;
typedef struct KinokoArchiveReader KinokoArchiveReader;
/* Decoded fields, not a packed overlay of bytes in the PAT file. */
typedef struct KinokoPatFrameFields {
    uint32_t resource_index;
    int16_t sprite_x, sprite_y, source_width, source_height;
    int16_t offset_x, offset_y, duration;
    uint8_t type;
    int16_t auxiliary_mode;
    uint16_t auxiliary_values[5];
    uint8_t auxiliary_bytes[4];
} KinokoPatFrameFields;
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_pat_build_frame(KinokoActorManager *manager,KinokoAnimationFrame *frame,
    const KinokoPatFrameFields *fields,uint32_t resource_base);
int32_t kinoko_pat_read_animations(KinokoArchiveReader *reader,KinokoActorManager *manager,uint32_t resource_base);
int32_t kinoko_pat_load(KinokoActorManager *manager,const char *file_name,const char *directory);
const void *kinoko_pat_frame_methods(void);
#ifdef __cplusplus
}
#endif
