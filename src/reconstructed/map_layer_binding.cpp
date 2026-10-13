#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/map_chip_cache.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/native_buffer.h"
#include <new>
#include <vector>

namespace {
uint32_t checked_placement_count(KinokoActLayout *layout) {
    const auto range = kinoko::map::LayoutView(layout).get(&kinoko::map::LayoutRecord::placements);
    const auto begin = reinterpret_cast<uintptr_t>(range.begin);
    const auto end = reinterpret_cast<uintptr_t>(range.end);
    constexpr auto element_bytes = sizeof(kinoko::map::Placement);
    if (end < begin || (end-begin)%element_bytes || (!begin && end) ||
        (end-begin)/element_bytes > 0x10000)
        throw std::bad_alloc();
    return static_cast<uint32_t>((end-begin)/element_bytes);
}
}

extern "C" int32_t __fastcall kinoko_method_map_set_layer(
    KinokoActLayout* layout, void*, KinokoActLayer* layer) {
    constexpr int32_t fail = static_cast<int32_t>(0x80004005u);
    if (!layout || !layer) return fail;
    try {
        using namespace kinoko::map;
        const LayoutView map(layout);
        auto *owner = layer;
        auto *resource = LayerView(owner).get(&LayerRecord::resource);
        map.set(&LayoutRecord::cached_chip_resource, static_cast<KinokoActResource *>(nullptr));
        if (!resource || map.get(&LayoutRecord::suppress_next_binding)) {
            // Original one-shot suppression is distinct from an invalid type.
            map.set(&LayoutRecord::suppress_next_binding, uint8_t{0});
        } else {
            if (ChipResourceView(resource).get(&ChipResourceRecord::methods) != kinoko_act_host_symbols()->chip_resource_vtable)
                return fail;
            map.set(&LayoutRecord::cached_chip_resource, resource);
            // Source MCD loading already loads each texture once. The original
            // sorted/unique texture preload therefore needs no second acquire.
            kinoko::map::rebuild_chip_index(layout);
        }
        map.set(&LayoutRecord::owning_layer, owner);
        auto chip_range = map.get(&LayoutRecord::chip_references);
        chip_range.end = chip_range.begin;
        map.set(&LayoutRecord::chip_references, chip_range);
        if (!map.get(&LayoutRecord::cached_chip_resource)) return 0;
        const auto count = checked_placement_count(layout);
        const auto data = ChipResourceView(resource).get(&ChipResourceRecord::data);
        if (!data && count) return fail;
        auto records = kinoko::map::LayoutView(layout).get(&kinoko::map::LayoutRecord::placements).begin;
        std::vector<const ChipDefinition*> chip_refs;
        std::vector<kinoko_mcd_texture*> texture_refs;
        // 434380 clears only the chip-reference vector; texture refs append.
        const auto textures = map.get(&LayoutRecord::texture_references);
        auto begin = reinterpret_cast<uintptr_t>(textures.begin), end = reinterpret_cast<uintptr_t>(textures.end);
        const auto texture_pointer_bytes = sizeof(*textures.begin);
        if (end < begin || (end-begin)%texture_pointer_bytes ||
            (end-begin)/texture_pointer_bytes > 0x10000 || (!begin && end)) return fail;
        if (end != begin) texture_refs.assign(textures.begin, textures.end);
        for (uint32_t i=0; i<count; ++i) {
            auto chip = kinoko_mcd_find_chip(data, static_cast<uint32_t>(records[i].chip_id));
            auto texture = chip ? kinoko_mcd_find_texture(data, kinoko::legacy::load<uint32_t>(chip->bytes+4)) : nullptr;
            chip_refs.push_back(chip ? reinterpret_cast<const ChipDefinition*>(chip->bytes) : nullptr);
            texture_refs.push_back(texture);
        }
        kinoko_native_buffer_replace(map.bytes(&LayoutRecord::chip_references), chip_refs.data(),
            static_cast<uint32_t>(chip_refs.size()*sizeof(chip_refs.front())));
        kinoko_native_buffer_replace(map.bytes(&LayoutRecord::texture_references), texture_refs.data(),
            static_cast<uint32_t>(texture_refs.size()*sizeof(texture_refs.front())));
        return 0;
    } catch (...) { return fail; }
}
