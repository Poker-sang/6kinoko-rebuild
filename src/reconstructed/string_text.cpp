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

#include "kinoko/act_host.h"
namespace {


using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::StringView;
struct Glyph {
    alignas(4) unsigned char bytes[256];
    Glyph() { auto quad=kinoko::native::RecordView<kinoko::act::StringGlyphRecord>(bytes).view(&kinoko::act::StringGlyphRecord::quad); quad.set(&kinoko::render::QuadRecord::vtable, kinoko_act_host_symbols()->chip_quad_vtable); quad.set(&kinoko::render::QuadRecord::texture, int32_t{0}); }
    Glyph(const Glyph& other) : Glyph() { *this=other; }
    Glyph& operator=(const Glyph& other) {
        std::copy_n(other.bytes,20,bytes);
        std::copy_n(other.bytes+24,232,bytes+24);return *this;
    }
    ~Glyph() { kinoko::native::RecordView<kinoko::act::StringGlyphRecord>(bytes).view(&kinoko::act::StringGlyphRecord::quad).set(&kinoko::render::QuadRecord::vtable, kinoko_act_host_symbols()->color_vtable); }
};
using Deque=std::deque<Glyph>;
using LayoutRecord=kinoko::act::StringLayoutRecord;
Deque* storage(KinokoStringLayout* layout) {
    const kinoko::native::RecordView<LayoutRecord> record(layout);
    return static_cast<Deque*>(record.get(&LayoutRecord::glyph_owner));
}
void set_storage(KinokoStringLayout* layout,Deque* value) {
    const kinoko::native::RecordView<LayoutRecord> record(layout);
    record.set(&LayoutRecord::glyph_owner,static_cast<void*>(value));
}
Deque& queue(KinokoStringLayout* layout) { return *storage(layout); }
const void* sprite(const Deque& q,uint32_t i) { return q[i].bytes; }
using Layout=kinoko::act::StringLayoutRecord;
using GlyphRecord=kinoko::act::StringGlyphRecord;
void adjust_atlas_reference(const void* glyph_address,int delta) {
    const kinoko::native::RecordView<GlyphRecord> glyph(const_cast<void*>(glyph_address));
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
    const void* value=sprite(q,front?0:static_cast<uint32_t>(q.size()-1));
    adjust_atlas_reference(value,-1);
    if(front) q.pop_front();else q.pop_back();
}
void assign(Deque& out,const Deque& in) { if(&out!=&in) out=in; }

}

extern "C" int32_t kinoko_string_push_back(KinokoStringLayout* object,const char* text) {
    if(!text) return 0;
    const kinoko::native::RecordView<Layout> layout(object);
    StringView(layout.bytes(&Layout::pending)).append(text,static_cast<uint32_t>(std::strlen(text)));
    return 1;
}
extern "C" int32_t kinoko_string_mark_rebuild(KinokoStringLayout* object) {
    kinoko::native::RecordView<Layout>(object).set(&Layout::rebuild,uint8_t{1});
    return 1;
}
extern "C" int32_t kinoko_string_clear(KinokoStringLayout* object) {
    const kinoko::native::RecordView<Layout> layout(object);
    StringView(layout.bytes(&Layout::text)).assign("",0);
    StringView(layout.bytes(&Layout::pending)).assign("",0);
    layout.set(&Layout::cursor_x,0);
    layout.set(&Layout::cursor_y,0);
    layout.set(&Layout::maximum_width,0);
    layout.set(&Layout::line_height,layout.get(&Layout::font_height));
    return kinoko_string_mark_rebuild((KinokoStringLayout*)(uintptr_t)(object));
}
extern "C" int32_t kinoko_string_character_bytes(const char* text) {
    return text && *text?static_cast<int32_t>(CharNextA(text)-text):0;
}
extern "C" int32_t kinoko_string_pop(KinokoStringLayout* object,int32_t count,int32_t front) {
    if(count<0) return 0;
    if(!count) return 1;
    auto& q=queue(object);
    const kinoko::native::RecordView<Layout> layout(object);
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
    return kinoko_string_prune_atlases(object);
}
extern "C" int32_t kinoko_string_replicate(KinokoStringLayout* object,KinokoStringLayout* source) {
    if(!source) return 0;
    auto& out=queue(object);
    references(out,-1);
    assign(out,queue(source));
    references(out,1);
    return 1;
}

// 442300 appends a default CSpriteEx value into the one-element block deque.
// The original leaves non-vtable/default-texture fields for 404EE0 to fill.
extern "C" void* kinoko_string_append_glyph(KinokoStringLayout* layout) {
    auto& q=queue(layout);
    q.emplace_back();
    return q.back().bytes;
}

// 43EC30 copies deque values without adjusting atlas references; 43EB80
// immediately destroys that copied queue without decrementing references.
extern "C" void kinoko_string_copy_queue_storage(KinokoStringLayout* object,KinokoStringLayout* source) {
    assign(queue(object),queue(source));
}
extern "C" void kinoko_string_drop_queue_storage(KinokoStringLayout* object) {
    queue(object).clear();
}

extern "C" void kinoko_string_queue_construct(KinokoStringLayout* object) { set_storage(object,new Deque); }
extern "C" void kinoko_string_queue_destroy(KinokoStringLayout* object) { delete storage(object);set_storage(object,nullptr); }
extern "C" uint32_t kinoko_string_queue_size(KinokoStringLayout* object) { return static_cast<uint32_t>(queue(object).size()); }
extern "C" void* kinoko_string_queue_at(KinokoStringLayout* object,uint32_t index) { return queue(object).at(index).bytes; }
