#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/string_layout.h"
#include "kinoko/string_font.h"
#include <stdexcept>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

extern "C" int32_t function_405d60(int32_t texture);

namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;

// 445230 / 423F00: the original assignment copies pixel pointers, not pixel
// buffers. Keep that ownership contract; atlas compaction is not a deep clone.
void assign_renderer(int32_t out,int32_t in) {
    if(out==in) return;
    std::copy_n(pointer<unsigned char>(in),348,pointer<unsigned char>(out));
    kinoko_string_font_copy_pixels(out,in);
    field<uint32_t>(out+360)=field<uint32_t>(in+360);
    field<uint8_t>(out+364)=field<uint8_t>(in+364);
    field<uint8_t>(out+365)=field<uint8_t>(in+365);
    StringView(pointer<void>(out+368)).assign(StringView(pointer<void>(in+368)),0,UINT32_MAX);
    field<uint32_t>(out+396)=field<uint32_t>(in+396);
    field<uint32_t>(out+400)=field<uint32_t>(in+400);
}

// 40ED40 receives its renderer in ESI. The old C signature lost it entirely.
void destroy_renderer(int32_t renderer) {
    kinoko_string_font_destroy_pixels(renderer);
    std::free(field<void*>(renderer+344));
    field<void*>(renderer+344)=nullptr;
    StringView text(pointer<void>(renderer+368));
    text.destroy();
    field<uint32_t>(renderer+388)=15;
    field<uint32_t>(renderer+384)=0;
    field<uint8_t>(renderer+368)=0;
}
struct Atlas {
    alignas(4) unsigned char bytes[436];
    int32_t address() const { return kinoko::legacy::address(bytes); }
    Atlas() { kinoko_string_font_construct(address()+24);field<int32_t>(address()+428)=0; }
    Atlas(const Atlas& other) : Atlas() { *this=other; }
    Atlas& operator=(const Atlas& other) {
        if(this==&other) return *this;
        std::copy_n(other.bytes,24,bytes);
        assign_renderer(address()+24,other.address()+24);
        std::copy_n(other.bytes+428,8,bytes+428);return *this;
    }
    ~Atlas() { destroy_renderer(address()+24); }
};
static_assert(sizeof(Atlas)==436);
using Atlases=std::vector<Atlas>;
Atlases*& atlases(int32_t layout) { return field<Atlases*>(layout+160); }

}

// 441250: VC8 deque<256-byte glyph sprite> has one sprite per block. Its
// proxy/map/map-size/first/size fields start at CStringLayout+176.
extern "C" int32_t function_441250(int32_t* object) {
    const int32_t layout=static_cast<int32_t>(reinterpret_cast<intptr_t>(object));
    int32_t minimum=INT_MAX;
    const uint32_t count=kinoko_string_queue_size(layout);
    for(uint32_t i=0;i<count;++i) {
        const int32_t sprite=kinoko_string_queue_at(layout,i);
        minimum=(std::min)(minimum,field<int32_t>(sprite+8));
    }
    auto& pages=*atlases(layout);
    for(size_t i=0;i<pages.size();) {
        const int32_t atlas=pages[i].address();
        if(field<int32_t>(atlas+20)>=minimum || field<int32_t>(atlas+432)>0) { ++i;continue; }
        function_405d60(field<int32_t>(atlas+428));
        pages.erase(pages.begin()+i);
        i=0;
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
    const uint32_t count=kinoko_string_queue_size(layout);
    for(uint32_t i=0;i<count;++i) {
        const int32_t sprite=kinoko_string_queue_at(layout,i);
        const int32_t atlas=field<int32_t>(sprite+252);
        if(atlas) --field<int32_t>(atlas+432);
    }
    kinoko_string_drop_queue_storage(layout);
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
    atlases(layout)=new Atlases;
    kinoko_string_queue_construct(layout);
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
    kinoko_string_queue_destroy(layout);
    delete atlases(layout);atlases(layout)=nullptr;
    for(int offset : {60,32,4}) {
        StringView value(pointer<void>(layout+offset));
        value.destroy();
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

// Glyph pointers remain borrowed, including across original atlas relocation.
extern "C" int32_t kinoko_string_append_atlas(int32_t layout) {
    atlases(layout)->emplace_back();return atlases(layout)->back().address();
}
extern "C" uint32_t kinoko_string_atlas_size(int32_t layout) { return static_cast<uint32_t>(atlases(layout)->size()); }
extern "C" int32_t kinoko_string_atlas_at(int32_t layout,uint32_t index) { return atlases(layout)->at(index).address(); }

extern "C" void kinoko_string_copy_queue_storage(int32_t object,int32_t source);
extern "C" void kinoko_string_drop_queue_storage(int32_t object);
extern "C" int32_t __fastcall kinoko_method_clone_string_layout(int32_t source,void*) {
    using kinoko::legacy::address;
    const int32_t out=address(std::malloc(260));
    if(!out) return 0;
    kinoko_construct_string_layout(out);
    // 43EC30: three independent strings, scalar style fields, atlas/deque
    // assignment, and all 60 tail bytes. Padding 129..131 remains untouched.
    for(int offset:{4,32,60})
        StringView(pointer<void>(out+offset)).assign(StringView(pointer<void>(source+offset)),0,UINT32_MAX);
    std::copy_n(pointer<unsigned char>(source+88),40,pointer<unsigned char>(out+88));
    field<uint8_t>(out+128)=field<uint8_t>(source+128);
    std::copy_n(pointer<unsigned char>(source+132),28,pointer<unsigned char>(out+132));
    *atlases(out)=*atlases(source);
    kinoko_string_copy_queue_storage(out,source);
    std::copy_n(pointer<unsigned char>(source+200),60,pointer<unsigned char>(out+200));
    // 43EB80 clears cloned atlas values (retains vector capacity, no texture
    // Release), then frees deque blocks/map while retaining its own proxy.
    atlases(out)->clear();
    kinoko_string_drop_queue_storage(out);
    return out;
}
extern "C" int32_t __fastcall kinoko_method_destroy_string_layout(int32_t object,void*) {
    return object?kinoko_method_delete_string_layout(object,nullptr,1):0;
}
extern "C" int32_t g926;
extern "C" int32_t __fastcall kinoko_method_string_layout_type(int32_t,void*) {
    return kinoko::legacy::address(&g926);
}
