#include "kinoko/actor_pool.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/actor_records.hpp"
#include <windows.h>
#include <list>
#include <vector>
#include <memory>
#include <stdexcept>


namespace {

using kinoko::legacy::pointer;
using kinoko::legacy::address;
struct Pool {
    std::vector<KinokoActor *> actors;
    std::vector<uint32_t> generations;
    std::list<uint32_t> free_slots;
    uint32_t generation = 0;
};
struct PoolHost {
    const void *methods;
    Pool *state;
    std::array<unsigned char,44> unknown8;
    CRITICAL_SECTION lock;
    uint32_t unknown76;
};
static_assert(sizeof(PoolHost)==80 && offsetof(PoolHost,lock)==52);
using PoolView=kinoko::native::RecordView<PoolHost>;
PoolView host(KinokoActorPool* manager) { return PoolView(manager); }
Pool& pool(KinokoActorPool* manager) { return *host(manager).get(&PoolHost::state); }
struct Lock {
    CRITICAL_SECTION* section;
    explicit Lock(KinokoActorPool* manager) : section(reinterpret_cast<CRITICAL_SECTION *>(host(manager).bytes(&PoolHost::lock))) {
        EnterCriticalSection(section);
    }
    ~Lock() { LeaveCriticalSection(section); }
};
void destroy_actor(KinokoActor *actor, unsigned char flags) {
    using Delete = int32_t (__thiscall *)(void*, unsigned char);
    auto method = kinoko::legacy::load<Delete>(kinoko::actor::ActorView(actor).get(&kinoko::actor::ActorRecord::vtable));
    method(actor, flags);
}
}

extern "C" KinokoActorPool *kinoko_actor_pool_construct(KinokoActorPool *receiver) {
    if (!receiver) return nullptr;
    auto* manager = receiver;
    auto state = std::make_unique<Pool>();
    InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION *>(host(manager).bytes(&PoolHost::lock)));
    host(manager).set(&PoolHost::methods,static_cast<const void *>(kinoko_actor_pool_methods()));
    host(manager).set(&PoolHost::state,state.release());
    return receiver;
}

// Original 46AB10: publish the packed handle before constructing a new Actor;
// reuse the most recently retired slot, updating its generation before reset.
extern "C" KinokoActor *kinoko_actor_pool_acquire(KinokoActorPool *receiver, uint32_t *output) {
    if (!receiver || !output) return nullptr;
    auto* manager = receiver;
    Lock lock(manager);
    auto& state = pool(manager);
    const bool fresh = state.free_slots.empty();
    const auto slot = fresh ? static_cast<uint32_t>(state.actors.size()) : state.free_slots.back();
    if (++state.generation > 0xffffu) state.generation = 1;
    *output=(slot&0xffffu)|(state.generation<<16);
    if (fresh) {
        kinoko::legacy::Allocation<void> storage(std::malloc(sizeof(kinoko::actor::ActorRecord)));
        if (!storage) throw std::bad_alloc();
        auto *actor = kinoko_actor_construct(static_cast<KinokoActor *>(storage.get()));
        state.actors.push_back(actor);
        storage.release(); // ownership passes to the pool, including if the next push throws
        state.generations.push_back(state.generation);
    } else {
        state.free_slots.pop_back();
        state.generations.at(slot) = state.generation;
        const auto actor = state.actors.at(slot);
        if (actor) kinoko_actor_construct(actor);
    }
    return state.actors.at(slot);
}


// Virtual receivers and results retain their native pointer types.
extern "C" KinokoActor* __fastcall kinoko_method_lookup_actor(KinokoActorPool* manager, void*, uint32_t handle) {
    auto* receiver = manager;
    Lock lock(receiver);
    auto& state = pool(receiver);
    const uint32_t slot = handle & 0xffffu;
    if (slot >= state.generations.size() || state.generations[slot] != (handle >> 16)) return 0;
    return state.actors.at(slot);
}

extern "C" int32_t kinoko_actor_pool_retire(KinokoActorPool *receiver, uint32_t handle) {
    if (!receiver) return 0;
    auto* manager = receiver;
    Lock lock(manager);
    auto& state = pool(manager);
    const uint32_t slot = handle & 0xffffu;
    // The original assumes an in-range handle here. Reject invalid external
    // indices instead of reading beyond native vector storage.
    if (slot >= state.generations.size() || state.generations[slot] != (handle >> 16)) return 0;
    state.generations[slot] = 0;
    destroy_actor(state.actors.at(slot), 0);
    if (state.free_slots.size() == 0x3ffffffeu) throw std::length_error("list<T> too long");
    state.free_slots.push_back(slot);
    return 1; // native callers ignore the original unspecified unlock result
}


extern "C" int32_t __fastcall kinoko_method_actor_pool_count(KinokoActorPool* manager, void*) {
    return static_cast<int32_t>(pool(manager).actors.size());
}
extern "C" KinokoActorPool* __fastcall kinoko_method_actor_pool_base_delete(KinokoActorPool* manager, void*, unsigned char flags) {
    auto* receiver = manager;
    host(receiver).set(&PoolHost::methods,static_cast<const void *>(kinoko_actor_pool_base_methods()));
    if (flags & 1) std::free(receiver);
    return manager;
}
extern "C" KinokoActorPool* __fastcall kinoko_method_actor_pool_delete(KinokoActorPool* manager, void*, unsigned char flags) {
    auto* receiver = manager;
    auto* state = host(receiver).get(&PoolHost::state);
    // 46A450 visits every allocated slot, including recycled ones, before
    // destroying the lock, free-list, generations, and actor-pointer vector.
    for (size_t index = 0; index < state->actors.size(); ++index)
        if (const auto actor = state->actors[index]) destroy_actor(actor, 1);
    DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION *>(host(receiver).bytes(&PoolHost::lock)));
    delete state;
    host(receiver).set(&PoolHost::state,static_cast<Pool *>(nullptr));
    return kinoko_method_actor_pool_base_delete(manager, nullptr, flags);
}

extern "C" void kinoko_trace_i32(const char *,int32_t);
extern "C" KinokoActor *kinoko_actor_pool_request(KinokoActorPool *pool,uint32_t *handle) {
    static uint32_t count;
    if (count<3 || (count&63u)==0) kinoko_trace_i32("actor:request",static_cast<int32_t>(count));
    auto *actor=kinoko_actor_pool_acquire(pool,handle);
    if (actor) {
        ++count;
        if (count<=3 || (count&63u)==0) kinoko_trace_i32("actor:created",static_cast<int32_t>(count));
    }
    return actor;
}
