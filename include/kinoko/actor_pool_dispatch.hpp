#pragma once
#include "kinoko/actor_pool.h"
#include <cstring>

namespace kinoko::actor {
// Preserve the original virtual interface, including ABI fixture pools.
struct PoolMethods {
    int32_t (__thiscall *destroy)(KinokoActorPool *,unsigned char);
    KinokoActor *(__thiscall *acquire)(KinokoActorPool *,uint32_t *);
    int32_t (__thiscall *retire)(KinokoActorPool *,uint32_t);
};
inline PoolMethods pool_methods(KinokoActorPool *pool) {
    const PoolMethods *table;
    std::memcpy(&table,pool,sizeof(table));
    return *table;
}
}
