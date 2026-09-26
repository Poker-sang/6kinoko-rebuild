#include "kinoko/act_layer_access.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/stage_records.hpp"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"

// The remaining allocator ABI still returns an integer address. Convert here,
// not throughout the layer/document APIs. It currently forwards to malloc.
extern "C" int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);
extern "C" void kinoko_trace_i32(const char* label, int32_t value);

namespace {
using namespace kinoko::act;
using namespace kinoko::legacy;
using kinoko::native::RecordView;
KinokoActDocument *source_document(const KinokoActSourceHolder *holder) {
    return holder ? load<KinokoActSourceHolder>(holder).document : nullptr;
}
KinokoActLayer *layer_value(const KinokoActLayerHolder *holder) {
    return holder ? load<KinokoActLayerHolder>(holder).layer : nullptr;
}
bool has_layer(KinokoActDocument *document, int32_t index) {
    if (!document || index < 0) return false;
    const auto layers = DocumentView(document).get(&DocumentRecord::layers);
    return ordered_layers(layers) && index < layer_distance(layers);
}
template<class Holder> void make_holder(Holder **output, const Holder& borrowed) {
    auto *holder = pointer<Holder>(_3f__3f_2_40_YAPAXI_40_Z(sizeof(Holder)));
    if (holder) {
        store(holder, borrowed);
        store(output, holder);
    }
}
// Free the wrapper only, retaining the same trace boundaries and order.
template<class Holder>
void release_holder(Allocation<Holder>& holder, const char* before, const char* after) {
    kinoko_trace(before);
    holder.reset();
    kinoko_trace(after);
}
}

extern "C" KinokoActLayerHolder **kinoko_act_layer_holder(
    const KinokoActSourceHolder *holder, int32_t index, KinokoActLayerHolder **output) {
    if (!output) return nullptr;
    store(output, static_cast<KinokoActLayerHolder *>(nullptr));
    if (!holder || index < 0) return output;
    auto *document = source_document(holder);
    if (!has_layer(document, index)) return output;
    const auto layers = DocumentView(document).get(&DocumentRecord::layers);
    make_holder(output, KinokoActLayerHolder{layer_at(layers, index)});
    return output;
}

extern "C" KinokoActKeyHolder **kinoko_act_key_holder(
    const KinokoActLayerHolder *holder, int32_t index, KinokoActKeyHolder **output) {
    if (!output) return nullptr;
    store(output, static_cast<KinokoActKeyHolder *>(nullptr));
    if (!holder || index < 0) return output;
    auto *layer = layer_value(holder);
    if (!layer) return output;
    const RecordView<LayerKeys> keys(layer);
    if (index >= keys.get(&LayerKeys::key_count)) return output;
    const auto head = keys.get(&LayerKeys::key_head);
    if (!head) return output;
    auto *node = RecordView<KeyNode>(head).get(&KeyNode::next);
    if (!node) return output;
    while (index-- > 0) {
        node = RecordView<KeyNode>(node).get(&KeyNode::next);
        if (!node) return output;
    }
    make_holder(output, KinokoActKeyHolder{RecordView<KeyNode>(node).get(&KeyNode::key)});
    return output;
}

extern "C" KinokoActKey *kinoko_act_first_key(KinokoActRuntime *resource, int32_t index) {
    kinoko_trace_i32("452040:resource", address(resource));
    kinoko_trace_i32("452040:index", index);
    if (!resource || index < 0 || !RecordView<RuntimeRecord>(resource).get(&RuntimeRecord::stage_active)) return nullptr;
    auto *holder = RecordView<RuntimeRecord>(resource).get(&RuntimeRecord::active_holder);
    if (!has_layer(source_document(holder), index)) return nullptr;
    KinokoActLayerHolder *layer_result = nullptr;
    kinoko_act_layer_holder(holder, index, &layer_result);
    Allocation<KinokoActLayerHolder> layer_holder(layer_result);
    kinoko_trace_i32("452040:item-holder", address(layer_result));
    auto *layer = layer_value(layer_result);
    kinoko_trace_i32("452040:item", address(layer));
    if (!layer) {
        release_holder(layer_holder, "452040:free-item-holder-before", "452040:free-item-holder-after");
        return nullptr;
    }
    const RecordView<LayerKeys> keys(layer);
    const auto extra_count = keys.get(&LayerKeys::extra_count);
    const auto key_count = keys.get(&LayerKeys::key_count);
    kinoko_trace_i32("452040:item-extra-count", extra_count);
    kinoko_trace_i32("452040:item-key-count", key_count);
    if (extra_count != 0 || key_count == 0) {
        release_holder(layer_holder, "452040:free-item-invalid-before", "452040:free-item-invalid-after");
        return nullptr;
    }
    KinokoActKeyHolder *key_result = nullptr;
    kinoko_act_key_holder(layer_holder.get(), 0, &key_result);
    Allocation<KinokoActKeyHolder> key_holder(key_result);
    kinoko_trace_i32("452040:value-holder", address(key_result));
    auto *key = key_result ? load<KinokoActKeyHolder>(key_result).key : nullptr;
    kinoko_trace_i32("452040:value", address(key));
    if (key_holder) release_holder(key_holder, "452040:free-value-holder-before", "452040:free-value-holder-after");
    if (!key) {
        release_holder(layer_holder, "452040:free-item-holder-empty-before", "452040:free-item-holder-empty-after");
        return nullptr;
    }
    kinoko_trace_i32("452040:result", address(key));
    release_holder(layer_holder, "452040:free-item-holder-before", "452040:free-item-holder-after");
    return key;
}

extern "C" KinokoActLayout *kinoko_act_layer_layout(KinokoActRuntime *resource, int32_t index) {
    auto *key = kinoko_act_first_key(resource, index);
    return key ? RecordView<LayoutKey>(key).get(&LayoutKey::layout) : nullptr;
}
