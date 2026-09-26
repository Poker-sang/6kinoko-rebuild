#include "kinoko/act_ownership.hpp"
#include "kinoko/act_list.h"
#include "kinoko/legacy_memory.hpp"
#include <list>
#include <new>
namespace {
using namespace kinoko::legacy;
// Native list owns its link records; external readers borrow the same prefix.
struct Link { Link *next, *previous; void* value; };
struct List {
    Link head;
    std::list<Link> values;
    List() : head{&head, &head, nullptr} {}
};
static_assert(offsetof(List,head)==0 && sizeof(Link)==12);
}
extern "C" int32_t kinoko_act_make_list(void* slot) {
    if (!slot) return 0;
    try { store(slot, new List); return 1; } catch (const std::bad_alloc&) { return 0; }
}
extern "C" int32_t kinoko_act_append_list(void* slot, void* value) {
    if (!slot) return 0;
    auto* owner = load<List*>(slot);
    if (!owner) return 0;
    auto& list = *owner;
    try {
        list.values.push_back({&list.head, list.head.previous, value});
        auto* added = &list.values.back();
        list.head.previous->next = added; list.head.previous = added;
        return 1;
    } catch (const std::bad_alloc&) { return 0; }
}
extern "C" void kinoko_act_list_drop_storage(void* head) { delete static_cast<List*>(head); }
extern "C" void kinoko_act_list_dispose_payloads(void* head) {
    if (!head) return;
    auto* sentinel = static_cast<Link*>(head);
    for (auto* node = sentinel->next; node != sentinel; node = node->next)
        kinoko::act::dispose_owned(node->value);
}
