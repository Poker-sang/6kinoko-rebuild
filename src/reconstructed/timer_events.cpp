#include "kinoko/timer_events.h"
#include <mmsystem.h>
#include <list>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <new>
namespace {
struct FrameTimer {
    CRITICAL_SECTION lock{};
    HANDLE thread{};
    DWORD thread_id{};
    DWORD period_ms{16};
    DWORD extra_delay_ms{};
    std::atomic<bool> running{false};
    bool initialized{}, period_requested{};
    std::list<HANDLE> events;
};
FrameTimer timer;
struct Lock {
    Lock() { EnterCriticalSection(&timer.lock); }
    ~Lock() { LeaveCriticalSection(&timer.lock); }
};
DWORD WINAPI timer_worker(void*) {
    const HANDLE delay = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    while (timer.running.load()) {
        const DWORD timeout = timer.period_ms + timer.extra_delay_ms;
        if (delay) WaitForSingleObject(delay, timeout);
        else Sleep(timeout); // Native allocation-failure guard, avoids spinning.
        timer.extra_delay_ms = 0;
        Lock lock;
        for (HANDLE event : timer.events) SetEvent(event);
    }
    if (delay) CloseHandle(delay);
    return 0;
}
void shutdown() {
    if (!timer.initialized) return;
    timer.running.store(false);
    if (timer.thread) {
        SetThreadPriority(timer.thread, THREAD_PRIORITY_TIME_CRITICAL);
        WaitForSingleObject(timer.thread, INFINITE);
        CloseHandle(timer.thread); timer.thread = nullptr;
    }
    {
        Lock lock;
        for (HANDLE event : timer.events) { SetEvent(event); CloseHandle(event); }
        timer.events.clear();
    }
    DeleteCriticalSection(&timer.lock);
    if (timer.period_requested) timeEndPeriod(1);
    timer.initialized = timer.period_requested = false;
}
}
extern "C" void kinoko_frame_timer_initialize(void) {
    if (timer.initialized) return;
    InitializeCriticalSection(&timer.lock);
    if (std::atexit(shutdown) != 0) { DeleteCriticalSection(&timer.lock); throw std::bad_alloc(); }
    timer.initialized = true;
    timer.period_requested = timeBeginPeriod(1) == TIMERR_NOERROR;
    timer.running.store(true);
    timer.thread = CreateThread(nullptr, 0, timer_worker, nullptr, 0, &timer.thread_id);
    if (timer.thread) SetThreadPriority(timer.thread, THREAD_PRIORITY_TIME_CRITICAL);
}
extern "C" HANDLE kinoko_frame_timer_register(void) {
    if (!timer.initialized || !timer.thread) return nullptr;
    Lock lock;
    // 412BA6 inserts a null node before creating its auto-reset event.
    timer.events.push_back(nullptr);
    timer.events.back() = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    return timer.events.back();
}
extern "C" int32_t kinoko_frame_timer_unregister(HANDLE event) {
    if (!timer.initialized) return 0;
    Lock lock;
    const auto found = std::find(timer.events.begin(), timer.events.end(), event);
    if (found == timer.events.end()) return 0;
    SetEvent(*found);
    const BOOL closed = CloseHandle(*found);
    timer.events.erase(found);
    return closed ? 1 : 0;
}
extern "C" void kinoko_frame_timer_wait(HANDLE event) {
    // 40DECD: skip the wait when the registry is locked or the event is absent.
    // The update thread owns this registration; shutdown joins it before CRT teardown.
    if (!timer.initialized || !TryEnterCriticalSection(&timer.lock)) return;
    const bool registered = std::find(timer.events.begin(), timer.events.end(), event) != timer.events.end();
    LeaveCriticalSection(&timer.lock);
    if (registered) WaitForSingleObject(event, INFINITE);
}
