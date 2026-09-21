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
extern int32_t g603, g459;
extern char *g644;
}

namespace {
using namespace kinoko::legacy;
using namespace kinoko::stage;
using kinoko::act::RuntimeRecord;
using kinoko::native::RecordView;
using RuntimeView = RecordView<RuntimeRecord>;
struct DestroyStageOwner {
    void operator()(KinokoStageOwner *owner) const noexcept {
        kinoko_stage_owner_destroy(owner);
    }
};
using StageOwner = std::unique_ptr<KinokoStageOwner, DestroyStageOwner>;

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
        retdec_trace("466050:entry");
        retdec_trace_i32("466050:g603", g603);
        retdec_trace_i32("466050:first", address(node));
        retdec_trace_i32("466050:update-mask", g459);
    }
    if (node == kinoko_stage_list_end()) {
        if (trace_index <= 8) retdec_trace("466050:empty");
        return g603;
    }
    int32_t result = g603;
    while (node != kinoko_stage_list_end()) {
        const auto resource = stage_runtime(node);
        if (resource) {
            const RuntimeView runtime(resource);
            if (trace_index <= 8) {
                auto *act = pointer<KinokoActDocument>(runtime.get(&RuntimeRecord::act));
                retdec_trace_i32("466050:resource", address(resource));
                // Keep historical four-byte diagnostic snapshots (including
                // padding) without confusing them with one-byte game flags.
                retdec_trace_i32("466050:active", load<int32_t>(runtime.bytes(&RuntimeRecord::stage_active)));
                retdec_trace_i32("466050:suspend", load<int32_t>(runtime.bytes(&RuntimeRecord::hidden)));
                retdec_trace_i32("466050:time", runtime.get(&RuntimeRecord::wake_time));
                retdec_trace_i32("466050:act", address(act));
                if (act) retdec_trace_squirrel_name("466050:act-name", address(document_name(act)));
            }
            kinoko_act_increment_frame(address(resource), nullptr);
            result = kinoko_act_update_frame(address(resource));
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
        retdec_trace_i32("render:g603-first", address(kinoko_stage_list_first()));
    }
    if (!g603) return 0;
    int32_t result = g603;
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
        retdec_trace_i32("render:g603-float", g603);
        retdec_trace_i32("render:g603-float-first", address(kinoko_stage_list_first()));
    }
    if (!g603) return 0;
    int32_t result = g603, index = 0;
    for (auto node = kinoko_stage_list_first(); node != kinoko_stage_list_end(); node = kinoko_stage_list_next(node), ++index) {
        const auto resource = stage_runtime(node);
        if (!resource) continue;
        const RuntimeView runtime(resource);
        if (trace_index <= 3) {
            auto *act = pointer<KinokoActDocument>(runtime.get(&RuntimeRecord::act));
            retdec_trace_i32("4660c0:index", index);
            retdec_trace_i32("4660c0:resource", address(resource));
            retdec_trace_i32("4660c0:active", load<int32_t>(runtime.bytes(&RuntimeRecord::stage_active)));
            retdec_trace_i32("4660c0:suspend", load<int32_t>(runtime.bytes(&RuntimeRecord::hidden)));
            retdec_trace_i32("4660c0:act", address(act));
            if (act) retdec_trace_squirrel_name("4660c0:act-name", address(document_name(act)));
        }
        result = kinoko_act_draw(address(resource), 0.0f, 0.0f);
    }
    return result;
}

extern "C" KinokoStageOwner *kinoko_stage_load(const char *file_name) {
    static volatile LONG trace_count;
    const auto trace_index = InterlockedIncrement(&trace_count);
    if (trace_index <= 8) {
        retdec_trace("466100:entry");
        retdec_trace_squirrel_name("466100:file", address(file_name));
    }
    // Own the unpublished record until it is transferred to the stage list
    // (or returned to the caller when the list is absent, as in R125).
    StageOwner allocation(static_cast<KinokoStageOwner *>(std::malloc(sizeof(OwnerRecord))));
    if (!allocation) return nullptr;
    const OwnerView owner(allocation.get());
    owner.clear();

    auto *act = kinoko_act_document_create();
    owner.set(&OwnerRecord::document, act);
    if (!act) return nullptr;
    // Until publication this owner also owns the partial document. The scoped
    // reader in load closes first; failure/unwind then destroys the ACT using
    // the same virtual cleanup path as a published stage.
    if (!kinoko_act_document_load(act, file_name)) {
        retdec_trace("466100:act-header-failed");
        retdec_trace("466100:skip-invalid-act");
        return nullptr;
    }
    retdec_trace("466100:act-header-ok");

    KinokoActRuntime *resource = nullptr;
    auto *holder = static_cast<KinokoActSourceHolder *>(std::malloc(sizeof(SourceHolderRecord)));
    if (holder) {
        kinoko_act_source_initialize(holder, act);
        owner.set(&OwnerRecord::holder, holder);
        resource = kinoko_act_source_create_runtime(holder);
        owner.set(&OwnerRecord::runtime, resource);
    }
    retdec_trace_i32("466100:act", address(act));
    retdec_trace_i32("466100:holder", address(holder));
    retdec_trace_i32("466100:resource", address(resource));

    if (resource && g644) {
        // 466208 has a known callee and receiver. Call the existing explicit
        // host implementation directly, instead of casting a function to void*.
        const auto result = retdec_root_table_construct_this(address(resource), address(g644), 0);
        retdec_trace_i32("466100:450e30-result", result);
    }
    if (g603) {
        const auto node = kinoko_stage_list_append(allocation.get());
        if (trace_index <= 8) retdec_trace_i32("466100:list-node", address(node));
    }
    return allocation.release();
}
