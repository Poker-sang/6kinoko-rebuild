#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>

typedef struct KinokoCollisionState KinokoCollisionState;
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed inputs. Results borrow chip/placement storage, owned by the map.
   count selects the output cursor; the native buffer retains its high-water end.
   Internal success convention is 1/0, not the original HRESULT interface. */
int32_t kinoko_map_collision_append(KinokoCollisionState *state, int32_t *count,
    const unsigned char *chip, const void *placement, int32_t index);
int32_t kinoko_map_collision_query(KinokoCollisionState *state, KinokoActLayout *layout,
    int32_t *cached, int32_t left, int32_t top, int32_t right, int32_t bottom,
    int32_t *count);
#ifdef __cplusplus
}
#endif
