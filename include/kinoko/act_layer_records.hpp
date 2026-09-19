#pragma once
#include "kinoko/act_resource_records.hpp"

namespace kinoko::act {
// Verified prefixes, not allocation sizes. Unknown bytes stay opaque.
struct DocumentLayers {
    std::array<uint8_t, 208> unknown0;
    VectorStorage layers;
};
struct LayerKeys {
    std::array<uint8_t, 180> unknown0;
    Address key_head;
    int32_t key_count;
    std::array<uint8_t, 8> unknown188;
    int32_t extra_count;
};
struct KeyNode { Address next, previous, key; };
struct LayoutKey { uint32_t unknown0; Address layout; };
static_assert(offsetof(DocumentLayers, layers) == 208);
static_assert(offsetof(LayerKeys, key_head) == 180);
static_assert(offsetof(LayerKeys, key_count) == 184);
static_assert(offsetof(LayerKeys, extra_count) == 196);
static_assert(sizeof(KeyNode) == 12 && offsetof(KeyNode, key) == 8);
static_assert(offsetof(LayoutKey, layout) == 4);
}
