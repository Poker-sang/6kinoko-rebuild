#pragma once
#include <stdint.h>
#include "kinoko/actor_manager.h"
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_actor_owner_list_construct(KinokoActorManager *manager);
uint32_t kinoko_actor_owner_list_size(KinokoActorManager *manager);
void kinoko_actor_owner_list_clear(KinokoActorManager *manager);
KinokoActor *kinoko_actor_owner_list_acquire(KinokoActorManager *manager);
KinokoActorManager* __fastcall kinoko_method_actor_owner_delete(KinokoActorManager* manager, void *unused, unsigned char flags);
#ifdef __cplusplus
}
#endif
