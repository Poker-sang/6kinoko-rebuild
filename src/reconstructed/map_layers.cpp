#include "kinoko/map_render.h"
#include "kinoko/map_containers.h"
#include "kinoko/act_layer_access.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {
extern unsigned char g327, g37;
int32_t function_455890(int32_t holder);
}

namespace {
struct MapManager {
    unsigned char script_object[12];
    void *source_act;
    int32_t source_holder;
    int32_t player;
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
    const int32_t count = function_455890(manager->source_holder);
    for (int32_t index = 0; index < count; ++index) {
        const int32_t address = function_452020(manager->player, index);
        const auto *layout = reinterpret_cast<const LayoutView *>(address);
        if (!layout || layout->vtable != &g327 || !layout->layer)
            continue;
        const auto &layer_name = *reinterpret_cast<const LayerName *>(layout->layer + 112);
        if (std::strcmp(layer_name.data(), name) == 0)
            return address;
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
