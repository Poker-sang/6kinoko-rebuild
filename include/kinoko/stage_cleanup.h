#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_stage_list_construct(void);
void kinoko_stage_list_destroy(void);
/* The opaque end pointer preserves the host's sentinel identity. It must
   only be compared, never dereferenced as a node. */
KinokoStageNode *kinoko_stage_list_end(void);
KinokoStageNode *kinoko_stage_list_first(void);
KinokoStageNode *kinoko_stage_list_next(KinokoStageNode *node);
KinokoStageOwner *kinoko_stage_list_value(const KinokoStageNode *node);
KinokoStageNode *kinoko_stage_list_append(KinokoStageOwner *owner);
int32_t kinoko_clear_global_stages(void);
int32_t kinoko_clear_global_sound(void);
void kinoko_initialize_render_queue(void);
#ifdef __cplusplus
}
#endif
