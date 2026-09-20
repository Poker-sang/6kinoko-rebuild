#include "kinoko/legacy_container_memory.h"
#include "kinoko/legacy_memory.hpp"
#include <windows.h>
#include <cstdlib>
#include <new>
#include <stdexcept>
namespace {
using kinoko::legacy::field;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
// Explicit boundary record still consumed by C stage-owner iteration. It is not a
// modern STL container overlay. Allocation pairs with those consumers' free.
struct Node { int32_t next,previous,value; };
static_assert(sizeof(Node)==12);

}
extern "C" int32_t function_4214a0(int32_t next,int32_t previous,int32_t *value) {
    auto *node=static_cast<Node*>(std::malloc(sizeof(Node)));
    if(!node) throw std::bad_alloc();
    *node={next,previous,*value};return address(node);
}
