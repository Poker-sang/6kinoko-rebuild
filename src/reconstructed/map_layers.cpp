#include "kinoko/map_render.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/act_source.h"
#include "kinoko/map_containers.h"
#include "kinoko/act_layer_access.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {
extern unsigned char g327, g37;
}

namespace {
struct MapManager {
    unsigned char script_object[12];
    KinokoActDocument *source_act;
    KinokoActSourceHolder *source_holder;
    KinokoActRuntime *player;
};
using namespace kinoko::map;
static_assert(offsetof(MapManager, player) == 20);

}

extern "C" KinokoActLayout *kinoko_map_lookup_layout(KinokoMapManager *storage, const char *name) {
    const auto *manager = reinterpret_cast<const MapManager *>(storage);
    if (!manager || !manager->source_act || !name)
        return 0;

    // 46F140 uses the source holder only for the layer count. 452020 resolves
    // the key from player+16 (the live ACT holder), after BeginStage's clone.
    const int32_t count = kinoko_act_source_layer_count(manager->source_holder);
    for (int32_t index = 0; index < count; ++index) {
        auto *borrowed_layout = kinoko_act_layer_layout(manager->player, index);
        if (!borrowed_layout) continue;
        const LayoutView layout(borrowed_layout);
        auto *layer = layout.get(&LayoutRecord::owning_layer);
        if (layout.get(&LayoutRecord::methods) != &g327 || !layer) continue;
        const kinoko::legacy::StringView layer_name(LayerView(layer).bytes(&LayerRecord::name));
        if (std::strcmp(layer_name.data(), name) == 0)
            return borrowed_layout;
    }
    return 0;
}

extern "C" int32_t kinoko_map_find_layout(int32_t manager_address, const char *name) {
    return kinoko::legacy::address(kinoko_map_lookup_layout(
        kinoko::legacy::pointer<KinokoMapManager>(manager_address), name));
}

extern "C" int32_t kinoko_map_create_render_layer(int32_t manager_address,
                                                   const char *name) {
    return kinoko::legacy::address(kinoko_map_make_render_layer(
        kinoko::legacy::pointer<KinokoMapManager>(manager_address), name));
}
extern "C" KinokoRenderLayer *kinoko_map_make_render_layer(KinokoMapManager *manager,
                                                          const char *name) {
    auto *layout = kinoko_map_lookup_layout(manager, name);
    if (!layout)
        return 0;
    // std::list preserves the returned eight-byte object's address on append.
    return kinoko::legacy::pointer<KinokoRenderLayer>(kinoko_map_append_render(
        kinoko::legacy::address(manager), kinoko::legacy::address(layout)));
}

namespace {
using kinoko::legacy::load;
using QueryResource = uint8_t (__thiscall *)(KinokoActResource *, const void *, KinokoActResource **);
using SetMapLayer = int32_t (__thiscall *)(KinokoActLayout *, KinokoActLayer *);
struct ChipTypeDescriptor { void *table, *cache; char name[sizeof(".?AVCActResourceChip@@")]; };
const ChipTypeDescriptor chip_type{{}, {}, ".?AVCActResourceChip@@"};
struct ResourceMethods { void *prefix[2]; QueryResource query; };
struct LayoutMethods { void *prefix[6]; SetMapLayer set_layer; };
static_assert(offsetof(ResourceMethods, query) == 8);
static_assert(offsetof(LayoutMethods, set_layer) == 24);
}

extern "C" retdec_mcd_data *kinoko_map_layer_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    auto *layer = LayoutView(layout).get(&LayoutRecord::owning_layer);
    if (!layer) return nullptr;
    auto *resource = LayerView(layer).get(&LayerRecord::resource);
    if (!resource) return nullptr;
    // Original virtual QueryType; this does not read or populate the cache.
    const auto *methods = ChipResourceView(resource).get(&ChipResourceRecord::methods);
    auto query = load<QueryResource>(methods + offsetof(ResourceMethods, query));
    KinokoActResource *chip = nullptr;
    if (!query(resource, &chip_type, &chip) || !chip) return nullptr;
    return ChipResourceView(chip).get(&ChipResourceRecord::data);
}

extern "C" retdec_mcd_data *kinoko_map_cached_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    auto *resource = LayoutView(layout).get(&LayoutRecord::cached_chip_resource);
    return resource ? ChipResourceView(resource).get(&ChipResourceRecord::data) : nullptr;
}

extern "C" retdec_mcd_data *kinoko_map_query_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    const LayoutView map(layout);
    if (!map.get(&LayoutRecord::cached_chip_resource)) {
        auto bind = load<SetMapLayer>(map.get(&LayoutRecord::methods) + offsetof(LayoutMethods, set_layer));
        bind(layout, map.get(&LayoutRecord::owning_layer));
    }
    // Read again after virtual SetLayer. A non-null cache is not rebound.
    return kinoko_map_cached_chip_data(layout);
}
