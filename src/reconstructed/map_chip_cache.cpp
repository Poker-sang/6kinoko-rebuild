#include "kinoko/map_chip_cache.hpp"
#include "kinoko/map_render.h"
#include "kinoko/act_runtime.h"
#include "kinoko/native_buffer.h"
#include "kinoko/texture_store.h"
#include <algorithm>
#include <cstring>
#include <climits>
#include <new>
#include <vector>
extern "C" unsigned char g25;
namespace kinoko::map {
namespace {
using kinoko::legacy::address;
using kinoko::native::RecordView;
using render::QuadRecord;
using render::QuadView;
template<class Span> uint32_t count(const Span &span) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(span.end)-
        reinterpret_cast<uintptr_t>(span.begin))/sizeof(*span.begin);
}
template<class Span> void replace(RecordView<Span> view,const void *data,size_t bytes) {
    if(bytes>INT32_MAX) throw std::bad_alloc();
    kinoko_native_buffer_replace(address(view.data()),data,static_cast<uint32_t>(bytes));
}
}
// 435B20 traverses the original ordered map, so cache indices are sorted by ID,
// not file order. Index zero is deliberately excluded by 435D00/435E80/435FD0.
void rebuild_chip_index(KinokoActLayout *layout) {
    auto *data=kinoko_map_cached_chip_data(layout);
    if(!data) return; // original callers ignore the failed rebuild result
    const LayoutView map(layout);
    std::vector<const retdec_mcd_chip *> chips;
    for(uint32_t i=0;i<data->chip_count;++i) chips.push_back(data->chips+i);
    std::sort(chips.begin(),chips.end(),[](auto a,auto b){return a->chip_id<b->chip_id;});
    auto maximum=map.get(&LayoutRecord::maximum_chip_id);
    if(maximum<0) for(auto *chip:chips) maximum=(std::max)(maximum,static_cast<int32_t>(chip->chip_id));
    // Retain the loader's existing malformed-ID allocation boundary.
    if(maximum>0x100000) throw std::bad_alloc();
    std::vector<int32_t> indices(static_cast<size_t>(maximum+1),-1);
    std::vector<ChipDefinition> definitions(chips.size());
    for(size_t i=0;i<chips.size();++i) {
        if(chips[i]->chip_id>=indices.size()) throw std::bad_alloc();
        indices[chips[i]->chip_id]=static_cast<int32_t>(i);
        definitions[i]=ChipView(const_cast<unsigned char*>(chips[i]->bytes)).load();
    }
    replace(map.view(&LayoutRecord::chip_definitions),definitions.data(),definitions.size()*sizeof(ChipDefinition));
    replace(map.view(&LayoutRecord::chip_indices),indices.data(),indices.size()*sizeof(int32_t));
    map.set(&LayoutRecord::maximum_chip_id,maximum);
    // Neither pending changes nor existing sprite entries are cleared by 435B20.
    // Their full 48-byte snapshots are checked before every reuse.
}
void prepare_placements(KinokoActLayout *layout) {
    auto *data=kinoko_map_cached_chip_data(layout);
    if(!data) return;
    const LayoutView map(layout);
    auto refs=map.get(&LayoutRecord::chip_references);refs.end=refs.begin;map.set(&LayoutRecord::chip_references,refs);
    auto textures=map.get(&LayoutRecord::texture_references);textures.end=textures.begin;map.set(&LayoutRecord::texture_references,textures);
    const auto placements=map.get(&LayoutRecord::placements);
    const auto size=count(placements);
    if(size) std::sort(placements.begin,placements.end,[](const auto &a,const auto &b){
        return a.left<b.left || (a.left==b.left && a.top<b.top);
    });
    rebuild_chip_index(layout);
    int32_t max_width=INT32_MIN,max_height=INT32_MIN;
    int32_t left=0,top=0,right=0,bottom=0;
    if(size) {
        left=placements.begin->left;top=placements.begin->top;
        right=placements.end[-1].left;bottom=right; // original 4359FF uses last X
    }
    for(uint32_t i=0;i<size;++i) {
        const auto &record=placements.begin[i];
        if(auto *chip=retdec_mcd_find_chip(data,record.chip_id)) {
            const auto definition=ChipView(chip->bytes).load();
            max_width=(std::max)(max_width,static_cast<int32_t>(definition.width));
            max_height=(std::max)(max_height,static_cast<int32_t>(definition.height));
            right=(std::max)(right,static_cast<int32_t>(static_cast<uint32_t>(record.left)+definition.width));
            bottom=(std::max)(bottom,static_cast<int32_t>(static_cast<uint32_t>(record.top)+definition.height));
        }
        left=(std::min)(left,record.left);top=(std::min)(top,record.top);
    }
    map.set(&LayoutRecord::max_chip_width,max_width);map.set(&LayoutRecord::max_chip_height,max_height);
    map.set(&LayoutRecord::chip_left,left);map.set(&LayoutRecord::chip_top,top);
    map.set(&LayoutRecord::chip_right,right);map.set(&LayoutRecord::chip_bottom,bottom);
}
// 404EE0 + the caller's base-to-current position copy. No texture ownership.
bool initialize_chip_quad(QuadRecord *storage,int32_t handle,const ChipDefinition *source) {
    if(!storage || !source || handle<0 || handle>=KINOKO_TEXTURE_CAPACITY) return false;
    const auto chip=ChipView(const_cast<ChipDefinition *>(source)).load();
    const QuadView quad(storage);
    quad.set(&QuadRecord::texture,handle);
    auto vertices=quad.get(&QuadRecord::vertices);
    if(handle) {
        const auto &texture=kinoko_texture_slots[handle];
        if(!texture.width || !texture.height) return false; // inherited malformed-texture guard
        const float width=static_cast<float>(texture.width),height=static_cast<float>(texture.height);
        quad.set(&QuadRecord::texture_width,width);quad.set(&QuadRecord::texture_height,height);
        const float du=chip.width/width,dv=chip.height/height;
        const float u=chip.source_left/width,v=chip.source_top/height;
        quad.set(&QuadRecord::source_u_extent,du);quad.set(&QuadRecord::source_v_extent,dv);
        vertices[0].u=u;vertices[0].v=v;
        vertices[1].u=u+du;vertices[1].v=v;
        vertices[2].u=u;vertices[2].v=v+dv;
        vertices[3].u=u+du;vertices[3].v=v+dv;
    } else {
        quad.set(&QuadRecord::texture_width,0.0f);quad.set(&QuadRecord::texture_height,0.0f);
        quad.set(&QuadRecord::source_u_extent,0.0f);quad.set(&QuadRecord::source_v_extent,0.0f);
    }
    for(auto &vertex:vertices) vertex.color=0xffffffffu;
    quad.set(&QuadRecord::vertices,vertices);
    const float width=static_cast<float>(chip.width),height=static_cast<float>(chip.height);
    const std::array<render::Position3,4> positions={{{-0.0f,-0.0f,0},{width,-0.0f,0},{-0.0f,height,0},{width,height,0}}};
    quad.set(&QuadRecord::base_positions,positions);quad.set(&QuadRecord::positions,positions);
    return true;
}
int32_t refresh_chip_sprite(KinokoActLayout *layout,const ChipDefinition *source) {
    auto *data=kinoko_map_cached_chip_data(layout);
    if(!data || !source) return E_FAIL;
    const auto chip=ChipView(const_cast<ChipDefinition *>(source)).load();
    const LayoutView map(layout);
    const auto indices=map.get(&LayoutRecord::chip_indices);
    const auto size=count(indices);
    if(chip.chip_id>=size) return E_FAIL;
    const int32_t index=indices.begin[chip.chip_id];
    if(index<=0 || static_cast<uint32_t>(index)>=data->chip_count) return E_FAIL;
    auto sprites=map.view(&LayoutRecord::chip_sprites);
    if(size>static_cast<uint32_t>(map.get(&LayoutRecord::chip_sprite_count))) {
        if(size>INT32_MAX/sizeof(ChipSpriteCache)) return E_FAIL;
        const auto previous=count(sprites.load());
        if(!kinoko_native_buffer_resize(address(sprites.data()),size*sizeof(ChipSpriteCache))) return E_FAIL;
        auto *begin=sprites.get(&ChipSpriteBuffer::begin);
        for(uint32_t i=previous;i<size;++i) {
            begin[i].quad.vtable=static_cast<uint32_t>(address(&g25));
            begin[i].definition={};begin[i].valid=0;
        }
        map.set(&LayoutRecord::chip_sprite_count,static_cast<int32_t>(count(sprites.load())));
    }
    auto &cached=sprites.get(&ChipSpriteBuffer::begin)[index];
    if(std::memcmp(&chip,&cached.definition,sizeof(chip))) cached.valid=0;
    if(cached.valid) return S_OK;
    cached.definition=chip;
    auto *texture=retdec_mcd_find_texture(data,chip.texture_id);
    if(!texture) return E_FAIL;
    if(!initialize_chip_quad(&cached.quad,texture->handle,&chip)) return E_FAIL;
    cached.valid=1;
    return S_OK;
}
const QuadRecord *find_chip_sprite(KinokoActLayout *layout,const ChipDefinition *source) {
    if(!layout || !source) return nullptr;
    const LayoutView map(layout);
    const auto id=ChipView(const_cast<ChipDefinition *>(source)).get(&ChipDefinition::chip_id);
    const auto indices=map.get(&LayoutRecord::chip_indices);
    if(id>=count(indices)) return nullptr;
    const auto index=indices.begin[id];
    if(FAILED(refresh_chip_sprite(layout,source)) || index<=0 || index>=map.get(&LayoutRecord::chip_sprite_count)) return nullptr;
    const auto &cached=map.get(&LayoutRecord::chip_sprites).begin[index];
    return cached.valid ? &cached.quad : nullptr;
}
void flush_changed_chips(KinokoActLayout *layout) {
    const LayoutView map(layout);
    auto pending=map.get(&LayoutRecord::changed_chips);
    for(uint32_t i=0;i<count(pending);++i) refresh_chip_sprite(layout,pending.begin[i]);
    pending.end=pending.begin;map.set(&LayoutRecord::changed_chips,pending);
}
bool set_chip_rectangle(KinokoActLayout *layout,int32_t id,int16_t x,int16_t y,int16_t width,int16_t height) {
    auto *data=kinoko_map_cached_chip_data(layout);
    auto *record=retdec_mcd_find_chip(data,static_cast<uint32_t>(id));
    if(!record) return false;
    const ChipView chip(record->bytes);
    chip.set(&ChipDefinition::source_left,x);chip.set(&ChipDefinition::source_top,y);
    chip.set(&ChipDefinition::width,width);chip.set(&ChipDefinition::height,height);
    const LayoutView map(layout);
    const auto definitions=map.get(&LayoutRecord::chip_definitions);
    // 43605F compares ID against DEFINITION COUNT (not maximum ID). Preserve
    // mutation-before-failure and the original index-zero exclusion.
    if(id<0 || static_cast<uint32_t>(id)>=count(definitions)) return false;
    const auto indices=map.get(&LayoutRecord::chip_indices);
    if(static_cast<uint32_t>(id)>=count(indices)) return false; // malformed-layout guard
    const auto index=indices.begin[id];
    if(index<=0 || static_cast<uint32_t>(index)>=data->chip_count) return false;
    definitions.begin[index]=chip.load();
    auto pending=map.view(&LayoutRecord::changed_chips);
    const auto size=count(pending.load());
    if(size>=INT32_MAX/sizeof(ChipDefinition *)) return false;
    if(!kinoko_native_buffer_resize(address(pending.data()),(size+1)*sizeof(ChipDefinition *))) return false;
    pending.get(&ChangedChipBuffer::begin)[size]=reinterpret_cast<const ChipDefinition *>(record->bytes);
    return true;
}
}
