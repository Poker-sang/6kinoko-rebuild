#pragma once
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_document_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <limits>

struct KinokoActLayerHolder { KinokoActLayer *layer; };
struct KinokoActKeyHolder { KinokoActKey *key; };
static_assert(sizeof(KinokoActLayerHolder) == sizeof(void *));
static_assert(sizeof(KinokoActKeyHolder) == sizeof(void *));

namespace kinoko::act {
// Verified prefixes, not allocation sizes. Unknown bytes stay opaque.
// One-word wrappers own only their allocations, never the pointees.
struct LayerKeys {
    std::array<uint8_t, 112> unknown0;
    legacy::StringRecord name;
    std::array<uint8_t, 8> unknown136;
    std::array<uint32_t, 3> position;
    std::array<uint8_t, 12> origin_bits;
    std::array<uint32_t, 3> previous_position;
    struct KeyNode *key_head;
    int32_t key_count;
    std::array<uint8_t, 4> unknown188;
    struct KeyNode *timeline_head;
    int32_t extra_count;
    std::array<uint8_t, 28> unknown200;
    std::array<int32_t, 5> update_callback;
};
struct KeyNode { KeyNode *next, *previous; KinokoActKey *key; };
struct LayoutKey { uint32_t unknown0; KinokoActLayout *layout; };
// Prefix within the document's embedded script, not a duplicate document schema.
struct ScriptUpdatePrefix {
    std::array<uint8_t, 24> before_update;
    std::array<int32_t, 5> update_callback;
};
static_assert(offsetof(ScriptUpdatePrefix, update_callback) == 24);
static_assert(offsetof(LayerKeys, name) == 112);
static_assert(offsetof(LayerKeys, position) == 144 && offsetof(LayerKeys, previous_position) == 168);
static_assert(offsetof(LayerKeys, update_callback) == 228);
static_assert(offsetof(LayerKeys, key_head) == 180);
static_assert(offsetof(LayerKeys, key_count) == 184);
static_assert(offsetof(LayerKeys, timeline_head) == 192);
static_assert(offsetof(LayerKeys, extra_count) == 196);
static_assert(sizeof(KeyNode) == 12 && offsetof(KeyNode, key) == 8);
static_assert(offsetof(LayoutKey, layout) == 4);
// Keep the original x86 DWORD-distance interpretation, including malformed
// raw fixture spans, without C++ subtraction of unrelated/null pointers.
inline int32_t layer_distance(const DocumentPointerSpan<KinokoActLayer>& layers) noexcept {
    const uint32_t bytes = static_cast<uint32_t>(
        reinterpret_cast<uintptr_t>(layers.end) - reinterpret_cast<uintptr_t>(layers.begin));
    return kinoko::legacy::load<int32_t>(&bytes) / static_cast<int32_t>(sizeof(KinokoActLayer *));
}
// The shared container stores void pointers. Read a borrowed typed pointer
// representation without aliasing that allocation as an array of another type.
inline KinokoActLayer *layer_at(const DocumentPointerSpan<KinokoActLayer>& layers, int32_t index) noexcept {
    const auto *slots = reinterpret_cast<const unsigned char *>(layers.begin);
    return kinoko::legacy::load<KinokoActLayer *>(slots + static_cast<size_t>(index) * sizeof(KinokoActLayer *));
}
// Native pointer spans are not the four-byte indices stored in ACT files.
// Validate their shape before a script can scan or reorder the layer slots.
inline bool checked_layer_count(const DocumentPointerSpan<KinokoActLayer>& layers,
                                int32_t& count) noexcept {
    count = 0;
    const auto begin = reinterpret_cast<uintptr_t>(layers.begin);
    const auto end = reinterpret_cast<uintptr_t>(layers.end);
    if (end < begin || (!begin && end)) return false;
    const auto bytes = end - begin;
    const auto size = bytes / sizeof(KinokoActLayer*);
    if (bytes % sizeof(KinokoActLayer*) ||
        size > static_cast<uintptr_t>(std::numeric_limits<int32_t>::max())) return false;
    count = static_cast<int32_t>(size);
    return true;
}
inline int32_t find_layer_order(const DocumentPointerSpan<KinokoActLayer>& layers,
                                KinokoActLayer* layer) noexcept {
    int32_t count = 0;
    if (!checked_layer_count(layers, count)) return -1;
    for (int32_t index = 0; index < count; ++index)
        if (layer_at(layers, index) == layer) return index;
    return -1;
}
inline bool ordered_layers(const DocumentPointerSpan<KinokoActLayer>& layers) noexcept {
    return layers.begin && kinoko::legacy::address(layers.end) >= kinoko::legacy::address(layers.begin);
}
}
