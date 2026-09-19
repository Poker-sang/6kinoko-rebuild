#pragma once
#include "kinoko/act_resource_records.hpp"

namespace kinoko::act {
// Verified prefixes, not allocation sizes. Unknown bytes stay opaque.
struct DocumentLayers {
    Address vtable;
    int32_t resolution;
    std::array<uint8_t, 8> unknown8;
    std::array<uint8_t, 24> name;
    std::array<uint8_t, 48> unknown40;
    float x, y;
    uint8_t visible;
    std::array<uint8_t, 27> unknown97;
    // Borrowed representation; callback dispatch remains in the Squirrel bridge.
    std::array<int32_t, 5> update_callback;
    std::array<uint8_t, 64> unknown144;
    VectorStorage layers;
};
struct LayerKeys {
    std::array<uint8_t, 144> unknown0;
    std::array<uint32_t, 3> position;
    std::array<uint8_t, 12> unknown156;
    std::array<uint32_t, 3> previous_position;
    Address key_head;
    int32_t key_count;
    std::array<uint8_t, 8> unknown188;
    int32_t extra_count;
    std::array<uint8_t, 28> unknown200;
    std::array<int32_t, 5> update_callback;
};
struct KeyNode { Address next, previous, key; };
struct LayoutKey { uint32_t unknown0; Address layout; };
static_assert(offsetof(DocumentLayers, layers) == 208);
static_assert(offsetof(DocumentLayers, x) == 88 && offsetof(DocumentLayers, visible) == 96);
static_assert(offsetof(DocumentLayers, update_callback) == 124);
static_assert(offsetof(LayerKeys, position) == 144 && offsetof(LayerKeys, previous_position) == 168);
static_assert(offsetof(LayerKeys, update_callback) == 228);
static_assert(offsetof(LayerKeys, key_head) == 180);
static_assert(offsetof(LayerKeys, key_count) == 184);
static_assert(offsetof(LayerKeys, extra_count) == 196);
static_assert(sizeof(KeyNode) == 12 && offsetof(KeyNode, key) == 8);
static_assert(offsetof(LayoutKey, layout) == 4);
}
