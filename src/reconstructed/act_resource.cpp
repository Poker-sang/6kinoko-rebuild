#include "kinoko/act_resource.h"

#include <cstring>
#include <windows.h>
#include <mmsystem.h>

extern "C" {
void retdec_trace_i32(const char *label, int32_t value);
int32_t retdec_act_clear_layout_vector(int32_t vector);
int32_t retdec_act_suspend_this(int32_t resource);
int32_t retdec_act_resume_this(int32_t resource);
}

namespace {
static_assert(sizeof(void *) == 4 && sizeof(CRITICAL_SECTION) == 24,
              "ACT resource fields require the original Win32 layout.");

class ResourceLock {
public:
    explicit ResourceLock(CRITICAL_SECTION &section) : section_(section) {
        EnterCriticalSection(&section_);
    }
    ~ResourceLock() { LeaveCriticalSection(&section_); }
    ResourceLock(const ResourceLock &) = delete;
    ResourceLock &operator=(const ResourceLock &) = delete;
private:
    CRITICAL_SECTION &section_;
};

class ActResourceView {
public:
    explicit ActResourceView(int32_t address) : address_(address) {}

    int32_t set_time(int32_t time) const {
        if (address_)
            field<int32_t>(current_time) = time;
        return 0;
    }

    int32_t time() const {
        return address_ ? field<int32_t>(current_time) : 0;
    }

    int32_t frame() const {
        const int32_t step = resolution();
        return step ? time() / step : 0;
    }

    int32_t increment_frame() const {
        if (!address_)
            return 0;
        static volatile LONG trace_count;
        const LONG index = InterlockedIncrement(&trace_count);
        const int32_t holder = field<int32_t>(act_holder);
        const int32_t act = holder ? at<int32_t>(holder) : 0;
        const int32_t step = act ? at<int32_t>(act + 4) : 0;
        if (index <= 64) {
            retdec_trace_i32("451620:resource", address_);
            retdec_trace_i32("451620:before", time());
            retdec_trace_i32("451620:holder", holder);
            retdec_trace_i32("451620:act", act);
            retdec_trace_i32("451620:resolution", step);
        }
        // 451620 uses an x86 ADD, including 32-bit wraparound.
        if (act)
            field<uint32_t>(current_time) += static_cast<uint32_t>(step);
        if (index <= 64)
            retdec_trace_i32("451620:after", time());
        return 0;
    }

    int32_t sleep_to(int32_t milliseconds) const {
        const DWORD deadline = timeGetTime() + static_cast<DWORD>(milliseconds);
        if (address_)
            field<DWORD>(wake_time) = deadline;
        return static_cast<int32_t>(deadline);
    }

    int32_t end_stage() const {
        if (!address_ || !field<uint8_t>(stage_active))
            return E_FAIL;
        ResourceLock lock(field<CRITICAL_SECTION>(critical_section));
        field<uint8_t>(stage_active) = 0;
        std::memset(&field<unsigned char>(stage_state), 0, stage_state_size);
        field<int32_t>(draw_end) = field<int32_t>(draw_begin);
        retdec_act_clear_layout_vector(address_ + layouts);
        return 0;
    }

private:
    enum Offset {
        act_holder = 0, current_time = 4, stage_active = 8,
        critical_section = 20, draw_begin = 44, draw_end = 48,
        layouts = 60, wake_time = 100, stage_state = 108, stage_state_size = 44
    };

    template <typename T>
    static T &at(int32_t address) {
        return *reinterpret_cast<T *>(static_cast<uintptr_t>(
            static_cast<uint32_t>(address)));
    }

    template <typename T>
    T &field(int offset) const { return at<T>(address_ + offset); }

    int32_t resolution() const {
        if (!address_)
            return 0;
        const int32_t holder = field<int32_t>(act_holder);
        const int32_t act = holder ? at<int32_t>(holder) : 0;
        return act ? at<int32_t>(act + 4) : 0;
    }

    int32_t address_;
};
}

// 451610, 451620, 4A9A30, 451630: original ActingPlayer clock methods.
extern "C" int32_t __fastcall kinoko_act_set_current_time(
    int32_t resource, void *, int32_t time) {
    return ActResourceView(resource).set_time(time);
}
extern "C" int32_t __fastcall kinoko_act_increment_frame(int32_t resource, void *) {
    return ActResourceView(resource).increment_frame();
}
extern "C" int32_t __fastcall kinoko_act_get_current_time(int32_t resource, void *) {
    return ActResourceView(resource).time();
}
extern "C" int32_t __fastcall kinoko_act_get_current_frame(int32_t resource, void *) {
    return ActResourceView(resource).frame();
}

// 450D80: release the stage's pending work while holding its resource lock.
extern "C" int32_t __fastcall kinoko_act_end_stage(int32_t resource, void *) {
    return ActResourceView(resource).end_stage();
}

// 451590/4515A0: blocking sleep and deferred wake time are distinct operations.
extern "C" int32_t __fastcall kinoko_act_sleep(int32_t, void *, int32_t milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
    return 0;
}
extern "C" int32_t __fastcall kinoko_act_sleep_to(
    int32_t resource, void *, int32_t milliseconds) {
    return ActResourceView(resource).sleep_to(milliseconds);
}

// Keep the compatibility bodies until the original virtual dispatch is restored.
extern "C" int32_t __fastcall kinoko_act_suspend(int32_t resource, void *) {
    return retdec_act_suspend_this(resource);
}
extern "C" int32_t __fastcall kinoko_act_resume(int32_t resource, void *) {
    return retdec_act_resume_this(resource);
}
