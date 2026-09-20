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

}
extern "C" int32_t function_4214a0(int32_t next,int32_t previous,int32_t *value) {
    auto *node=static_cast<Node*>(std::malloc(sizeof(Node)));
    if(!node) throw std::bad_alloc();
    *node={next,previous,*value};return address(node);
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
