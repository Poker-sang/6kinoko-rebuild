#include "kinoko/act_frame.h"
#include "kinoko/act_layer_access.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/stage_records.hpp"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/windows_owner.hpp"
#include <mmsystem.h>

extern "C" {
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_name(const char*, int32_t);
const char* retdec_std_string_data(int32_t);
int32_t function_415810_this(int32_t);
}

namespace {
using namespace kinoko::act;
using namespace kinoko::legacy;
using kinoko::native::RecordView;
using RuntimeView = RecordView<RuntimeRecord>;
KinokoActDocument *source_document(const RuntimeView& resource) {
    const auto holder = resource.get(&RuntimeRecord::source_holder);
    return holder ? load<KinokoActSourceHolder>(holder).document : nullptr;
}
int32_t layer_count(KinokoActDocument *document) {
    if (!document) return 0;
    const auto layers = DocumentView(document).get(&DocumentRecord::layers);
    return layer_distance(layers);
}
}

extern "C" int32_t kinoko_act_layer_update(KinokoActLayer *object) {
    if (!object) return -1;
    const RecordView<LayerKeys> layer(object);
    static volatile LONG trace_count;
    if (InterlockedIncrement(&trace_count) <= 160) {
        const auto callback = layer.get(&LayerKeys::update_callback);
        retdec_trace_i32("41efb0:layer", address(object));
        const char* labels[] = {"41efb0:callback-vm", "41efb0:callback-env-type",
            "41efb0:callback-env-data", "41efb0:callback-type", "41efb0:callback-data"};
        for (int i = 0; i < 5; ++i) retdec_trace_i32(labels[i], callback[i]);
    }
    layer.set(&LayerKeys::previous_position, layer.get(&LayerKeys::position));
    return layer.get(&LayerKeys::update_callback)[3] != 0x1000001
        ? function_415810_this(address(layer.bytes(&LayerKeys::update_callback))) : 0;
}

// Original 451640: the root callback belongs to the source ACT, while layer
// holders come from the active runtime. Callbacks can change either container.
extern "C" int32_t kinoko_act_update_frame(int32_t self) {
    const RuntimeView resource(pointer(self));
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 48) {
        const auto act = self ? resource.get(&RuntimeRecord::active_document) : 0;
        retdec_trace_i32("451640:resource", self);
        retdec_trace_i32("451640:active", self ? load<int32_t>(resource.bytes(&RuntimeRecord::stage_active)) : 0);
        retdec_trace_i32("451640:suspend", self ? load<int32_t>(resource.bytes(&RuntimeRecord::hidden)) : 0);
        retdec_trace_i32("451640:time", self ? resource.get(&RuntimeRecord::wake_time) : 0);
        retdec_trace_i32("451640:current", self ? resource.get(&RuntimeRecord::current_time) : 0);
        retdec_trace_i32("451640:act", address(act));
        if (act) retdec_trace_squirrel_name("451640:act-name", address(retdec_std_string_data(
            address(DocumentView(act).bytes(&DocumentRecord::name)))));
    }
    if (!self || resource.get(&RuntimeRecord::hidden)) {
        if (trace_index <= 48) retdec_trace("451640:skip-suspended");
        return 0;
    }
    kinoko::windows::CriticalLock lock(reinterpret_cast<CRITICAL_SECTION*>(resource.bytes(&RuntimeRecord::lock)));
    if (!resource.get(&RuntimeRecord::stage_active) || !resource.get(&RuntimeRecord::active_holder)) return E_FAIL;
    kinoko_act_commands_clear(self);
    // 4516C4 is JNB: compare DWORDs, including uptime above 0x80000000.
    if (resource.get(&RuntimeRecord::wake_time) >= timeGetTime()) {
        if (trace_index <= 48) retdec_trace("451640:skip-time");
        return 0;
    }
    const auto document = source_document(resource);
    if (!document) {
        if (trace_index <= 48) retdec_trace("451640:skip-no-act-object");
        return 0;
    }
    const auto script = DocumentView(document).view(&DocumentRecord::script);
    const RecordView<ScriptUpdatePrefix> source(script.data());
    const auto callback_type = source.get(&ScriptUpdatePrefix::update_callback)[3];
    if (trace_index <= 48) retdec_trace_i32("451640:root-update-type", callback_type);
    if (callback_type != 0x1000001) {
        const auto result = function_415810_this(address(source.bytes(&ScriptUpdatePrefix::update_callback)));
        if (trace_index <= 48) retdec_trace_i32("451640:root-update-result", result);
    }
    const auto count = layer_count(source_document(resource));
    if (trace_index <= 48) retdec_trace_i32("451640:layer-count", count < 0 ? -1 : count);
    if (count <= 0) return 0;
    for (int32_t index = 0;;) {
        if (resource.get(&RuntimeRecord::stage_active) && resource.get(&RuntimeRecord::active_holder)) {
            KinokoActLayerHolder *temporary = nullptr;
            kinoko_act_layer_holder(resource.get(&RuntimeRecord::active_holder), index, &temporary);
            Allocation<KinokoActLayerHolder> holder(temporary);
            if (!holder) return 0;
            if (kinoko_act_layer_update(load<KinokoActLayerHolder>(holder.get()).layer) < 0) return E_FAIL;
        }
        // 451746 reloads **this and its end after every callback, including
        // source replacement. Do not cache the initial number of layers.
        if (++index >= layer_count(source_document(resource))) break;
    }
    return 0;
}
