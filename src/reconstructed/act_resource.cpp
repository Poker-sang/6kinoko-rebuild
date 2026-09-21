#include "kinoko/act_resource.h"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_document_records.hpp"
#include "kinoko/stage_records.hpp"
#include "kinoko/act_frame.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/windows_owner.hpp"
#include <mmsystem.h>

extern "C" {
void retdec_trace_i32(const char *label, int32_t value);
int32_t retdec_act_suspend_this(int32_t resource);
int32_t retdec_act_resume_this(int32_t resource);
}

namespace {
using kinoko::act::RuntimeRecord;
using kinoko::act::DocumentRecord;
using kinoko::native::RecordView;
using kinoko::stage::SourceHolderRecord;
using kinoko::legacy::address;

// This view borrows the existing record; it does not construct a new object or
// change the lifetime of the source holder, clone, VM, or critical section.
class ActResourceView {
    KinokoActRuntime *storage_;
    RecordView<RuntimeRecord> record_;

    KinokoActDocument *source_document() const {
        if (!storage_) return nullptr;
        auto *holder = record_.get(&RuntimeRecord::source_holder);
        return holder ? RecordView<SourceHolderRecord>(holder).get(&SourceHolderRecord::document) : nullptr;
    }

    int32_t resolution() const {
        auto *source = source_document();
        return source ? RecordView<DocumentRecord>(source).get(&DocumentRecord::resolution_ms) : 0;
    }

public:
    explicit ActResourceView(KinokoActRuntime *storage) : storage_(storage), record_(storage) {}

    int32_t set_time(int32_t time) const {
        if (storage_) record_.set(&RuntimeRecord::current_time, time);
        return 0;
    }

    int32_t time() const {
        return storage_ ? record_.get(&RuntimeRecord::current_time) : 0;
    }

    int32_t frame() const {
        const int32_t step = resolution();
        return step ? time() / step : 0;
    }

    int32_t increment_frame() const {
        if (!storage_) return 0;
        static volatile LONG trace_count;
        const LONG index = InterlockedIncrement(&trace_count);
        auto *holder = record_.get(&RuntimeRecord::source_holder);
        auto *source = holder ? RecordView<SourceHolderRecord>(holder).get(&SourceHolderRecord::document) : nullptr;
        const int32_t step = source ? RecordView<DocumentRecord>(source).get(&DocumentRecord::resolution_ms) : 0;
        if (index <= 64) {
            retdec_trace_i32("451620:resource", address(storage_));
            retdec_trace_i32("451620:before", time());
            retdec_trace_i32("451620:holder", address(holder));
            retdec_trace_i32("451620:act", address(source));
            retdec_trace_i32("451620:resolution", step);
        }
        // Original 451620 uses a DWORD ADD. Compute modulo 2^32, then copy the
        // result bits to the signed clock; no signed-overflow expression.
        if (source) {
            const uint32_t next = static_cast<uint32_t>(time()) + static_cast<uint32_t>(step);
            record_.set(&RuntimeRecord::current_time, kinoko::legacy::load<int32_t>(&next));
        }
        if (index <= 64) retdec_trace_i32("451620:after", time());
        return 0;
    }

    int32_t sleep_to(int32_t milliseconds) const {
        // The OS clock is sampled even for a null receiver, as in the baseline.
        const uint32_t deadline = timeGetTime() + static_cast<uint32_t>(milliseconds);
        if (storage_) record_.set(&RuntimeRecord::wake_time, deadline);
        return kinoko::legacy::load<int32_t>(&deadline);
    }

    int32_t end_stage() const {
        if (!storage_ || !record_.get(&RuntimeRecord::stage_active)) return E_FAIL;
        kinoko::windows::CriticalLock lock(reinterpret_cast<CRITICAL_SECTION *>(record_.bytes(&RuntimeRecord::lock)));
        record_.set(&RuntimeRecord::stage_active, uint8_t{0});
        record_.set(&RuntimeRecord::stage_state, std::array<uint32_t, 11>{});
        // Preserve the inherited published-vector write in this type-only
        // change. Native container semantics are audited separately.
        const auto commands = record_.view(&RuntimeRecord::draw_commands);
        commands.set(&kinoko::act::VectorStorage::end, commands.get(&kinoko::act::VectorStorage::begin));
        retdec_act_clear_layout_vector(address(record_.bytes(&RuntimeRecord::draw_sprites)));
        return 0;
    }
};
}

// The ECX receiver is a real runtime pointer, including on the C ABI. EDX is
// unused by the existing x86 native-binding adapter; stack arguments are intact.
extern "C" int32_t __fastcall kinoko_act_set_current_time(KinokoActRuntime *resource, void *, int32_t time) {
    return ActResourceView(resource).set_time(time);
}
extern "C" int32_t __fastcall kinoko_act_increment_frame(KinokoActRuntime *resource, void *) {
    return ActResourceView(resource).increment_frame();
}
extern "C" int32_t __fastcall kinoko_act_get_current_time(KinokoActRuntime *resource, void *) {
    return ActResourceView(resource).time();
}
extern "C" int32_t __fastcall kinoko_act_get_current_frame(KinokoActRuntime *resource, void *) {
    return ActResourceView(resource).frame();
}
extern "C" int32_t __fastcall kinoko_act_end_stage(KinokoActRuntime *resource, void *) {
    return ActResourceView(resource).end_stage();
}

// 451590/4515A0: blocking sleep and deferred wake time stay distinct.
extern "C" int32_t __fastcall kinoko_act_sleep(KinokoActRuntime *, void *, int32_t milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
    return 0;
}
extern "C" int32_t __fastcall kinoko_act_sleep_to(KinokoActRuntime *resource, void *, int32_t milliseconds) {
    return ActResourceView(resource).sleep_to(milliseconds);
}

// The unverified suspend/resume bodies remain explicit legacy boundaries.
extern "C" int32_t __fastcall kinoko_act_suspend(KinokoActRuntime *resource, void *) {
    return retdec_act_suspend_this(address(resource));
}
extern "C" int32_t __fastcall kinoko_act_resume(KinokoActRuntime *resource, void *) {
    return retdec_act_resume_this(address(resource));
}
