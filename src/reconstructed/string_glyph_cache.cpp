#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/string_layout.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <new>
#include <string>

extern "C" int32_t function_405d60(int32_t texture);
extern "C" int32_t g350;

namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
struct Node { Node *next, *previous; void* pixels; };
static_assert(sizeof(Node)==12);

// 445230 / 423F00: the original assignment copies pixel pointers, not pixel
// buffers. Keep that ownership contract; atlas compaction is not a deep clone.
void assign_renderer(int32_t out,int32_t in) {
    if(out==in) return;
    std::copy_n(pointer<unsigned char>(in),348,pointer<unsigned char>(out));
    auto* head=field<Node*>(out+348);
    auto* source=field<Node*>(in+348);
    auto* node=head->next;
    head->next=head->previous=head;
    field<uint32_t>(out+352)=0;
    while(node!=head) {
        auto* next=node->next;
        std::free(node);
        node=next;
    }
    for(node=source->next;node!=source;node=node->next) {
        auto* copy=static_cast<Node*>(std::malloc(sizeof(Node)));
        if(!copy) throw std::bad_alloc();
        *copy={head,head->previous,node->pixels};
        head->previous->next=copy;
        head->previous=copy;
        ++field<uint32_t>(out+352);
    }
    field<uint32_t>(out+360)=field<uint32_t>(in+360);
    field<uint8_t>(out+364)=field<uint8_t>(in+364);
    field<uint8_t>(out+365)=field<uint8_t>(in+365);
    StringView(pointer<void>(out+368)).assign(StringView(pointer<void>(in+368)),0,UINT32_MAX);
    field<uint32_t>(out+396)=field<uint32_t>(in+396);
    field<uint32_t>(out+400)=field<uint32_t>(in+400);
}

// 40ED40 receives its renderer in ESI. The old C signature lost it entirely.
void destroy_renderer(int32_t renderer) {
    auto* head=field<Node*>(renderer+348);
    auto* node=head->next;
    while(node!=head) {
        auto* next=node->next;
        std::free(node->pixels);
        std::free(node);
        node=next;
    }
    field<uint32_t>(renderer+352)=0;
    std::free(field<void*>(renderer+344));
    field<void*>(renderer+344)=nullptr;
    StringView text(pointer<void>(renderer+368));
    if(text.is_heap()) std::free(text.data());
    field<uint32_t>(renderer+388)=15;
    field<uint32_t>(renderer+384)=0;
    field<uint8_t>(renderer+368)=0;
    std::free(head);
}
}

// 441250: VC8 deque<256-byte glyph sprite> has one sprite per block. Its
// proxy/map/map-size/first/size fields start at CStringLayout+176.
extern "C" int32_t function_441250(int32_t* object) {
    const int32_t layout=static_cast<int32_t>(reinterpret_cast<intptr_t>(object));
    int32_t minimum=INT_MAX;
    const uint32_t count=field<uint32_t>(layout+192);
    const uint32_t first=field<uint32_t>(layout+188);
    const uint32_t map_size=field<uint32_t>(layout+184);
    auto* map=field<int32_t*>(layout+180);
    for(uint32_t i=0;i<count;++i) {
        const int32_t sprite=map[(first+i)%map_size];
        minimum=(std::min)(minimum,field<int32_t>(sprite+8));
    }
    const int32_t begin=field<int32_t>(layout+160);
    auto& end=field<int32_t>(layout+164);
    for(int32_t atlas=begin;atlas!=end;) {
        if(field<int32_t>(atlas+20)>=minimum || field<int32_t>(atlas+432)>0) {
            atlas+=436;
            continue;
        }
        function_405d60(field<int32_t>(atlas+428));
        // 444A50 assigns forward through existing renderer/list/string storage.
        for(int32_t out=atlas,in=atlas+436;in!=end;out+=436,in+=436) {
            std::copy_n(pointer<unsigned char>(in),24,pointer<unsigned char>(out));
            assign_renderer(out+24,in+24);
            field<uint32_t>(out+428)=field<uint32_t>(in+428);
            field<uint32_t>(out+432)=field<uint32_t>(in+432);
        }
        destroy_renderer(end-436+24);
        end-=436;
        atlas=begin;
    }
    return 1;
}

// 4410C0 rebuilds the pending byte string and discards glyph sprites. It does
// not rasterize text here: later update consumes stBackQueue using CharNextA.
extern "C" int32_t function_4410c0(int32_t layout) {
    StringView text(pointer<void>(layout+4)), queue(pointer<void>(layout+32));
    std::string pending(text.data(),text.length());
    pending.append(queue.data(),queue.length());
    text.assign("",0);
    queue.assign("",0);
    field<uint8_t>(layout+228)=1;
    field<int32_t>(layout+204)=field<int32_t>(layout+208)=0;
    field<int32_t>(layout+216)=field<int32_t>(layout+88);
    field<int32_t>(layout+212)=0;
    // Original passes the concatenated buffer through strlen (441160).
    queue.append(pending.c_str(),static_cast<uint32_t>(std::strlen(pending.c_str())));
    const uint32_t count=field<uint32_t>(layout+192);
    const uint32_t first=field<uint32_t>(layout+188);
    const uint32_t map_size=field<uint32_t>(layout+184);
    auto* map=field<int32_t*>(layout+180);
    for(uint32_t i=0;i<count;++i) {
        const int32_t sprite=map[(first+i)%map_size];
        const int32_t atlas=field<int32_t>(sprite+252);
        if(atlas) --field<int32_t>(atlas+432);
    }
    // 442D20 clears all allocated one-element deque blocks and the map, but
    // retains its proxy at +176. Sprite destruction owns no atlas or texture.
    for(uint32_t i=map_size;i>0;--i) std::free(pointer<void>(map[i-1]));
    std::free(map);
    field<int32_t>(layout+180)=field<int32_t>(layout+184)=0;
    if(count) field<int32_t>(layout+188)=0;
    field<int32_t>(layout+192)=0;
    return function_441250(pointer<int32_t>(layout));
}

// 43E890 ends at 43EA0F. RetDec erroneously included 43EA10's destructor
// after its allocation-failure throw and lost the constructor's ECX receiver.
extern "C" int32_t kinoko_construct_string_layout(int32_t layout) {
    field<void*>(layout)=&g350;
    for(int offset : {4,32,60}) {
        field<uint32_t>(layout+offset+20)=15;
        field<uint32_t>(layout+offset+16)=0;
        field<uint8_t>(layout+offset)=0;
    }
    for(int offset : {160,164,168,176,180,184,188,192}) field<int32_t>(layout+offset)=0;
    auto* proxy=static_cast<int32_t*>(std::malloc(8));
    if(!proxy) throw std::bad_alloc();
    proxy[0]=layout+176;proxy[1]=0;
    field<void*>(layout+176)=proxy;
    // Original CP932 face name, 13 bytes before its NUL terminator.
    static const char face[]="\x82\x6c\x82\x72\x20\x83\x53\x83\x56\x83\x62\x83\x4e";
    StringView(pointer<void>(layout+60)).assign(face,13);
    field<float>(layout+136)=field<float>(layout+140)=field<float>(layout+152)=1.0f;
    field<int32_t>(layout+148)=0;field<int32_t>(layout+156)=1;
    field<int32_t>(layout+108)=field<int32_t>(layout+112)=field<int32_t>(layout+116)=255;
    field<int32_t>(layout+88)=16;field<int32_t>(layout+92)=1;
    for(int offset : {96,100,104,120,132,204,208,212,220,224}) field<int32_t>(layout+offset)=0;
    field<int32_t>(layout+124)=2;field<int32_t>(layout+144)=-1;
    field<int32_t>(layout+216)=16;
    field<uint8_t>(layout+128)=field<uint8_t>(layout+228)=0;
    return layout;
}

extern "C" void kinoko_clear_string_layout(int32_t layout) {
    field<void*>(layout)=&g350;
    StringView(pointer<void>(layout+4)).assign("",0);
    StringView(pointer<void>(layout+32)).assign("",0);
    function_4410c0(layout);
    function_441250(pointer<int32_t>(layout));
    std::free(field<void*>(layout+176));
    field<void*>(layout+176)=nullptr;
    const int32_t end=field<int32_t>(layout+164);
    for(int32_t atlas=field<int32_t>(layout+160);atlas!=end;atlas+=436)
        destroy_renderer(atlas+24);
    std::free(field<void*>(layout+160));
    field<int32_t>(layout+160)=field<int32_t>(layout+164)=field<int32_t>(layout+168)=0;
    for(int offset : {60,32,4}) {
        StringView value(pointer<void>(layout+offset));
        if(value.is_heap()) std::free(value.data());
        field<uint32_t>(layout+offset+20)=15;
        field<uint32_t>(layout+offset+16)=0;
        field<uint8_t>(layout+offset)=0;
    }
}

extern "C" int32_t __fastcall kinoko_method_delete_string_layout(int32_t object,void*,unsigned char flags) {
    if(flags&2) {
        const uint32_t count=field<uint32_t>(object-4);
        for(uint32_t i=count;i>0;--i) kinoko_clear_string_layout(object+(i-1)*260);
        if(flags&1) std::free(pointer<void>(object-4));
        return object-4;
    }
    kinoko_clear_string_layout(object);
    if(flags&1) std::free(pointer<void>(object));
    return object;
}
