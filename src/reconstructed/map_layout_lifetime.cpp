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
struct VectorView { int32_t begin,end,capacity; };
struct Spec { int offset,width,copy_bytes; };
constexpr std::array<Spec,10> vectors={{{264,32,32},{280,4,4},{296,4,4},
    {332,232,232},{348,4,4},{364,12,12},{offsetof(kinoko::map::LayoutRecord,chip_sprites),sizeof(kinoko::map::ChipSpriteCache),281},
    {offsetof(kinoko::map::LayoutRecord,chip_definitions),sizeof(kinoko::map::ChipDefinition),48},
    {offsetof(kinoko::map::LayoutRecord,changed_chips),sizeof(kinoko::map::ChipDefinition*),4},
    {offsetof(kinoko::map::LayoutRecord,chip_indices),sizeof(int32_t),4}}};
struct Free { void operator()(void *p) const { std::free(p); } };
using Owned=std::unique_ptr<void,Free>;
Owned allocate(size_t size) {
    Owned value(std::malloc(size));if(!value) throw std::bad_alloc();return value;
}
}
extern "C" int32_t __fastcall kinoko_clone_map_layout(int32_t source,void*) {
    // This is 433AA0's full virtual clone, not ACT activation's cache reset.
    Owned output=allocate(464);
    const auto result=address(output.get());
    field<int32_t>(result)=kinoko::legacy::address(kinoko_act_host_symbols()->map_layout_vtable);field<int32_t>(result+4)=kinoko::legacy::address(kinoko_act_host_symbols()->map_view_vtable);
    std::memcpy(pointer<void>(result+8),pointer<void>(source+8),228);
    for(auto range: {std::pair<int,int>{236,28},{312,20},{380,4},{400,4},{452,8}})
        std::memcpy(pointer<void>(result+range.first),pointer<void>(source+range.first),range.second);
    std::array<std::vector<uint32_t>,vectors.size()> buffers;
    for(size_t i=0;i<vectors.size();++i) {
        const auto spec=vectors[i];const auto in=field<VectorView>(source+spec.offset);
        const auto bytes=static_cast<uint32_t>(in.end)-static_cast<uint32_t>(in.begin);
        if(in.end<in.begin || bytes%spec.width || bytes>0x7fffffffu) throw std::bad_alloc();
        auto &out=field<VectorView>(result+spec.offset);out={};
        if(!bytes) continue;
        buffers[i].resize(bytes/4);
        const auto begin=address(buffers[i].data());
        for(uint32_t pos=0;pos<bytes;pos+=spec.width) {
            std::memcpy(pointer<void>(begin+pos),pointer<void>(in.begin+pos),spec.copy_bytes);
            if(spec.width==232 || spec.width==288) field<int32_t>(begin+pos)=address(kinoko_act_host_symbols()->chip_quad_vtable);
        }

    }
    // Fourth words between vector views are untouched, as in 433780/433AE0.
    kinoko::map::LayoutView(output.get()).set(&kinoko::map::LayoutRecord::suppress_next_binding, uint8_t{1});
    try {
        for(size_t i=0;i<vectors.size();++i)
            kinoko_native_buffer_replace(result+vectors[i].offset,buffers[i].data(),static_cast<uint32_t>(buffers[i].size()*4));
    } catch(...) {
        for(auto spec:vectors) kinoko_native_buffer_destroy(result+spec.offset);
        throw;
    }
    return address(output.release());
}
extern "C" void kinoko_clear_map_layout(int32_t layout) {
    if(!layout) return;
    field<int32_t>(layout)=kinoko::legacy::address(kinoko_act_host_symbols()->map_layout_vtable);field<int32_t>(layout+4)=kinoko::legacy::address(kinoko_act_host_symbols()->map_view_vtable);
    for(auto it=vectors.rbegin();it!=vectors.rend();++it) {
        auto &view=field<VectorView>(layout+it->offset);
        // Both concrete sprite element destructors only reset IColor identity;
        // their texture handles are borrowed, so no texture retain/release.
        if(it->width==232 || it->width==288)
            for(int32_t p=view.begin;p!=view.end;p+=it->width) field<int32_t>(p)=kinoko::legacy::address(kinoko_act_host_symbols()->color_vtable);
        kinoko_native_buffer_destroy(layout+it->offset);
    }
    field<int32_t>(layout+4)=kinoko::legacy::address(kinoko_act_host_symbols()->color_vtable);
}
extern "C" int32_t __fastcall kinoko_delete_map_sprite(int32_t sprite,void*,int32_t flags) {
    const int32_t layout=sprite-4; // Original 43C1C0 adjusts the secondary this.
    if(flags&2) {
        const int32_t allocation=layout-4;
        const uint32_t count=field<uint32_t>(allocation);
        for(uint32_t i=count;i>0;--i) kinoko_clear_map_layout(layout+464*(i-1));
        if(flags&1) std::free(pointer<void>(allocation));
        return allocation;
    }
    kinoko_clear_map_layout(layout);
    if(flags&1) std::free(pointer<void>(layout));
    return layout;
}
