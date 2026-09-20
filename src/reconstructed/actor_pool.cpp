#include "kinoko/actor_pool.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_container_memory.h"
#include "kinoko/legacy_memory.hpp"
#include <windows.h>
#include <list>
#include <vector>
#include <memory>
#include <stdexcept>

extern "C" { extern int32_t g29, g28; }
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::address;
struct Pool {
    std::vector<int32_t> actors;
    std::vector<uint32_t> generations;
    std::list<uint32_t> free_slots;
    uint32_t generation = 0;
};
Pool& pool(int32_t manager) { return *field<Pool*>(manager + 4); }
struct Lock {
    CRITICAL_SECTION* section;
    explicit Lock(int32_t manager) : section(pointer<CRITICAL_SECTION>(manager + 52)) {
        EnterCriticalSection(section);
    }
    ~Lock() { LeaveCriticalSection(section); }
};
void destroy_actor(int32_t actor, unsigned char flags) {
    using Delete = int32_t (__thiscall *)(void*, unsigned char);
    auto method = field<Delete>(field<int32_t>(actor));
    method(pointer<void>(actor), flags);
}
}

extern "C" int32_t kinoko_actor_pool_construct(int32_t manager) {
    if (!manager) return 0;
    auto state = std::make_unique<Pool>();
    InitializeCriticalSection(pointer<CRITICAL_SECTION>(manager + 52));
    field<int32_t>(manager) = address(&g29);
    field<Pool*>(manager + 4) = state.release();
    return manager;
}

// Original 46AB10: publish the packed handle before constructing a new Actor;
// reuse the most recently retired slot, updating its generation before reset.
extern "C" int32_t kinoko_actor_pool_get(int32_t manager, int32_t output) {
    if (!manager || !output) return 0;
    Lock lock(manager);
    auto& state = pool(manager);
    const bool fresh = state.free_slots.empty();
    const auto slot = fresh ? static_cast<uint32_t>(state.actors.size()) : state.free_slots.back();
    if (++state.generation > 0xffffu) state.generation = 1;
    field<uint16_t>(output) = static_cast<uint16_t>(slot);
    field<uint16_t>(output + 2) = static_cast<uint16_t>(state.generation);
    if (fresh) {
        kinoko::legacy::Allocation<void> storage(std::malloc(0x220));
        if (!storage) throw std::bad_alloc();
        const int32_t actor = function_45e300_this(address(storage.get()));
        state.actors.push_back(actor);
        storage.release(); // ownership passes to the pool, including if the next push throws
        state.generations.push_back(state.generation);
    } else {
        state.free_slots.pop_back();
        state.generations.at(slot) = state.generation;
        const auto actor = state.actors.at(slot);
        if (actor) function_45e300_this(actor);
    }
    return state.actors.at(slot);
}

extern "C" int32_t __fastcall kinoko_method_lookup_actor(int32_t manager, void*, uint32_t handle) {
    Lock lock(manager);
    auto& state = pool(manager);
    const uint32_t slot = handle & 0xffffu;
    if (slot >= state.generations.size() || state.generations[slot] != (handle >> 16)) return 0;
    return state.actors.at(slot);
}

extern "C" int32_t function_46a6f0_this(int32_t manager, uint32_t handle) {
    if (!manager) return 0;
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

extern "C" int32_t __fastcall kinoko_method_actor_pool_count(int32_t manager, void*) {
    return static_cast<int32_t>(pool(manager).actors.size());
}
extern "C" int32_t __fastcall kinoko_method_actor_pool_base_delete(int32_t manager, void*, unsigned char flags) {
    field<int32_t>(manager) = address(&g28);
    if (flags & 1) std::free(pointer<void>(manager));
    return manager;
}
extern "C" int32_t __fastcall kinoko_method_actor_pool_delete(int32_t manager, void*, unsigned char flags) {
    auto* state = field<Pool*>(manager + 4);
    // 46A450 visits every allocated slot, including recycled ones, before
    // destroying the lock, free-list, generations, and actor-pointer vector.
    for (size_t index = 0; index < state->actors.size(); ++index)
        if (const auto actor = state->actors[index]) destroy_actor(actor, 1);
    DeleteCriticalSection(pointer<CRITICAL_SECTION>(manager + 52));
    delete state;
    field<Pool*>(manager + 4) = nullptr;
    return kinoko_method_actor_pool_base_delete(manager, nullptr, flags);
}
