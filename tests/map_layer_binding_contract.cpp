// Exercise the real SetLayer entry and native buffers without game/VM startup.
#include "kinoko/act_resource_records_io.hpp"
#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/map_chip_cache.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/native_buffer.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
using namespace kinoko::map;
constexpr int32_t failure = static_cast<int32_t>(0x80004005u);
const int chip_methods = 1;
KinokoActHostSymbols host{};
int index_rebuilds = 0;
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
template<class T> size_t buffer_count(const MapRecordBuffer<T> &buffer) {
    const auto bytes = reinterpret_cast<uintptr_t>(buffer.end) - reinterpret_cast<uintptr_t>(buffer.begin);
    require(bytes % sizeof(T) == 0, "buffer ends at a complete native element");
    return bytes / sizeof(T);
}
template<class T> T buffer_value(const MapRecordBuffer<T> &buffer, size_t index) {
    require(index < buffer_count(buffer), "buffer contains expected element");
    return kinoko::legacy::load<T>(reinterpret_cast<const unsigned char *>(buffer.begin) + index * sizeof(T));
}
struct Fixture {
    LayoutRecord map{};
    LayerRecord layer{};
    kinoko::act::ChipResourceRecord resource{};
    std::array<kinoko_mcd_chip, 2> chips{};
    std::array<kinoko_mcd_texture, 2> textures{};
    kinoko_mcd_data data{};

    Fixture() {
        host.chip_resource_vtable = &chip_methods;
        resource.methods = &chip_methods;
        resource.data = &data;
        layer.resource = reinterpret_cast<KinokoActResource *>(&resource);
        data.chip_count = static_cast<uint32_t>(chips.size());
        data.chips = chips.data();
        data.texture_count = static_cast<uint32_t>(textures.size());
        data.textures = textures.data();
        for (uint32_t i = 0; i < chips.size(); ++i) {
            chips[i].chip_id = i + 10;
            textures[i].texture_id = i + 20;
            std::memcpy(chips[i].bytes + 4, &textures[i].texture_id, sizeof(uint32_t));
        }
    }
    ~Fixture() {
        kinoko_native_buffer_destroy(&map.placements);
        kinoko_native_buffer_destroy(&map.chip_references);
        kinoko_native_buffer_destroy(&map.texture_references);
    }
    int32_t bind() {
        return kinoko_method_map_set_layer(reinterpret_cast<KinokoActLayout *>(&map), nullptr,
            reinterpret_cast<KinokoActLayer *>(&layer));
    }
    void placements() {
        std::array<Placement, 3> records{};
        records[0].chip_id = 10; records[1].chip_id = 11; records[2].chip_id = 999;
        kinoko_native_buffer_replace(&map.placements, records.data(), sizeof(records));
    }
};
void complete_pointer_copies() {
    Fixture f;
    f.placements();
    require(f.bind() == 0, "bind valid MCD resources");
    require(f.map.owning_layer == reinterpret_cast<KinokoActLayer *>(&f.layer), "preserve full owner pointer");
    require(buffer_count(f.map.chip_references) == 3 && buffer_count(f.map.texture_references) == 3,
        "one native pointer per placement");
    for (size_t i = 0; i < 2; ++i) {
        require(buffer_value(f.map.chip_references, i) == reinterpret_cast<const ChipDefinition *>(f.chips[i].bytes),
            "chip pointer keeps every address bit");
        require(buffer_value(f.map.texture_references, i) == &f.textures[i], "texture pointer keeps every address bit");
    }
    require(!buffer_value(f.map.chip_references, 2) && !buffer_value(f.map.texture_references, 2),
        "missing chip retains null reference slots");
    require(f.bind() == 0, "repeated binding succeeds");
    require(buffer_count(f.map.chip_references) == 3 && buffer_count(f.map.texture_references) == 6,
        "original contract replaces chip references but appends texture references");
    for (size_t i = 0; i < 6; ++i) {
        auto *expected = i % 3 == 2 ? nullptr : &f.textures[i % 3];
        require(buffer_value(f.map.texture_references, i) == expected, "append preserves old and new pointers");
    }
}
void range_validation() {
    Fixture f;
    std::vector<kinoko_mcd_texture *> pointers(0x10000, &f.textures[0]);
    kinoko_native_buffer_replace(&f.map.texture_references, pointers.data(),
        static_cast<uint32_t>(pointers.size() * sizeof(pointers.front())));
    require(f.bind() == 0, "maximum existing texture count is measured in pointers");
    require(buffer_count(f.map.texture_references) == pointers.size() &&
        buffer_value(f.map.texture_references, pointers.size() - 1) == pointers.back(),
        "maximum pointer range is copied completely");
    pointers.push_back(&f.textures[1]);
    kinoko_native_buffer_replace(&f.map.texture_references, pointers.data(),
        static_cast<uint32_t>(pointers.size() * sizeof(pointers.front())));
    require(f.bind() == failure, "reject over-limit existing texture range");
    const auto valid = f.map.texture_references;
    f.map.texture_references.end = reinterpret_cast<kinoko_mcd_texture **>(
        reinterpret_cast<unsigned char *>(valid.begin) + sizeof(kinoko_mcd_texture *) / 2);
    require(f.bind() == failure, "reject a partial pointer without reading beyond it");
    f.map.texture_references = valid;
}
void binding_state() {
    Fixture f;
    f.placements();
    f.map.suppress_next_binding = 1;
    require(f.bind() == 0 && !f.map.suppress_next_binding && !f.map.cached_chip_resource,
        "one-shot suppression remains intact");
    require(!buffer_count(f.map.chip_references) && !buffer_count(f.map.texture_references),
        "suppressed binding publishes no references");
    require(f.bind() == 0 && f.map.cached_chip_resource == f.layer.resource,
        "next binding acquires the resource");
    f.layer.resource = nullptr;
    require(f.bind() == 0 && !f.map.cached_chip_resource && !buffer_count(f.map.chip_references),
        "missing resource clears chip references");
    require(buffer_count(f.map.texture_references) == 3, "missing resource retains previous texture references");
}
}

extern "C" const KinokoActHostSymbols *kinoko_act_host_symbols() { return &host; }
namespace kinoko::map {
void rebuild_chip_index(KinokoActLayout *) { ++index_rebuilds; }
}
extern "C" kinoko_mcd_chip *kinoko_mcd_find_chip(kinoko_mcd_data *data, uint32_t id) {
    if (data) for (uint32_t i = 0; i < data->chip_count; ++i)
        if (data->chips[i].chip_id == id) return &data->chips[i];
    return nullptr;
}
extern "C" kinoko_mcd_texture *kinoko_mcd_find_texture(kinoko_mcd_data *data, uint32_t id) {
    if (data) for (uint32_t i = 0; i < data->texture_count; ++i)
        if (data->textures[i].texture_id == id) return &data->textures[i];
    return nullptr;
}
int main() {
    try {
        complete_pointer_copies(); range_validation(); binding_state();
        require(index_rebuilds == 6, "preserve cache rebuild calls before reference validation");
        std::puts("map layer binding contract passed");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "map layer binding contract: %s\n", e.what());
        return 1;
    }
}
