#include "kinoko/stage_runtime.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/act_frame.h"
#include "kinoko/act_resource.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"

extern "C" {
extern int32_t g603, g459;
extern char *g644;
int32_t function_427530(int32_t document);
int32_t function_428000(int32_t document, const char *file_name);
int32_t function_455880(int32_t holder, int32_t document);
int32_t function_455e40(int32_t holder, int32_t output, int32_t flags);
}

namespace {
using namespace kinoko::legacy;
using namespace kinoko::stage;
using kinoko::act::RuntimeRecord;
using kinoko::act::DocumentLayers;
using kinoko::native::RecordView;
using RuntimeView = RecordView<RuntimeRecord>;

int32_t stage_runtime(int32_t node) {
    const auto owner = kinoko_stage_list_value(node);
    return owner ? static_cast<int32_t>(OwnerView(pointer(owner)).get(&OwnerRecord::runtime)) : 0;
}

int32_t document_name(uint32_t document) {
    const RecordView<DocumentLayers> view(pointer(document));
    return address(retdec_std_string_data(address(view.bytes(&DocumentLayers::name))));
}
}

// 466050: increment the clock before executing each stage's frame update.
extern "C" int32_t kinoko_stages_update() {
    auto node = kinoko_stage_list_first();
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 8) {
        retdec_trace("466050:entry");
        retdec_trace_i32("466050:g603", g603);
        retdec_trace_i32("466050:first", node);
        retdec_trace_i32("466050:update-mask", g459);
    }
    if (node == g603) {
        if (trace_index <= 8) retdec_trace("466050:empty");
        return g603;
    }
    int32_t result = g603;
    while (node != g603) {
        const auto resource = stage_runtime(node);
        if (resource) {
            const RuntimeView runtime(pointer(resource));
            if (trace_index <= 8) {
                const auto act = runtime.get(&RuntimeRecord::act);
                retdec_trace_i32("466050:resource", resource);
                // Keep historical four-byte diagnostic snapshots (including
                // padding) without confusing them with one-byte game flags.
                retdec_trace_i32("466050:active", load<int32_t>(runtime.bytes(&RuntimeRecord::stage_active)));
                retdec_trace_i32("466050:suspend", load<int32_t>(runtime.bytes(&RuntimeRecord::hidden)));
                retdec_trace_i32("466050:time", runtime.get(&RuntimeRecord::wake_time));
                retdec_trace_i32("466050:act", act);
                if (act) retdec_trace_squirrel_name("466050:act-name", document_name(act));
            }
            kinoko_act_increment_frame(resource, nullptr);
            result = kinoko_act_update_frame(resource);
            if (trace_index <= 8) retdec_trace_i32("466050:update-result", result);
        }
        // Advance after the callback, as in the original traversal.
        node = kinoko_stage_list_next(node);
    }
    return result;
}

// 466090: all stages prepare their draws before the separate drawing pass.
extern "C" int32_t kinoko_stages_prepare_draw() {
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index == 1) {
        retdec_trace_i32("render:g603", g603);
        retdec_trace_i32("render:g603-first", kinoko_stage_list_first());
    }
    if (!g603) return 0;
    int32_t result = g603;
    for (auto node = kinoko_stage_list_first(); node != g603; node = kinoko_stage_list_next(node)) {
        const auto resource = stage_runtime(node);
        result = resource ? kinoko_act_prepare_draw(resource) : 0;
    }
    return result;
}

// 4660C0: preserve the origin (0,0), stage ordering and null-runtime handling.
extern "C" int32_t kinoko_stages_draw() {
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index == 1) {
        retdec_trace_i32("render:g603-float", g603);
        retdec_trace_i32("render:g603-float-first", kinoko_stage_list_first());
    }
    if (!g603) return 0;
    int32_t result = g603, index = 0;
    for (auto node = kinoko_stage_list_first(); node != g603; node = kinoko_stage_list_next(node), ++index) {
        const auto resource = stage_runtime(node);
        if (!resource) continue;
        const RuntimeView runtime(pointer(resource));
        if (trace_index <= 3) {
            const auto act = runtime.get(&RuntimeRecord::act);
            retdec_trace_i32("4660c0:index", index);
            retdec_trace_i32("4660c0:resource", resource);
            retdec_trace_i32("4660c0:active", load<int32_t>(runtime.bytes(&RuntimeRecord::stage_active)));
            retdec_trace_i32("4660c0:suspend", load<int32_t>(runtime.bytes(&RuntimeRecord::hidden)));
            retdec_trace_i32("4660c0:act", act);
            if (act) retdec_trace_squirrel_name("4660c0:act-name", document_name(act));
        }
        result = kinoko_act_draw(resource, 0.0f, 0.0f);
    }
    return result;
}

extern "C" int32_t kinoko_stage_load(const char *file_name) {
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 8) {
        retdec_trace("466100:entry");
        retdec_trace_squirrel_name("466100:file", address(file_name));
    }
    // Own the unpublished record until it is transferred to the stage list
    // (or returned to the caller when the list is absent, as in R125).
    Allocation<unsigned char> allocation(pointer<unsigned char>(
        _3f__3f_2_40_YAPAXI_40_Z(sizeof(OwnerRecord))));
    if (!allocation) return 0;
    const OwnerView owner(allocation.get());
    owner.clear();

    // CAct remains a legacy allocation; this batch does not reconstruct its
    // full class or alter the pre-existing partial-load failure policy.
    constexpr int32_t document_allocation_size = 240; // 466146: push 0F0h
    auto act = _3f__3f_2_40_YAPAXI_40_Z(document_allocation_size);
    if (!act) return 0;
    act = function_427530(act);
    owner.set(&OwnerRecord::document, static_cast<uint32_t>(act));
    if (!act) return 0;
    if (!function_428000(act, file_name)) {
        retdec_trace("466100:act-header-failed");
        retdec_trace("466100:skip-invalid-act");
        return 0;
    }
    retdec_trace("466100:act-header-ok");

    int32_t resource = 0;
    const auto holder = _3f__3f_2_40_YAPAXI_40_Z(sizeof(SourceHolderRecord));
    if (holder) {
        function_455880(holder, act);
        owner.set(&OwnerRecord::holder, static_cast<uint32_t>(holder));
        function_455e40(holder, address(&resource), 0);
        owner.set(&OwnerRecord::runtime, static_cast<uint32_t>(resource));
    }
    retdec_trace_i32("466100:act", act);
    retdec_trace_i32("466100:holder", holder);
    retdec_trace_i32("466100:resource", resource);

    if (resource && g644) {
        // 466208 has a known callee and receiver. Call the existing explicit
        // host implementation directly, instead of casting a function to void*.
        const auto result = retdec_root_table_construct_this(resource, address(g644), 0);
        retdec_trace_i32("466100:450e30-result", result);
    }
    if (g603) {
        const auto node = kinoko_stage_list_append(address(allocation.get()));
        if (trace_index <= 8) retdec_trace_i32("466100:list-node", node);
    }
    return address(allocation.release());
}
