#include "kinoko/actor_owner_list.h"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <stdexcept>

extern "C" { extern int32_t g31; }
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::address;
using Owners = std::list<int32_t>;
Owners& owners(int32_t manager) { return *field<Owners*>(manager + 8); }
template<class Method> Method slot(int32_t object, unsigned index) {
    return field<Method>(field<int32_t>(object) + index * 4);
}
using Get = int32_t (__thiscall *)(void*, int32_t);
using Release = int32_t (__thiscall *)(void*, uint32_t);
using Delete = int32_t (__thiscall *)(void*, unsigned char);
}

extern "C" void kinoko_actor_owner_list_construct(int32_t manager) {
    field<Owners*>(manager + 8) = new Owners;
}
extern "C" uint32_t kinoko_actor_owner_list_size(int32_t manager) {
    return static_cast<uint32_t>(owners(manager).size());
}
// 46AA60: get through the pool's virtual entry, install the handle/refcount,
// and append to the owning list in insertion order.
extern "C" int32_t function_46aa60_this(int32_t manager) {
    if (!manager) return 0;
    const auto pool = field<int32_t>(manager + 4);
    uint32_t handle = 0;
    const auto actor = slot<Get>(pool, 1)(pointer<void>(pool), address(&handle));
    if (!actor) return 0;
    field<uint32_t>(actor + 12) = handle;
    field<int32_t>(actor + 8) = 1;
    auto& list = owners(manager);
    if (list.size() == 0x3ffffffeu) throw std::length_error("list<T> too long");
    list.push_back(actor);
    return actor;
}
// 463580: release payload references first, then destroy the list nodes.
extern "C" void kinoko_actor_owner_list_clear(int32_t manager) {
    auto& list = owners(manager);
    for (auto actor : list) {
        if (--field<int32_t>(actor + 8) == 0) {
            const auto pool = field<int32_t>(manager + 4);
            slot<Release>(pool, 2)(pointer<void>(pool), field<uint32_t>(actor + 12));
        }
    }
    list.clear();
}
// 46A9C0/46AAE0: base ownership destruction, not derived animation cleanup.
extern "C" int32_t __fastcall kinoko_method_actor_owner_delete(int32_t manager, void*, unsigned char flags) {
    field<int32_t>(manager) = address(&g31);
    kinoko_actor_owner_list_clear(manager);
    const auto pool = field<int32_t>(manager + 4);
    if (pool) slot<Delete>(pool, 0)(pointer<void>(pool), 1);
    delete field<Owners*>(manager + 8);
    field<Owners*>(manager + 8) = nullptr;
    field<int32_t>(manager + 4) = 0;
    if (flags & 1) std::free(pointer<void>(manager));
    return manager;
}
