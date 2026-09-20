#include "kinoko/legacy_container_memory.h"
#include "kinoko/legacy_memory.hpp"
#include <windows.h>
#include <cstdlib>
#include <new>
#include <stdexcept>
extern "C" int32_t function_46ab10_this(int32_t manager,int32_t output);
namespace {
using kinoko::legacy::field;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
// Explicit boundary record still consumed by C actor iteration. It is not a
// modern STL container overlay. Allocation pairs with those consumers' free.
struct Node { int32_t next,previous,value; };
static_assert(sizeof(Node)==12);
struct Lock {
    CRITICAL_SECTION *value;
    explicit Lock(int32_t manager):value(pointer<CRITICAL_SECTION>(manager+52)) { EnterCriticalSection(value); }
    ~Lock() { LeaveCriticalSection(value); }
};
}
extern "C" int32_t function_4214a0(int32_t next,int32_t previous,int32_t *value) {
    auto *node=static_cast<Node*>(std::malloc(sizeof(Node)));
    if(!node) throw std::bad_alloc();
    *node={next,previous,*value};return address(node);
}
extern "C" int32_t __fastcall kinoko_method_lookup_actor(int32_t manager,void*,uint32_t handle) {
    Lock lock(manager);
    const uint32_t slot=handle&0xffffu, generation=handle>>16;
    const int32_t generations=field<int32_t>(manager+20);
    const uint32_t count=(static_cast<uint32_t>(field<int32_t>(manager+24))-static_cast<uint32_t>(generations))/4u;
    if(slot>=count || field<uint32_t>(generations+4*slot)!=generation) return 0;
    const int32_t actors=field<int32_t>(manager+4);
    const uint32_t actor_count=(static_cast<uint32_t>(field<int32_t>(manager+8))-static_cast<uint32_t>(actors))/4u;
    if(slot>=actor_count) throw std::out_of_range("invalid vector<T> subscript");
    return field<int32_t>(actors+4*slot);
}
extern "C" int32_t function_46aa60_this(int32_t manager) {
    if(!manager) return 0;
    int32_t handle[2]={0,0};
    int32_t actor=function_46ab10_this(field<int32_t>(manager+4),address(handle));
    if(!actor) return 0;
    field<int32_t>(actor+12)=handle[0];field<int32_t>(actor+8)=1;
    const int32_t sentinel=field<int32_t>(manager+8);
    if(!sentinel) return actor;
    const int32_t node=function_4214a0(sentinel,field<int32_t>(sentinel+4),&actor);
    if(field<uint32_t>(manager+12)==0x3ffffffeu) {
        std::free(pointer<void>(node));
        throw std::length_error("list<T> too long");
    }
    ++field<uint32_t>(manager+12);
    field<int32_t>(sentinel+4)=node;
    field<int32_t>(field<int32_t>(node+4))=node;
    return actor;
}
