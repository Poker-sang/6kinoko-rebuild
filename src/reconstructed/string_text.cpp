#include "kinoko/string_layout.h"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/string_atlas_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include <windows.h>
#include <algorithm>
#include <deque>
#include <cstdlib>
#include <new>
#include <stdexcept>

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
using LayoutRecord=kinoko::act::StringLayoutRecord;
Deque* storage(int32_t layout) {
    const kinoko::native::RecordView<LayoutRecord> record(pointer<void>(layout));
    return static_cast<Deque*>(record.get(&LayoutRecord::glyph_owner));
}
void set_storage(int32_t layout,Deque* value) {
    const kinoko::native::RecordView<LayoutRecord> record(pointer<void>(layout));
    record.set(&LayoutRecord::glyph_owner,static_cast<void*>(value));
}
Deque& queue(int32_t layout) { return *storage(layout); }
int32_t sprite(const Deque& q,uint32_t i) { return address(q[i].bytes); }
using Layout=kinoko::act::StringLayoutRecord;
using GlyphRecord=kinoko::act::StringGlyphRecord;
void adjust_atlas_reference(int32_t glyph_address,int delta) {
    const kinoko::native::RecordView<GlyphRecord> glyph(pointer<void>(glyph_address));
    auto* atlas=glyph.get(&GlyphRecord::atlas);
    if(!atlas) return;
    const kinoko::native::RecordView<kinoko::text::AtlasLifecycle> lifetime(atlas);
    lifetime.set(&kinoko::text::AtlasLifecycle::references,
        lifetime.get(&kinoko::text::AtlasLifecycle::references)+delta);
}
void references(const Deque& q,int delta) {
    for(uint32_t i=0;i<q.size();++i) {
        adjust_atlas_reference(sprite(q,i),delta);
    }
}
void pop(Deque& q,bool front) {
    const int32_t value=sprite(q,front?0:static_cast<uint32_t>(q.size()-1));
    adjust_atlas_reference(value,-1);
    if(front) q.pop_front();else q.pop_back();
}
void assign(Deque& out,const Deque& in) { if(&out!=&in) out=in; }

}

extern "C" int32_t kinoko_string_push_back(int32_t object,const char* text) {
    if(!text) return 0;
    const kinoko::native::RecordView<Layout> layout(pointer<void>(object));
    StringView(layout.bytes(&Layout::pending)).append(text,static_cast<uint32_t>(std::strlen(text)));
    return 1;
}
extern "C" int32_t kinoko_string_mark_rebuild(int32_t object) {
    kinoko::native::RecordView<Layout>(pointer<void>(object)).set(&Layout::rebuild,uint8_t{1});
    return 1;
}
extern "C" int32_t kinoko_string_clear(int32_t object) {
    const kinoko::native::RecordView<Layout> layout(pointer<void>(object));
    StringView(layout.bytes(&Layout::text)).assign("",0);
    StringView(layout.bytes(&Layout::pending)).assign("",0);
    layout.set(&Layout::cursor_x,0);
    layout.set(&Layout::cursor_y,0);
    layout.set(&Layout::maximum_width,0);
    layout.set(&Layout::line_height,layout.get(&Layout::font_height));
    return kinoko_string_mark_rebuild(object);
}
extern "C" int32_t kinoko_string_character_bytes(const char* text) {
    return text && *text?static_cast<int32_t>(CharNextA(text)-text):0;
}
extern "C" int32_t kinoko_string_pop(int32_t object,int32_t count,int32_t front) {
    if(count<0) return 0;
    if(!count) return 1;
    auto& q=queue(object);
    const kinoko::native::RecordView<Layout> layout(pointer<void>(object));
    StringView pending(layout.bytes(&Layout::pending));
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
    return kinoko_string_prune_atlases(pointer<KinokoStringLayout>(object));
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

extern "C" void kinoko_string_queue_construct(int32_t object) { set_storage(object,new Deque); }
extern "C" void kinoko_string_queue_destroy(int32_t object) { delete storage(object);set_storage(object,nullptr); }
extern "C" uint32_t kinoko_string_queue_size(int32_t object) { return static_cast<uint32_t>(queue(object).size()); }
extern "C" int32_t kinoko_string_queue_at(int32_t object,uint32_t index) { return address(queue(object).at(index).bytes); }
