#include "kinoko/actor_owner_list.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_pool_dispatch.hpp"
#include <list>
#include <stdexcept>

extern "C" { extern int32_t g31; }
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::address;
using namespace kinoko::actor;
using Owners = std::list<KinokoActor *>;
ManagerView view(int32_t manager) { return ManagerView(pointer<void>(manager)); }
Owners& owners(int32_t manager) { return *static_cast<Owners *>(view(manager).get(&ManagerPrefix::owner_list)); }
}

extern "C" void kinoko_actor_owner_list_construct(int32_t manager) {
    view(manager).set(&ManagerPrefix::owner_list,static_cast<void *>(new Owners));
}
extern "C" uint32_t kinoko_actor_owner_list_size(int32_t manager) {
    return static_cast<uint32_t>(owners(manager).size());
}
// 46AA60: get through the pool's virtual entry, install the handle/refcount,
// and append to the owning list in insertion order.
extern "C" int32_t function_46aa60_this(int32_t manager) {
    if (!manager) return 0;
    const auto pool = view(manager).get(&ManagerPrefix::pool);
    uint32_t handle = 0;
    auto *actor = pool_methods(pool).acquire(pool,&handle);
    if (!actor) return 0;
    const ActorView state(actor);
    state.set(&ActorRecord::pool_handle,handle);
    state.set(&ActorRecord::owner_references,int32_t{1});
    auto& list = owners(manager);
    if (list.size() == 0x3ffffffeu) throw std::length_error("list<T> too long");
    list.push_back(actor);
    return address(actor);
}
// 463580: release payload references first, then destroy the list nodes.
extern "C" void kinoko_actor_owner_list_clear(int32_t manager) {
    auto& list = owners(manager);
    for (auto actor : list) {
        const ActorView state(actor);
        const auto references=state.get(&ActorRecord::owner_references)-1;
        state.set(&ActorRecord::owner_references,references);
        if (!references) {
            const auto pool = view(manager).get(&ManagerPrefix::pool);
            pool_methods(pool).retire(pool,state.get(&ActorRecord::pool_handle));
        }
    }
    list.clear();
}
// 46A9C0/46AAE0: base ownership destruction, not derived animation cleanup.
extern "C" int32_t __fastcall kinoko_method_actor_owner_delete(int32_t manager, void*, unsigned char flags) {
    view(manager).set(&ManagerPrefix::methods,static_cast<const void *>(&g31));
    kinoko_actor_owner_list_clear(manager);
    const auto pool = view(manager).get(&ManagerPrefix::pool);
    if (pool) pool_methods(pool).destroy(pool,1);
    delete &owners(manager);
    view(manager).set(&ManagerPrefix::owner_list,static_cast<void *>(nullptr));
    view(manager).set(&ManagerPrefix::pool,static_cast<KinokoActorPool *>(nullptr));
    if (flags & 1) std::free(pointer<void>(manager));
    return manager;
}
