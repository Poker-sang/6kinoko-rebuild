#pragma once
#include <stdint.h>
typedef struct KinokoActor KinokoActor;
#ifdef __cplusplus
extern "C" {
#endif
void *kinoko_actor_priority_insert(void *index, KinokoActor *actor);
void kinoko_actor_priority_erase(void *index, void *entry);
void *kinoko_actor_priority_first(void *index);
void *kinoko_actor_priority_next(void *index, void *entry);
KinokoActor *kinoko_actor_priority_value(void *entry);
void kinoko_priority_construct(void *index);
void kinoko_priority_destroy(void *index);
void kinoko_priority_clear(void *index);
void *kinoko_actor_priority_insert_ordered(void *index, KinokoActor *actor, int32_t insert_left);
void *kinoko_actor_priority_erase_next(void *index, void *entry);
#ifdef __cplusplus
}
#endif
