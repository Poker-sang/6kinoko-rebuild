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
void kinoko_priority_construct(int32_t tree);
void kinoko_priority_destroy(int32_t tree);
void kinoko_priority_clear(int32_t tree);
int32_t kinoko_priority_first(int32_t tree);
int32_t kinoko_priority_next(int32_t tree,int32_t token);
int32_t kinoko_priority_value(int32_t token);
int32_t function_463210_this(int32_t tree,int32_t source);
int32_t function_463610_this(int32_t tree,int32_t output,int32_t token,int32_t insert_left);
int32_t function_463280_this(int32_t tree,int32_t output,int32_t token);
#ifdef __cplusplus
}
#endif
