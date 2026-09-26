#pragma once
#include <stdint.h>
typedef struct KinokoAnimation KinokoAnimation;
typedef struct KinokoActorManager KinokoActorManager;
#ifdef __cplusplus
extern "C" {
#endif
KinokoAnimation *kinoko_animation_allocate(uint32_t frames);
void kinoko_animation_release(KinokoAnimation *animation);
/* On success manager owns the allocation; on exception caller still owns it. */
void kinoko_animation_manager_adopt(KinokoActorManager *manager,KinokoAnimation *animation);
int32_t kinoko_animation_bind(KinokoActorManager *manager,int32_t take,KinokoAnimation *animation);
KinokoAnimation *kinoko_animation_find(KinokoActorManager *manager,int32_t take);
void kinoko_animation_add_texture(KinokoActorManager *manager,int32_t handle);
void kinoko_animation_list_construct(void* list);
void kinoko_animation_list_destroy(void* list);
int32_t kinoko_clear_animation_list(void* list);
#ifdef __cplusplus
}
#endif
