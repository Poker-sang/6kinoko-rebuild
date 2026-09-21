#include "kinoko/string_layout.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include <windows.h>
#include <algorithm>
#include <deque>
#include <cstdlib>
#include <new>
#include <stdexcept>

extern "C" int32_t function_441250(int32_t* object);
extern "C" int32_t g23, g25;
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::StringView;
struct Glyph {
    alignas(4) unsigned char bytes[256];
    Glyph() { field<void*>(address(bytes)+20)=&g25;field<int32_t>(address(bytes)+24)=0; }
    Glyph(const Glyph& other) : Glyph() { *this=other; }
    Glyph& operator=(const Glyph& other) {
        std::copy_n(other.bytes,20,bytes);
        std::copy_n(other.bytes+24,232,bytes+24);return *this;
    }
    ~Glyph() { field<void*>(address(bytes)+20)=&g23; }
};
using Deque=std::deque<Glyph>;
Deque*& storage(int32_t layout) { return field<Deque*>(layout+176); }
Deque& queue(int32_t layout) { return *storage(layout); }
int32_t sprite(const Deque& q,uint32_t i) { return address(q[i].bytes); }
void references(const Deque& q,int delta) {
    for(uint32_t i=0;i<q.size();++i) {
        const int32_t atlas=field<int32_t>(sprite(q,i)+252);
        if(atlas) field<int32_t>(atlas+432)+=delta;
    }
}
void pop(Deque& q,bool front) {
    const int32_t value=sprite(q,front?0:static_cast<uint32_t>(q.size()-1));
    const int32_t atlas=field<int32_t>(value+252);
    if(atlas) --field<int32_t>(atlas+432);
    if(front) q.pop_front();else q.pop_back();
}
void assign(Deque& out,const Deque& in) { if(&out!=&in) out=in; }

}

extern "C" int32_t kinoko_string_push_back(int32_t object,const char* text) {
    if(!text) return 0;
    StringView(pointer<void>(object+32)).append(text,static_cast<uint32_t>(std::strlen(text)));
    return 1;
}
extern "C" int32_t kinoko_string_mark_rebuild(int32_t object) {
    field<uint8_t>(object+228)=1;
    return 1;
}
extern "C" int32_t kinoko_string_clear(int32_t object) {
    StringView(pointer<void>(object+4)).assign("",0);
    StringView(pointer<void>(object+32)).assign("",0);
    field<int32_t>(object+204)=field<int32_t>(object+208)=field<int32_t>(object+212)=0;
    field<int32_t>(object+216)=field<int32_t>(object+88);
    return kinoko_string_mark_rebuild(object);
}
extern "C" int32_t kinoko_string_character_bytes(const char* text) {
    return text && *text?static_cast<int32_t>(CharNextA(text)-text):0;
}
extern "C" int32_t kinoko_string_pop(int32_t object,int32_t count,int32_t front) {
    if(count<0) return 0;
    if(!count) return 1;
    auto& q=queue(object);
    StringView pending(pointer<void>(object+32));
    if(front) while(count && !q.empty()) { pop(q,true);--count; }
    while(count && pending.length()) {
        const uint32_t size=pending.length();
        const char* data=pending.data();
        if(front) {
            // 4406AA subtracts one before the erase end iterator. Preserve the
            // original quirk: an ASCII pending character erases zero bytes.
            const uint32_t bytes=static_cast<uint32_t>(CharNextA(data)-data)-1;
            pending.assign(pending,bytes,UINT32_MAX);
        } else {
            const uint32_t bytes=static_cast<uint32_t>(data+size-CharPrevA(data,data+size));
            pending.assign(pending,0,size-bytes);
        }
        --count;
    }
    if(!front) while(count && !q.empty()) { pop(q,false);--count; }
    return function_441250(pointer<int32_t>(object));
}
extern "C" int32_t kinoko_string_replicate(int32_t object,int32_t source) {
    if(!source) return 0;
    auto& out=queue(object);
    references(out,-1);
    assign(out,queue(source));
    references(out,1);
    return 1;
}

// 442300 appends a default CSpriteEx value into the one-element block deque.
// The original leaves non-vtable/default-texture fields for 404EE0 to fill.
extern "C" int32_t kinoko_string_append_glyph(int32_t layout) {
    auto& q=queue(layout);
    q.emplace_back();
    return address(q.back().bytes);
}

// 43EC30 copies deque values without adjusting atlas references; 43EB80
// immediately destroys that copied queue without decrementing references.
extern "C" void kinoko_string_copy_queue_storage(int32_t object,int32_t source) {
    assign(queue(object),queue(source));
}
extern "C" void kinoko_string_drop_queue_storage(int32_t object) {
    queue(object).clear();
}

extern "C" void kinoko_string_queue_construct(int32_t object) { storage(object)=new Deque; }
extern "C" void kinoko_string_queue_destroy(int32_t object) { delete storage(object);storage(object)=nullptr; }
extern "C" uint32_t kinoko_string_queue_size(int32_t object) { return static_cast<uint32_t>(queue(object).size()); }
extern "C" int32_t kinoko_string_queue_at(int32_t object,uint32_t index) { return address(queue(object).at(index).bytes); }
