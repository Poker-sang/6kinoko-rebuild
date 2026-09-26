#include "kinoko/texture_store.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/string_layout.h"
#include "kinoko/string_font.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/string_atlas_records.hpp"
#include "kinoko/string_font_renderer_records.hpp"
#include <stdexcept>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>


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
    using Renderer=kinoko::text::FontRendererRecord;
    const kinoko::native::RecordView<Renderer> target(pointer<void>(out)), source(pointer<void>(in));
    StringView(target.bytes(&Renderer::label)).assign(StringView(source.bytes(&Renderer::label)),0,UINT32_MAX);
    field<uint32_t>(out+396)=field<uint32_t>(in+396);
    field<uint32_t>(out+400)=field<uint32_t>(in+400);
}

// 40ED40 receives its renderer in ESI. The old C signature lost it entirely.
void destroy_renderer(int32_t renderer) {
    kinoko_string_font_destroy_pixels(renderer);
    using Renderer=kinoko::text::FontRendererRecord;
    using StringRecord=kinoko::legacy::StringRecord;
    const kinoko::native::RecordView<Renderer> record(pointer<void>(renderer));
    std::free(record.get(&Renderer::bitmap));
    record.set(&Renderer::bitmap,static_cast<void*>(nullptr));
    const kinoko::native::RecordView<StringRecord> label(record.bytes(&Renderer::label));
    StringView(label.data()).destroy();
    label.set(&StringRecord::capacity,uint32_t{15});
    label.set(&StringRecord::length,uint32_t{0});
    label.bytes(&StringRecord::characters)[0]=0;
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
using LayoutRecord=kinoko::act::StringLayoutRecord;
Atlases* atlases(int32_t layout) {
    const kinoko::native::RecordView<LayoutRecord> record(pointer<void>(layout));
    return static_cast<Atlases*>(record.get(&LayoutRecord::atlas_owner));
}
void set_atlases(int32_t layout,Atlases* value) {
    const kinoko::native::RecordView<LayoutRecord> record(pointer<void>(layout));
    record.set(&LayoutRecord::atlas_owner,static_cast<void*>(value));
}

}

// 441250: VC8 deque<256-byte glyph sprite> has one sprite per block. Its
// proxy/map/map-size/first/size fields start at CStringLayout+176.
extern "C" int32_t kinoko_string_prune_atlases(KinokoStringLayout* receiver) {
    const int32_t layout=kinoko::legacy::address(receiver);
    int32_t minimum=INT_MAX;
    const uint32_t count=kinoko_string_queue_size((KinokoStringLayout*)(uintptr_t)(layout));
    for(uint32_t i=0;i<count;++i) {
        const int32_t sprite=kinoko_string_queue_at((KinokoStringLayout*)(uintptr_t)(layout), i);
        minimum=(std::min)(minimum,kinoko::native::RecordView<kinoko::act::StringGlyphRecord>(pointer<void>(sprite)).get(&kinoko::act::StringGlyphRecord::id));
    }
    auto& pages=*atlases(layout);
    for(size_t i=0;i<pages.size();) {
        const int32_t atlas=pages[i].address();
        const kinoko::native::RecordView<kinoko::text::AtlasLifecycle> lifetime(pointer<void>(atlas));
        if(lifetime.get(&kinoko::text::AtlasLifecycle::last_glyph_id)>=minimum ||
           lifetime.get(&kinoko::text::AtlasLifecycle::references)>0) { ++i;continue; }
        kinoko_texture_release(lifetime.get(&kinoko::text::AtlasLifecycle::texture));
        pages.erase(pages.begin()+i);
        i=0;
    }
    return 1;
}

// 4410C0 rebuilds the pending byte string and discards glyph sprites. It does
// not rasterize text here: later update consumes stBackQueue using CharNextA.
extern "C" int32_t kinoko_string_rebuild_queue(KinokoStringLayout* receiver) {
    const auto layout=kinoko::legacy::address(receiver);
    const kinoko::native::RecordView<kinoko::act::StringLayoutRecord> record(receiver);
    StringView text(record.bytes(&kinoko::act::StringLayoutRecord::text));
    StringView queue(record.bytes(&kinoko::act::StringLayoutRecord::pending));
    std::string pending(text.data(),text.length());
    pending.append(queue.data(),queue.length());
    text.assign("",0);
    queue.assign("",0);
    using Layout=kinoko::act::StringLayoutRecord;
    record.set(&Layout::rebuild,uint8_t{1});
    record.set(&Layout::cursor_x,0);
    record.set(&Layout::cursor_y,0);
    record.set(&Layout::line_height,record.get(&Layout::font_height));
    record.set(&Layout::maximum_width,0);
    // Original passes the concatenated buffer through strlen (441160).
    queue.append(pending.c_str(),static_cast<uint32_t>(std::strlen(pending.c_str())));
    const uint32_t count=kinoko_string_queue_size((KinokoStringLayout*)(uintptr_t)(layout));
    for(uint32_t i=0;i<count;++i) {
        const int32_t sprite=kinoko_string_queue_at((KinokoStringLayout*)(uintptr_t)(layout), i);
        const auto glyph=kinoko::native::RecordView<kinoko::act::StringGlyphRecord>(pointer<void>(sprite));
        auto* atlas=glyph.get(&kinoko::act::StringGlyphRecord::atlas);
        if(atlas) {
            const kinoko::native::RecordView<kinoko::text::AtlasLifecycle> lifetime(atlas);
            lifetime.set(&kinoko::text::AtlasLifecycle::references,
                lifetime.get(&kinoko::text::AtlasLifecycle::references)-1);
        }
    }
    kinoko_string_drop_queue_storage((KinokoStringLayout*)(uintptr_t)(layout));
    return kinoko_string_prune_atlases(receiver);
}

// 43E890 ends at 43EA0F. RetDec erroneously included 43EA10's destructor
// after its allocation-failure throw and lost the constructor's ECX receiver.
extern "C" int32_t kinoko_construct_string_layout(int32_t layout) {
    field<void*>(layout)=const_cast<void*>(kinoko_string_layout_methods());
    using Layout=kinoko::act::StringLayoutRecord;
    using StringRecord=kinoko::legacy::StringRecord;
    const kinoko::native::RecordView<Layout> text(pointer<void>(layout));
    for (auto member : {&Layout::text, &Layout::pending, &Layout::face}) {
        const kinoko::native::RecordView<StringRecord> value(text.bytes(member));
        value.set(&StringRecord::capacity, uint32_t{15});
        value.set(&StringRecord::length, uint32_t{0});
        value.bytes(&StringRecord::characters)[0] = 0;
    }
    for(int offset : {160,164,168,176,180,184,188,192}) field<int32_t>(layout+offset)=0;
    set_atlases(layout,new Atlases);
    kinoko_string_queue_construct((KinokoStringLayout*)(uintptr_t)(layout));
    // Original CP932 face name, 13 bytes before its NUL terminator.
    static const char face[]="\x82\x6c\x82\x72\x20\x83\x53\x83\x56\x83\x62\x83\x4e";
    StringView(text.bytes(&Layout::face)).assign(face,13);
    text.set(&Layout::alpha,1.0f);
    text.set(&Layout::layer,static_cast<KinokoActLayer*>(nullptr));
    text.set(&Layout::blend,1);
    text.set(&Layout::scale_x,1.0f);
    text.set(&Layout::scale_y,1.0f);
    text.set(&Layout::base_red,255);
    text.set(&Layout::base_green,255);
    text.set(&Layout::base_blue,255);
    text.set(&Layout::font_height,16);
    text.set(&Layout::font_weight,1);
    text.set(&Layout::red,0);
    text.set(&Layout::green,0);
    text.set(&Layout::blue,0);
    text.set(&Layout::character_space,0);
    text.set(&Layout::line_space,2);
    text.set(&Layout::edge,uint8_t{0});
    text.set(&Layout::alignment,0);
    text.set(&Layout::wrap_width,-1);
    text.set(&Layout::origin_x,0);
    text.set(&Layout::origin_y,0);
    text.set(&Layout::rebuild,uint8_t{0});
    text.set(&Layout::cursor_y,0);
    text.set(&Layout::cursor_x,0);
    text.set(&Layout::line_height,16);
    text.set(&Layout::maximum_width,0);
    return layout;
}

extern "C" void kinoko_clear_string_layout(int32_t layout) {
    field<void*>(layout)=const_cast<void*>(kinoko_string_layout_methods());
    using Layout=kinoko::act::StringLayoutRecord;
    using StringRecord=kinoko::legacy::StringRecord;
    const kinoko::native::RecordView<Layout> text(pointer<void>(layout));
    StringView(text.bytes(&Layout::text)).assign("",0);
    StringView(text.bytes(&Layout::pending)).assign("",0);
    kinoko_string_rebuild_queue(pointer<KinokoStringLayout>(layout));
    kinoko_string_prune_atlases(pointer<KinokoStringLayout>(layout));
    kinoko_string_queue_destroy((KinokoStringLayout*)(uintptr_t)(layout));
    delete atlases(layout);set_atlases(layout,nullptr);
    // Original 43EA10 releases face, pending and displayed text in that order.
    for(auto member : {&Layout::face,&Layout::pending,&Layout::text}) {
        const kinoko::native::RecordView<StringRecord> string(text.bytes(member));
        StringView(string.data()).destroy();
        string.set(&StringRecord::capacity,uint32_t{15});
        string.set(&StringRecord::length,uint32_t{0});
        string.bytes(&StringRecord::characters)[0]=0;
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

extern "C" void kinoko_string_copy_queue_storage(KinokoStringLayout* object,KinokoStringLayout* source);
extern "C" void kinoko_string_drop_queue_storage(KinokoStringLayout* object);
extern "C" int32_t __fastcall kinoko_method_clone_string_layout(int32_t source,void*) {
    using kinoko::legacy::address;
    const int32_t out=address(std::malloc(260));
    if(!out) return 0;
    kinoko_construct_string_layout(out);
    // 43EC30: three independent strings, scalar style fields, atlas/deque
    // assignment, and all 60 tail bytes. Padding 129..131 remains untouched.
    using Layout=kinoko::act::StringLayoutRecord;
    const kinoko::native::RecordView<Layout> target(pointer<void>(out)), origin(pointer<void>(source));
    for(auto member : {&Layout::text,&Layout::pending,&Layout::face})
        StringView(target.bytes(member)).assign(StringView(origin.bytes(member)),0,UINT32_MAX);
    std::copy_n(origin.bytes(&Layout::font_height),40,target.bytes(&Layout::font_height));
    target.set(&Layout::edge,origin.get(&Layout::edge));
    std::copy_n(origin.bytes(&Layout::alignment),28,target.bytes(&Layout::alignment));
    *atlases(out)=*atlases(source);
    kinoko_string_copy_queue_storage((KinokoStringLayout*)(uintptr_t)(out), (KinokoStringLayout*)(uintptr_t)(source));
    std::copy_n(origin.bytes(&Layout::next_glyph_id),60,target.bytes(&Layout::next_glyph_id));
    // 43EB80 clears cloned atlas values (retains vector capacity, no texture
    // Release), then frees deque blocks/map while retaining its own proxy.
    atlases(out)->clear();
    kinoko_string_drop_queue_storage((KinokoStringLayout*)(uintptr_t)(out));
    return out;
}
extern "C" int32_t __fastcall kinoko_method_destroy_string_layout(int32_t object,void*) {
    return object?kinoko_method_delete_string_layout(object,nullptr,1):0;
}
extern "C" int32_t kinoko_string_layout_type_identity;
namespace { inline auto string_layout_type_info = &kinoko_string_layout_type_identity; }
extern "C" int32_t __fastcall kinoko_method_string_layout_type(int32_t,void*) {
    return kinoko::legacy::address(string_layout_type_info);
}

