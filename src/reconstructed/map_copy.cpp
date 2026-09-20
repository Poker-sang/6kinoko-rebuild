#include "kinoko/squirrel_host_compat.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

extern "C" {
extern unsigned char g37;
int32_t function_450020(int32_t player);
}

namespace {
// ABI views only: modern STL objects must never overlay VC8 container storage.
struct RenderLayer { void* vtable; int32_t layout; };
struct Node { Node *next, *previous; RenderLayer value; };
struct List { Node* sentinel; uint32_t count; };
struct Vector { int32_t *begin, *end, *capacity; };
struct Manager {
    unsigned char script_object[12];
    int32_t source_act, source_holder, player;
    List layers;
    int32_t list_proxy;
    Vector vector;
    int32_t vector_proxy;
    int32_t fields[8];
};
static_assert(sizeof(Node)==16 && offsetof(Node,value)==8);
static_assert(offsetof(Manager,layers)==24 && offsetof(Manager,vector)==36);
static_assert(offsetof(Manager,fields)==52 && sizeof(Manager)==84);

void assign_layers(List& out, const List& in) {
    // 4700B0 clears nodes, not the borrowed layouts. 46F450 constructs an
    // eight-byte RenderLayer, restoring its own vtable and copying layout+4.
    Node* node=out.sentinel->next;
    out.sentinel->next=out.sentinel->previous=out.sentinel;
    out.count=0;
    while(node!=out.sentinel) {
        Node* next=node->next;
        std::free(node);
        node=next;
    }
    for(node=in.sentinel->next;node!=in.sentinel;node=node->next) {
        auto* copy=static_cast<Node*>(std::malloc(sizeof(Node)));
        if(!copy) throw std::bad_alloc();
        *copy={out.sentinel,out.sentinel->previous,{&g37,node->value.layout}};
        out.sentinel->previous->next=copy;
        out.sentinel->previous=copy;
        ++out.count;
    }
}

void assign_vector(Vector& out,const Vector& in) {
    if(&out==&in) return;
    const size_t count=in.begin?static_cast<size_t>(in.end-in.begin):0;
    const size_t capacity=out.begin?static_cast<size_t>(out.capacity-out.begin):0;
    // 46F320 retains capacity when shrinking, including assignment of empty.
    if(!count) { out.end=out.begin; return; }
    if(count>capacity) {
        auto* replacement=static_cast<int32_t*>(std::malloc(count*sizeof(int32_t)));
        if(!replacement) throw std::bad_alloc();
        std::free(out.begin);
        out.begin=replacement;
        out.capacity=replacement+count;
    }
    std::copy_n(in.begin,count,out.begin);
    out.end=out.begin+count;
}
}

// SqPlus's explicit destination/source callback (4701B0 -> 470100).
extern "C" int32_t function_4701b0(int32_t destination,int32_t source) {
    auto& out=*reinterpret_cast<Manager*>(destination);
    auto& in=*reinterpret_cast<Manager*>(source);
    function_4a95c0_this(destination,source);
    out.source_act=in.source_act;
    out.source_holder=in.source_holder;
    // Original auto_ptr-style transfer is intentional, including self-copy.
    const int32_t player=in.player;
    in.player=0;
    if(player!=out.player && out.player) {
        function_450020(out.player);
        std::free(reinterpret_cast<void*>(out.player));
    }
    out.player=player;
    if(destination!=source) assign_layers(out.layers,in.layers);
    assign_vector(out.vector,in.vector);
    if(destination!=source) std::copy_n(in.fields,8,out.fields);
    return destination;
}
