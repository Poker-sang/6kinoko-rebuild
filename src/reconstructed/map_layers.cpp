#include "kinoko/map_render.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_memory.hpp"
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
struct LayerName {
    union { char local[16]; const char *heap; } storage;
    uint32_t size, capacity;

    const char *data() const { return capacity < 16 ? storage.local : storage.heap; }
};
struct LayoutView {
    void *vtable;
    unsigned char prefix[308];
    unsigned char *layer;
};
static_assert(offsetof(MapManager, player) == 20);
static_assert(sizeof(LayerName) == 24 && offsetof(LayoutView, layer) == 312);
}

extern "C" int32_t kinoko_map_find_layout(int32_t manager_address, const char *name) {
    const auto *manager = reinterpret_cast<const MapManager *>(manager_address);
    if (!manager || !manager->source_act || !name)
        return 0;

    // 46F140 uses the source holder only for the layer count. 452020 resolves
    // the key from player+16 (the live ACT holder), after BeginStage's clone.
    const int32_t count = kinoko_act_source_layer_count(manager->source_holder);
    for (int32_t index = 0; index < count; ++index) {
        auto *borrowed_layout = kinoko_act_layer_layout(manager->player, index);
        const auto *layout = reinterpret_cast<const LayoutView *>(borrowed_layout);
        if (!layout || layout->vtable != &g327 || !layout->layer)
            continue;
        const auto &layer_name = *reinterpret_cast<const LayerName *>(layout->layer + 112);
        if (std::strcmp(layer_name.data(), name) == 0)
            return static_cast<int32_t>(reinterpret_cast<uintptr_t>(borrowed_layout));
    }
    return 0;
}

extern "C" int32_t kinoko_map_create_render_layer(int32_t manager_address,
                                                   const char *name) {
    const int32_t layout = kinoko_map_find_layout(manager_address, name);
    if (!layout)
        return 0;
    // std::list preserves the returned eight-byte object's address on append.
    return kinoko_map_append_render(manager_address, layout);
}

namespace {
using kinoko::legacy::load;
struct ChipDataPrefix { unsigned char prefix[64]; retdec_mcd_data *data; };
struct LayerResourcePrefix { unsigned char prefix[100]; KinokoActResource *resource; };
struct MapResourcePrefix {
    const unsigned char *vtable;
    unsigned char prefix[308];
    KinokoActLayer *layer;
    KinokoActResource *cached_resource;
};
using QueryResource = uint8_t (__thiscall *)(KinokoActResource *, const void *, KinokoActResource **);
using SetMapLayer = int32_t (__thiscall *)(KinokoActLayout *, KinokoActLayer *);
struct ChipTypeDescriptor { void *table, *cache; char name[sizeof(".?AVCActResourceChip@@")]; };
const ChipTypeDescriptor chip_type{{}, {}, ".?AVCActResourceChip@@"};
static_assert(offsetof(MapResourcePrefix, layer) == 312);
static_assert(offsetof(MapResourcePrefix, cached_resource) == 316);
static_assert(offsetof(LayerResourcePrefix, resource) == 100);
static_assert(offsetof(ChipDataPrefix, data) == 64);
}

extern "C" retdec_mcd_data *kinoko_map_layer_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    const auto map = load<MapResourcePrefix>(layout);
    if (!map.layer) return nullptr;
    auto *resource = load<LayerResourcePrefix>(map.layer).resource;
    if (!resource) return nullptr;
    // The original uses the resource's virtual QueryType, independently of
    // the map layout's lazy +316 binding. The output is a borrowed pointer.
    const auto *methods = load<const unsigned char *>(resource);
    auto query = load<QueryResource>(methods + 8);
    KinokoActResource *chip = nullptr;
    if (!query(resource, &chip_type, &chip) || !chip) return nullptr;
    return load<ChipDataPrefix>(chip).data;
}

extern "C" retdec_mcd_data *kinoko_map_query_chip_data(KinokoActLayout *layout) {
    if (!layout) return nullptr;
    auto map = load<MapResourcePrefix>(layout);
    if (!map.cached_resource) {
        load<SetMapLayer>(map.vtable + 24)(layout, map.layer);
        map = load<MapResourcePrefix>(layout);
    }
    return map.cached_resource ? load<ChipDataPrefix>(map.cached_resource).data : nullptr;
}
