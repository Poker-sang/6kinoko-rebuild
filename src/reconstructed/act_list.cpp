#include "kinoko/act_list.h"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <new>
namespace {
using namespace kinoko::legacy;
// C readers still follow these borrowed tokens. They are value records owned
// by std::list, never an STL node overlay or separately malloc-owned nodes.
struct Link { int32_t next,previous,value; };
struct List {
    Link head;
    std::list<Link> values;
    List() : head{address(&head),address(&head),0} {}
};
static_assert(offsetof(List,head)==0);
}
extern "C" int32_t retdec_act_make_list(int32_t* slot) {
    if(!slot) return 0;
    try { *slot=address(new List);return 1; } catch(const std::bad_alloc&) { return 0; }
}
extern "C" int32_t retdec_act_append_list(int32_t slot,int32_t value) {
    if(!slot || !field<int32_t>(slot)) return 0;
    auto& list=*pointer<List>(field<int32_t>(slot));
    try {
        list.values.push_back({address(&list.head),list.head.previous,value});
        const int32_t added=address(&list.values.back());
        pointer<Link>(list.head.previous)->next=added;list.head.previous=added;
        return 1;
    } catch(const std::bad_alloc&) { return 0; }
}
extern "C" void kinoko_act_list_drop_storage(int32_t head) { delete pointer<List>(head); }
