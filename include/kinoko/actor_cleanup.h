#pragma once
#include <stdint.h>
#include "kinoko/actor_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t kinoko_clear_animation_list(int32_t list);
KinokoActor **kinoko_actor_manager_clear_resources(KinokoActorManager *manager);

#ifdef __cplusplus
}
#endif
