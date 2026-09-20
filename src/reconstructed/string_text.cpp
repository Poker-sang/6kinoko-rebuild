#include "kinoko/string_layout.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include <windows.h>
#include <algorithm>
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
struct Deque { int32_t proxy; int32_t* map; uint32_t capacity, first, size; };
static_assert(sizeof(Deque)==20);
Deque& queue(int32_t layout) { return field<Deque>(layout+176); }
int32_t sprite(const Deque& q,uint32_t i) { return q.map[(q.first+i)%q.capacity]; }
void references(const Deque& q,int delta) {
    for(uint32_t i=0;i<q.size;++i) {
        const int32_t atlas=field<int32_t>(sprite(q,i)+252);
        if(atlas) field<int32_t>(atlas+432)+=delta;
    }
}
void pop(Deque& q,bool front) {
    const int32_t value=sprite(q,front?0:q.size-1);
    const int32_t atlas=field<int32_t>(value+252);
    if(atlas) --field<int32_t>(atlas+432);
    field<void*>(value+20)=&g23;
    if(front) q.first=(q.first+1)%q.capacity;
    if(!--q.size) q.first=0;
}
void grow(Deque& q) {
    // 442BD0 preserves first and rotates all allocated blocks, including spare
    // blocks. Its maximum is for 256-byte elements, not pointer elements.
    uint32_t increment=(std::max)(q.capacity/2,8u);
    if(q.capacity==0xffffffu) throw std::length_error("deque<T> too long");
    if(q.capacity>0xffffffu-increment) increment=1;
    const uint32_t capacity=q.capacity+increment;
    auto* map=static_cast<int32_t*>(std::calloc(capacity,4));
    if(!map) throw std::bad_alloc();
    for(uint32_t i=0;i<q.capacity;++i)
        map[(q.first+i)%capacity]=q.map[(q.first+i)%q.capacity];
    std::free(q.map);q.map=map;q.capacity=capacity;
}
void assign(Deque& out,const Deque& in) {
    if(&out==&in) return;
    if(!in.size) {
        for(uint32_t i=out.capacity;i>0;--i) std::free(pointer<void>(out.map[i-1]));
        std::free(out.map);out.map=nullptr;out.capacity=out.first=out.size=0;
        return;
    }
    const uint32_t common=(std::min)(out.size,in.size);
    for(uint32_t i=0;i<in.size;++i) {
        if(i>=common) {
            if(out.capacity<=out.size+1) grow(out);
            auto& block=out.map[(out.first+out.size)%out.capacity];
            if(!block) {
                block=address(std::malloc(256));
                if(!block) throw std::bad_alloc();
            }
            field<void*>(block+20)=&g25;
            ++out.size;
        }
        // 445A30 preserves existing sprite vtables; copy construction installs
        // CSpriteEx. All other 252 bytes are value/borrowed fields.
        const int32_t from=sprite(in,i),to=sprite(out,i);
        std::copy_n(pointer<unsigned char>(from),20,pointer<unsigned char>(to));
        std::copy_n(pointer<unsigned char>(from+24),232,pointer<unsigned char>(to+24));
    }
    for(uint32_t i=in.size;i<out.size;++i) field<void*>(sprite(out,i)+20)=&g23;
    out.size=in.size;
}
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
    if(front) while(count && q.size) { pop(q,true);--count; }
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
    if(!front) while(count && q.size) { pop(q,false);--count; }
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
    if(q.capacity<=q.size+1) grow(q);
    auto& block=q.map[(q.first+q.size)%q.capacity];
    if(!block) { block=address(std::malloc(256));if(!block) throw std::bad_alloc(); }
    field<void*>(block+20)=&g25;field<int32_t>(block+24)=0;
    ++q.size;
    return block;
}
