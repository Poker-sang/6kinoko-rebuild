#pragma once
#include <stdint.h>
typedef struct KinokoActor KinokoActor;
typedef struct KinokoActorPool KinokoActorPool;
#ifdef __cplusplus
extern "C" {
#endif
KinokoActor *kinoko_actor_pool_acquire(KinokoActorPool *pool, uint32_t *handle);
int32_t kinoko_actor_pool_retire(KinokoActorPool *pool, uint32_t handle);
/* The 80-byte host owns an opaque native container state at +4. No STL
   layout is exposed to C. The existing critical section remains at +52. */
int32_t __fastcall kinoko_method_lookup_actor(int32_t manager, void *unused, uint32_t handle);
int32_t kinoko_actor_pool_construct(int32_t manager);
int32_t kinoko_actor_pool_get(int32_t manager, int32_t output);
int32_t function_46a6f0_this(int32_t manager, uint32_t handle);
int32_t __fastcall kinoko_method_actor_pool_count(int32_t manager, void *unused);
int32_t __fastcall kinoko_method_actor_pool_delete(int32_t manager, void *unused, unsigned char flags);
int32_t __fastcall kinoko_method_actor_pool_base_delete(int32_t manager, void *unused, unsigned char flags);
#ifdef __cplusplus
}
#endif
