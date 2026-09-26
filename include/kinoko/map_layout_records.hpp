#pragma once
#include "kinoko/act_types.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

struct kinoko_mcd_data;
struct kinoko_mcd_texture;

namespace kinoko::map {
// Schemas for existing Win32 storage, not objects constructed over its bytes.
// Unknown bytes remain opaque. Pointers here are borrowed unless noted.
struct Placement {
    uint32_t chip_id;
    int32_t left, top;
    float fractional_left, fractional_top;
    uint32_t unknown20;
    uint8_t visible;
    std::array<uint8_t, 3> padding25;
    float alpha;
};
struct ChipDefinition {
    uint32_t chip_id, texture_id;
    int16_t source_left, source_top, width, height;
    uint32_t flags;
    std::array<uint8_t, 14> unknown20;
    int16_t shape;
    std::array<uint8_t, 12> unknown36;
};
// native_buffer owns the flat storage; begin/end borrow from it. The third
// word is the reconstructed owner, NOT the original vector capacity pointer.
struct PlacementBuffer { Placement *begin, *end; void *storage_owner; };
struct QuadBuffer { render::QuadRecord *begin, *end; void *storage_owner; };
struct ChipSpriteCache {
    render::QuadRecord quad;
    ChipDefinition definition;
    uint8_t valid;
    std::array<uint8_t,7> padding281;
};
template<class T> struct MapRecordBuffer { T *begin, *end; void *storage_owner; };
using ChipSpriteBuffer=MapRecordBuffer<ChipSpriteCache>;
using ChipDefinitionBuffer=MapRecordBuffer<ChipDefinition>;
using ChangedChipBuffer=MapRecordBuffer<const ChipDefinition *>;
using ChipIndexBuffer=MapRecordBuffer<int32_t>;
using ChipReferenceBuffer=MapRecordBuffer<const ChipDefinition *>;
using TextureReferenceBuffer=MapRecordBuffer<kinoko_mcd_texture *>;
struct RenderLayerRecord { const void *methods; KinokoActLayout *layout; };
struct LayoutRecord {
    const unsigned char *methods;
    std::array<uint8_t, 232> sprite_and_base;
    uint32_t unknown236;
    int32_t max_chip_width, max_chip_height;
    int32_t chip_left, chip_top, chip_right, chip_bottom;
    PlacementBuffer placements;
    uint32_t unknown276;
    ChipReferenceBuffer chip_references;
    uint32_t unknown292;
    TextureReferenceBuffer texture_references;
    uint32_t unknown308;
    KinokoActLayer *owning_layer;
    KinokoActResource *cached_chip_resource;
    float alpha, scale;
    int32_t blend;
    QuadBuffer render_quads;
    uint32_t unknown344;
    std::array<uint8_t, 32> render_reference_buffers;
    int32_t render_count;
    ChipSpriteBuffer chip_sprites;
    uint32_t unknown396;
    int32_t chip_sprite_count;
    ChipDefinitionBuffer chip_definitions;
    uint32_t unknown416;
    ChangedChipBuffer changed_chips; // borrows definitions from shared MCD
    uint32_t unknown432;
    ChipIndexBuffer chip_indices;
    uint32_t unknown448;
    int32_t maximum_chip_id;
    int32_t render_scan_cache;
    uint8_t suppress_next_binding;
    std::array<uint8_t, 3> padding461;
};
struct LayerRecord {
    const unsigned char *methods;
    std::array<uint8_t, 84> prefix;
    KinokoActLayer *parent;
    std::array<uint8_t, 8> unknown92;
    KinokoActResource *resource; // document-owned; authoritative for creation
    std::array<uint8_t, 8> unknown104;
    kinoko::legacy::StringRecord name;
    uint32_t unknown136;
    uint8_t visible;
    std::array<uint8_t, 3> padding141;
    float position_x, position_y, position_z;
};
struct ChipResourceRecord {
    const unsigned char *methods;
    std::array<uint8_t, 60> prefix;
    kinoko_mcd_data *data; // shared MCD; retained by the resource's control
};
using LayoutView = kinoko::native::RecordView<LayoutRecord>;
using LayerView = kinoko::native::RecordView<LayerRecord>;
using PlacementView = kinoko::native::RecordView<Placement>;
using ChipView = kinoko::native::RecordView<ChipDefinition>;
using ChipResourceView = kinoko::native::RecordView<ChipResourceRecord>;

static_assert(sizeof(ChipSpriteCache)==288 && offsetof(ChipSpriteCache,definition)==232);
static_assert(offsetof(ChipSpriteCache,valid)==280);
static_assert(offsetof(LayoutRecord,chip_references)==280 && offsetof(LayoutRecord,texture_references)==296);
static_assert(offsetof(LayoutRecord,chip_sprites)==384 && offsetof(LayoutRecord,chip_sprite_count)==400);
static_assert(offsetof(LayoutRecord,chip_definitions)==404 && offsetof(LayoutRecord,changed_chips)==420);
static_assert(offsetof(LayoutRecord,chip_indices)==436 && offsetof(LayoutRecord,maximum_chip_id)==452);
static_assert(sizeof(Placement) == 32 && offsetof(Placement, alpha) == 28);
static_assert(offsetof(Placement, fractional_left) == 12 && offsetof(Placement, visible) == 24);
static_assert(sizeof(ChipDefinition) == 48 && offsetof(ChipDefinition, width) == 12);
static_assert(offsetof(ChipDefinition, height) == 14 && offsetof(ChipDefinition, flags) == 16);
static_assert(offsetof(ChipDefinition, shape) == 34);
static_assert(offsetof(LayoutRecord, max_chip_width) == 240);
static_assert(offsetof(LayoutRecord, placements) == 264);
static_assert(offsetof(LayoutRecord, owning_layer) == 312);
static_assert(offsetof(LayoutRecord, cached_chip_resource) == 316);
static_assert(offsetof(LayoutRecord, alpha) == 320 && offsetof(LayoutRecord, blend) == 328);
static_assert(offsetof(LayoutRecord, render_quads) == 332 && offsetof(LayoutRecord, render_count) == 380);
static_assert(offsetof(LayoutRecord, render_scan_cache) == 456);
static_assert(offsetof(LayoutRecord, suppress_next_binding) == 460 && sizeof(LayoutRecord) == 464);
static_assert(offsetof(LayerRecord, resource) == 100 && offsetof(LayerRecord, name) == 112);
static_assert(offsetof(LayerRecord, visible) == 140 && offsetof(LayerRecord, position_x) == 144);
static_assert(offsetof(LayerRecord, position_y) == 148);
static_assert(offsetof(ChipResourceRecord, data) == 64);

inline int32_t placement_count(KinokoActLayout *layout) {
    if (!layout) return 0;
    const auto span = LayoutView(layout).get(&LayoutRecord::placements);
    // Keep the x86 byte-distance contract, including borrowed C fixtures.
    const uint32_t bytes = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(span.end)) -
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(span.begin));
    return kinoko::legacy::load<int32_t>(&bytes) / static_cast<int32_t>(sizeof(Placement));
}
inline Placement *placement_at(KinokoActLayout *layout, int32_t index) {
    if (index < 0 || index >= placement_count(layout)) return nullptr;
    auto *begin = LayoutView(layout).get(&LayoutRecord::placements).begin;
    return reinterpret_cast<Placement *>(reinterpret_cast<unsigned char *>(begin) + index * sizeof(Placement));
}
}
