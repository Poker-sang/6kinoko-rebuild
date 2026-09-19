#include "kinoko/act_layer_access.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"

extern "C" int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);
extern "C" void retdec_trace_i32(const char* label, int32_t value);

namespace {
using namespace kinoko::act;
using namespace kinoko::legacy;
using kinoko::native::RecordView;
template<class T> RecordView<T> view(int32_t value) { return RecordView<T>(pointer(value)); }
int32_t pointee(int32_t holder) { return holder ? load<int32_t>(pointer(holder)) : 0; }
bool has_layer(int32_t document, int32_t index) {
    if (!document || index < 0) return false;
    const auto layers = view<DocumentLayers>(document).get(&DocumentLayers::layers);
    const auto begin = static_cast<int32_t>(layers.begin);
    const auto end = static_cast<int32_t>(layers.end);
    return begin && end >= begin && index < (end - begin) / 4;
}
void make_holder(int32_t output, int32_t borrowed) {
    const auto holder = _3f__3f_2_40_YAPAXI_40_Z(sizeof(Address));
    if (holder) {
        store(pointer(holder), borrowed);
        store(pointer(output), holder);
    }
}
// Free the wrapper only, retaining the existing trace boundaries and order.
void release_holder(Allocation<int32_t>& holder, const char* before, const char* after) {
    retdec_trace(before);
    holder.reset();
    retdec_trace(after);
}
}

extern "C" int32_t kinoko_act_layer_holder(int32_t holder, int32_t index, int32_t output) {
    if (!output) return 0;
    store(pointer(output), int32_t{0});
    if (!holder || index < 0) return output;
    const auto document = pointee(holder);
    if (!has_layer(document, index)) return output;
    const auto layers = view<DocumentLayers>(document).get(&DocumentLayers::layers);
    make_holder(output, load<int32_t>(pointer<unsigned char>(layers.begin) + index * sizeof(Address)));
    return output;
}

extern "C" int32_t kinoko_act_key_holder(int32_t holder, int32_t index, int32_t output) {
    if (!output) return 0;
    store(pointer(output), int32_t{0});
    if (!holder || index < 0) return output;
    const auto layer = pointee(holder);
    if (!layer) return output;
    const auto keys = view<LayerKeys>(layer);
    if (index >= keys.get(&LayerKeys::key_count)) return output;
    const auto head = keys.get(&LayerKeys::key_head);
    if (!head) return output;
    auto node = view<KeyNode>(head).get(&KeyNode::next);
    if (!node) return output;
    while (index-- > 0) {
        node = view<KeyNode>(node).get(&KeyNode::next);
        if (!node) return output;
    }
    make_holder(output, view<KeyNode>(node).get(&KeyNode::key));
    return output;
}

extern "C" int32_t function_452040(int32_t resource, int32_t index) {
    retdec_trace_i32("452040:resource", resource);
    retdec_trace_i32("452040:index", index);
    if (!resource || index < 0 || !view<RuntimeRecord>(resource).get(&RuntimeRecord::stage_active)) return 0;
    const auto holder = view<RuntimeRecord>(resource).get(&RuntimeRecord::owned_storage);
    if (!has_layer(pointee(holder), index)) return 0;
    int32_t temporary = 0;
    kinoko_act_layer_holder(holder, index, address(&temporary));
    Allocation<int32_t> layer_holder(pointer<int32_t>(temporary));
    retdec_trace_i32("452040:item-holder", temporary);
    const auto layer = pointee(temporary);
    retdec_trace_i32("452040:item", layer);
    if (!layer) {
        release_holder(layer_holder, "452040:free-item-holder-before", "452040:free-item-holder-after");
        return 0;
    }
    const auto keys = view<LayerKeys>(layer);
    const auto extra_count = keys.get(&LayerKeys::extra_count);
    const auto key_count = keys.get(&LayerKeys::key_count);
    retdec_trace_i32("452040:item-extra-count", extra_count);
    retdec_trace_i32("452040:item-key-count", key_count);
    if (extra_count != 0 || key_count == 0) {
        release_holder(layer_holder, "452040:free-item-invalid-before", "452040:free-item-invalid-after");
        return 0;
    }
    kinoko_act_key_holder(address(layer_holder.get()), 0, address(&temporary));
    Allocation<int32_t> key_holder(pointer<int32_t>(temporary));
    retdec_trace_i32("452040:value-holder", temporary);
    const auto key = pointee(temporary);
    retdec_trace_i32("452040:value", key);
    if (key_holder) release_holder(key_holder, "452040:free-value-holder-before", "452040:free-value-holder-after");
    if (!key) {
        release_holder(layer_holder, "452040:free-item-holder-empty-before", "452040:free-item-holder-empty-after");
        return 0;
    }
    retdec_trace_i32("452040:result", key);
    release_holder(layer_holder, "452040:free-item-holder-before", "452040:free-item-holder-after");
    return key;
}

extern "C" int32_t function_452020(int32_t resource, int32_t index) {
    const auto key = function_452040(resource, index);
    return key ? view<LayoutKey>(key).get(&LayoutKey::layout) : 0;
}
