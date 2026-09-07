#ifndef KINOKO_ACTOR_COLLISION_H
#define KINOKO_ACTOR_COLLISION_H

#include <stdint.h>

typedef struct KinokoCollisionRecord {
    const unsigned char *chip;
    const void *layout;
    int32_t index;
} KinokoCollisionRecord;

#ifdef __cplusplus
extern "C" {
#endif

/* Original 4689D0 narrow phase, using the native Actor and ChipLayout fields. */
int32_t kinoko_actor_collision_move(void *actor,
    const KinokoCollisionRecord *records, int32_t count, float dx, float dy);

#ifdef __cplusplus
}
#endif
#endif
