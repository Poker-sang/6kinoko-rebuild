#pragma once
#include <stdint.h>
typedef struct KinokoActor KinokoActor;
typedef struct KinokoActorPool KinokoActorPool;
#ifdef __cplusplus
extern "C" {
#endif
const void *kinoko_actor_pool_methods(void);
const void *kinoko_actor_pool_base_methods(void);
KinokoActor *kinoko_actor_pool_acquire(KinokoActorPool *pool, uint32_t *handle);
int32_t kinoko_actor_pool_retire(KinokoActorPool *pool, uint32_t handle);
/* The 80-byte host owns an opaque native container state at +4. No STL
   layout is exposed to C. The existing critical section remains at +52. */
KinokoActor* __fastcall kinoko_method_lookup_actor(KinokoActorPool* manager, void *unused, uint32_t handle);
KinokoActorPool *kinoko_actor_pool_construct(KinokoActorPool *pool);

KinokoActor *kinoko_actor_pool_request(KinokoActorPool *pool, uint32_t *handle);
int32_t __fastcall kinoko_method_actor_pool_count(KinokoActorPool* manager, void *unused);
KinokoActorPool* __fastcall kinoko_method_actor_pool_delete(KinokoActorPool* manager, void *unused, unsigned char flags);
KinokoActorPool* __fastcall kinoko_method_actor_pool_base_delete(KinokoActorPool* manager, void *unused, unsigned char flags);
#ifdef __cplusplus
}
#endif
