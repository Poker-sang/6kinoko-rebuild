#include "kinoko/act_source.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_resource.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>

namespace {
using kinoko::stage::SourceHolderRecord;
using kinoko::native::RecordView;
using kinoko::act::DocumentLayers;
using kinoko::act::RuntimeRecord;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
}

// 455880: explicit receiver, one borrowed document pointer.
extern "C" KinokoActSourceHolder *kinoko_act_source_initialize(
    KinokoActSourceHolder *holder, KinokoActDocument *document) {
    if (holder) RecordView<SourceHolderRecord>(holder).set(&SourceHolderRecord::document, document);
    return holder;
}

// 455890: count source ACT layers, not the cloned live runtime's layers.
extern "C" int32_t kinoko_act_source_layer_count(const KinokoActSourceHolder *holder) {
    if (!holder) return 0;
    // memcpy reads tolerate legacy records that were not C++-constructed.
    const auto document = kinoko::legacy::load<SourceHolderRecord>(holder).document;
    if (!document) return 0;
    const auto layers = RecordView<DocumentLayers>(document).get(&DocumentLayers::layers);
    // The R126 guards and signed Win32 arithmetic are retained until the
    // broader vector record itself has a typed representation.
    const auto begin = static_cast<int32_t>(layers.begin);
    const auto end = static_cast<int32_t>(layers.end);
    if (!begin || end < begin) return 0;
    return (end - begin) / static_cast<int32_t>(sizeof(void *));
}

// 455E40: the recovered second stack argument is unused. Both callers took
// the single runtime pointer out of the temporary result; return it directly.
extern "C" KinokoActRuntime *kinoko_act_source_create_runtime(KinokoActSourceHolder *holder) {
    kinoko::legacy::Allocation<KinokoActRuntime> storage(
        static_cast<KinokoActRuntime *>(std::malloc(sizeof(RuntimeRecord))));
    if (!storage) return nullptr;
    // Remaining constructor ABI boundary; no integer pointer storage inside
    // the holder/owner or the public source API.
    // 44FDE0 may throw while allocating its FindMap, before the caller has
    // received a runtime to own. Release the raw allocation on that unwind.
    auto *result = pointer<KinokoActRuntime>(function_44fde0(address(storage.get()), address(holder)));
    if (result) storage.release();
    return result;
}
