#include "kinoko/native_buffer.h"
#include "kinoko/map_render.h"
#include "kinoko/act_host.h"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>

namespace {

using kinoko::legacy::field;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
struct VectorView { unsigned char *begin, *end; void* owner; };
struct Spec { int offset,width,copy_bytes; };
using Layout = kinoko::map::LayoutRecord;
constexpr std::array<Spec,10> vectors={{{offsetof(Layout, placements),32,32},{offsetof(Layout, chip_references),4,4},{offsetof(Layout, texture_references),4,4},
    {offsetof(Layout, render_quads),232,232},
    {offsetof(Layout, render_reference_buffers),4,4},{offsetof(Layout, render_reference_buffers)+16,12,12},{offsetof(kinoko::map::LayoutRecord,chip_sprites),sizeof(kinoko::map::ChipSpriteCache),281},
    {offsetof(kinoko::map::LayoutRecord,chip_definitions),sizeof(kinoko::map::ChipDefinition),48},
    {offsetof(kinoko::map::LayoutRecord,changed_chips),sizeof(kinoko::map::ChipDefinition*),4},
    {offsetof(kinoko::map::LayoutRecord,chip_indices),sizeof(int32_t),4}}};
struct Free { void operator()(void *p) const { std::free(p); } };
using Owned=std::unique_ptr<void,Free>;
Owned allocate(size_t size) {
    Owned value(std::malloc(size));if(!value) throw std::bad_alloc();return value;
}
}
extern "C" KinokoActLayout* __fastcall kinoko_clone_map_layout(KinokoActLayout* source,void*) {
    // This is 433AA0's full virtual clone, not ACT activation's cache reset.
    Owned output=allocate(sizeof(Layout));
    auto* result=static_cast<unsigned char*>(output.get());
    const auto* input=reinterpret_cast<const unsigned char*>(source);
    kinoko::legacy::store(result, kinoko_act_host_symbols()->map_layout_vtable);
    kinoko::legacy::store(result+sizeof(void*), kinoko_act_host_symbols()->map_view_vtable);
    std::memcpy(result+8,input+8,228);
    for(auto range: {std::pair<int,int>{236,28},{312,20},{380,4},{400,4},{452,8}})
        std::memcpy(result+range.first,input+range.first,range.second);
    std::array<std::vector<uint32_t>,vectors.size()> buffers;
    for(size_t i=0;i<vectors.size();++i) {
        const auto spec=vectors[i];const auto in=kinoko::legacy::load<VectorView>(input+spec.offset);
        const auto bytes=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(in.end)-reinterpret_cast<uintptr_t>(in.begin));
        if(reinterpret_cast<intptr_t>(in.end)<reinterpret_cast<intptr_t>(in.begin) || bytes%spec.width || bytes>0x7fffffffu) throw std::bad_alloc();
        kinoko::legacy::store(result+spec.offset, VectorView{});
        if(!bytes) continue;
        buffers[i].resize(bytes/4);
        auto* begin=reinterpret_cast<unsigned char*>(buffers[i].data());
        for(uint32_t pos=0;pos<bytes;pos+=spec.width) {
            std::memcpy(begin+pos,in.begin+pos,spec.copy_bytes);
            if(spec.width==232 || spec.width==288) kinoko::legacy::store(begin+pos, kinoko_act_host_symbols()->chip_quad_vtable);
        }

    }
    // Fourth words between vector views are untouched, as in 433780/433AE0.
    kinoko::map::LayoutView(output.get()).set(&kinoko::map::LayoutRecord::suppress_next_binding, uint8_t{1});
    try {
        for(size_t i=0;i<vectors.size();++i)
            kinoko_native_buffer_replace(result+vectors[i].offset, buffers[i].data(), static_cast<uint32_t>(buffers[i].size()*4));
    } catch(...) {
        for(auto spec:vectors) kinoko_native_buffer_destroy(result+spec.offset);
        throw;
    }
    return static_cast<KinokoActLayout*>(output.release());
}
extern "C" void kinoko_clear_map_layout(KinokoActLayout* layout) {
    if (!layout) return;
    auto* bytes = reinterpret_cast<unsigned char*>(layout);
    kinoko::legacy::store(bytes, kinoko_act_host_symbols()->map_layout_vtable);
    kinoko::legacy::store(bytes + sizeof(void*), kinoko_act_host_symbols()->map_view_vtable);
    for (auto it = vectors.rbegin(); it != vectors.rend(); ++it) {
        const auto view = kinoko::legacy::load<VectorView>(bytes + it->offset);
        // Sprite destructors reset identity; texture handles remain borrowed.
        if (it->width == 232 || it->width == 288)
            for (auto* p = view.begin; p != view.end; p += it->width)
                kinoko::legacy::store(p, kinoko_act_host_symbols()->color_vtable);
        kinoko_native_buffer_destroy(bytes + it->offset);
    }
    kinoko::legacy::store(bytes + sizeof(void*), kinoko_act_host_symbols()->color_vtable);
}
extern "C" void* __fastcall kinoko_delete_map_sprite(void* sprite, void*, int32_t flags) {
    auto* layout = static_cast<unsigned char*>(sprite) - sizeof(void*);
    if (flags & 2) {
        auto* allocation = layout - sizeof(uint32_t);
        const auto count = kinoko::legacy::load<uint32_t>(allocation);
        for (auto i = count; i > 0; --i)
            kinoko_clear_map_layout(reinterpret_cast<KinokoActLayout*>(layout + sizeof(Layout)*(i-1)));
        if (flags & 1) std::free(allocation);
        return allocation;
    }
    kinoko_clear_map_layout(reinterpret_cast<KinokoActLayout*>(layout));
    if (flags & 1) std::free(layout);
    return layout;
}
