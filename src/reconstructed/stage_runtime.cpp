#include "kinoko/game_runtime.h"
#include "kinoko/act_document.h"
#include "kinoko/stage_runtime.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/act_source.h"
#include "kinoko/act_frame.h"
#include "kinoko/act_resource.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"

extern "C" {
extern struct SQVM *kinoko_primary_vm;
}

namespace {
using namespace kinoko::legacy;
using namespace kinoko::stage;
using kinoko::act::RuntimeRecord;
using kinoko::native::RecordView;
using RuntimeView = RecordView<RuntimeRecord>;

// The list's end sentinel is the list allocation itself (465F70/466050).
// Keep the original integer return on empty or skipped passes at this boundary.
int32_t stage_list_identity() { return address(kinoko_stage_list_end()); }

KinokoActRuntime *stage_runtime(const KinokoStageNode *node) {
    const auto owner = kinoko_stage_list_value(node);
    return owner ? OwnerView(owner).get(&OwnerRecord::runtime) : nullptr;
}

const char *document_name(KinokoActDocument *document) {
    return kinoko_act_document_name(document);
}
}

// 466050: increment the clock before executing each stage's frame update.
extern "C" int32_t kinoko_stages_update() {
    auto node = kinoko_stage_list_first();
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 8) {
        kinoko_trace("466050:entry");
        kinoko_trace_i32("466050:g603", stage_list_identity());
        kinoko_trace_i32("466050:first", address(node));
        kinoko_trace_i32("466050:update-mask", kinoko_game_masks.update);
    }
    if (node == kinoko_stage_list_end()) {
        if (trace_index <= 8) kinoko_trace("466050:empty");
        return stage_list_identity();
    }
    int32_t result = stage_list_identity();
    while (node != kinoko_stage_list_end()) {
        const auto resource = stage_runtime(node);
        if (resource) {
            const RuntimeView runtime(resource);
            if (trace_index <= 8) {
                auto *act = runtime.get(&RuntimeRecord::active_document);
                kinoko_trace_i32("466050:resource", address(resource));
                // Keep historical four-byte diagnostic snapshots (including
                // padding) without confusing them with one-byte game flags.
                kinoko_trace_i32("466050:active", load<int32_t>(runtime.bytes(&RuntimeRecord::stage_active)));
                kinoko_trace_i32("466050:suspend", load<int32_t>(runtime.bytes(&RuntimeRecord::hidden)));
                kinoko_trace_i32("466050:time", runtime.get(&RuntimeRecord::wake_time));
                kinoko_trace_i32("466050:act", address(act));
                if (act) kinoko_trace_squirrel_name("466050:act-name", address(document_name(act)));
            }
            kinoko_act_increment_frame(resource, nullptr);
            result = kinoko_act_update_frame(address(resource));
            if (trace_index <= 8) kinoko_trace_i32("466050:update-result", result);
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
        kinoko_trace_i32("render:g603", stage_list_identity());
        kinoko_trace_i32("render:g603-first", address(kinoko_stage_list_first()));
    }
    if (!stage_list_identity()) return 0;
    int32_t result = stage_list_identity();
    for (auto node = kinoko_stage_list_first(); node != kinoko_stage_list_end(); node = kinoko_stage_list_next(node)) {
        const auto resource = stage_runtime(node);
        result = resource ? kinoko_act_prepare_draw(address(resource)) : 0;
    }
    return result;
}

// 4660C0: preserve the origin (0,0), stage ordering and null-runtime handling.
extern "C" int32_t kinoko_stages_draw() {
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index == 1) {
        kinoko_trace_i32("render:g603-float", stage_list_identity());
        kinoko_trace_i32("render:g603-float-first", address(kinoko_stage_list_first()));
    }
    if (!stage_list_identity()) return 0;
    int32_t result = stage_list_identity(), index = 0;
    for (auto node = kinoko_stage_list_first(); node != kinoko_stage_list_end(); node = kinoko_stage_list_next(node), ++index) {
        const auto resource = stage_runtime(node);
        if (!resource) continue;
        const RuntimeView runtime(resource);
        if (trace_index <= 3) {
            auto *act = runtime.get(&RuntimeRecord::active_document);
            kinoko_trace_i32("4660c0:index", index);
            kinoko_trace_i32("4660c0:resource", address(resource));
            kinoko_trace_i32("4660c0:active", load<int32_t>(runtime.bytes(&RuntimeRecord::stage_active)));
            kinoko_trace_i32("4660c0:suspend", load<int32_t>(runtime.bytes(&RuntimeRecord::hidden)));
            kinoko_trace_i32("4660c0:act", address(act));
            if (act) kinoko_trace_squirrel_name("4660c0:act-name", address(document_name(act)));
        }
        result = kinoko_act_draw(address(resource), 0.0f, 0.0f);
    }
    return result;
}

extern "C" KinokoStageOwner *kinoko_stage_load(const char *file_name) {
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 8) {
        kinoko_trace("466100:entry");
        kinoko_trace_squirrel_name("466100:file", address(file_name));
    }
    // 466100 owns only constructor storage during unwind, not the whole
    // partially loaded stage. In particular it does not reject Load returning 0.
    auto *allocation = static_cast<KinokoStageOwner *>(std::malloc(sizeof(OwnerRecord)));
    if (!allocation) return nullptr;
    const OwnerView owner(allocation);
    owner.clear();
    auto *act = kinoko_act_document_create();
    owner.set(&OwnerRecord::document, act);
    kinoko_act_document_load(act, file_name);
    kinoko_act_document_load_resources(act, ""); // 46618A, result ignored

    KinokoActRuntime *resource = nullptr;
    auto *holder = static_cast<KinokoActSourceHolder *>(std::malloc(sizeof(SourceHolderRecord)));
    if (holder) {
        kinoko_act_source_initialize(holder, act);
        owner.set(&OwnerRecord::holder, holder);
        resource = kinoko_act_source_create_runtime(holder);
        owner.set(&OwnerRecord::runtime, resource);
    }
    kinoko_trace_i32("466100:act", address(act));
    kinoko_trace_i32("466100:holder", address(holder));
    kinoko_trace_i32("466100:resource", address(resource));

    if (resource && kinoko_primary_vm) {
        // 466208 has a known callee and receiver. Call the existing explicit
        // host implementation directly, instead of casting a function to void*.
        const auto result = kinoko_root_table_construct_this(address(resource), address(kinoko_primary_vm), 0);
        kinoko_trace_i32("466100:450e30-result", result);
    }
    if (stage_list_identity()) {
        const auto node = kinoko_stage_list_append(allocation);
        if (trace_index <= 8) kinoko_trace_i32("466100:list-node", address(node));
    }
    return allocation;
}
