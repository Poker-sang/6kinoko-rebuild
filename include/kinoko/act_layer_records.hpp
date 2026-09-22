#pragma once
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_document_records.hpp"
#include "kinoko/legacy_memory.hpp"

struct KinokoActLayerHolder { KinokoActLayer *layer; };
struct KinokoActKeyHolder { KinokoActKey *key; };
static_assert(sizeof(KinokoActLayerHolder) == sizeof(void *));
static_assert(sizeof(KinokoActKeyHolder) == sizeof(void *));

namespace kinoko::act {
// Verified prefixes, not allocation sizes. Unknown bytes stay opaque.
// One-word wrappers own only their allocations, never the pointees.
struct LayerKeys {
    std::array<uint8_t, 144> unknown0;
    std::array<uint32_t, 3> position;
    std::array<uint8_t, 12> unknown156;
    std::array<uint32_t, 3> previous_position;
    struct KeyNode *key_head;
    int32_t key_count;
    std::array<uint8_t, 8> unknown188;
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
static_assert(offsetof(LayerKeys, position) == 144 && offsetof(LayerKeys, previous_position) == 168);
static_assert(offsetof(LayerKeys, update_callback) == 228);
static_assert(offsetof(LayerKeys, key_head) == 180);
static_assert(offsetof(LayerKeys, key_count) == 184);
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
// The upstream-compatible container still stores integer slots. Read a pointer
// representation without treating that allocation as a C++ array of pointers.
inline KinokoActLayer *layer_at(const DocumentPointerSpan<KinokoActLayer>& layers, int32_t index) noexcept {
    const auto *slots = reinterpret_cast<const unsigned char *>(layers.begin);
    return kinoko::legacy::load<KinokoActLayer *>(slots + static_cast<size_t>(index) * sizeof(KinokoActLayer *));
}
inline bool ordered_layers(const DocumentPointerSpan<KinokoActLayer>& layers) noexcept {
    return layers.begin && kinoko::legacy::address(layers.end) >= kinoko::legacy::address(layers.begin);
}
}
