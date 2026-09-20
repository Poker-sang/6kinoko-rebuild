#include "kinoko/timer_events.h"
#include "kinoko/diagnostics.h"
#include <windows.h>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <new>
extern "C" {
extern CRITICAL_SECTION g880;
extern int32_t g881,g882;
extern char g887;
void retdec_trace_i32(const char*,int32_t);
}
namespace {
std::list<HANDLE> events;
struct Lock {
    Lock() { EnterCriticalSection(&g880); }
    ~Lock() { LeaveCriticalSection(&g880); }
};
// Original 4129D0 stops/joins the timer before closing queued events and
// destroying its critical section. Register before the worker is created;
// this runs before the earlier-constructed std::list static destructor.
void shutdown() {
    g887=0;
    if(g881) {
        const auto thread=reinterpret_cast<HANDLE>(static_cast<intptr_t>(g881));
        SetThreadPriority(thread,THREAD_PRIORITY_TIME_CRITICAL);
        WaitForSingleObject(thread,INFINITE);CloseHandle(thread);g881=0;g882=0;
    }
    {
        Lock lock;
        for(HANDLE event:events) { SetEvent(event);CloseHandle(event); }
        events.clear();
    }
    DeleteCriticalSection(&g880);
}
}
extern "C" void kinoko_initialize_timer_events(void) {
    events.clear();
    if(std::atexit(shutdown)!=0) throw std::bad_alloc();
}
extern "C" int32_t kinoko_timer_events_identity(void) { return static_cast<int32_t>(reinterpret_cast<intptr_t>(&events)); }
extern "C" int32_t kinoko_timer_events_first(void) {
    return events.empty()?kinoko_timer_events_identity():static_cast<int32_t>(reinterpret_cast<intptr_t>(&events.front()));
}
extern "C" void kinoko_notify_timer_events(void) {
    // Called with the timer critical section held, in original insertion order.
    for(HANDLE event:events) SetEvent(event);
}
extern "C" int32_t function_412b80(int32_t) {
    retdec_trace("412b80:begin");retdec_trace("412b80:before-lock");
    Lock lock;
    retdec_trace_i32("412b80:sentinel",kinoko_timer_events_identity());
    if(events.size()==0x3ffffffeu) throw std::length_error("list<T> too long");
    // 412BA6 inserts a null node before 412BEF creates its auto-reset event.
    // Even a failed CreateEvent leaves that null entry, as in the original.
    events.push_back(nullptr);
    events.back()=CreateEventA(nullptr,FALSE,FALSE,nullptr);
    const auto result=static_cast<int32_t>(reinterpret_cast<intptr_t>(events.back()));
    retdec_trace_i32("412b80:event",result);retdec_trace("412b80:done");
    return result;
}
extern "C" int32_t function_412c10(int32_t value) {
    Lock lock;
    const auto event=reinterpret_cast<HANDLE>(static_cast<intptr_t>(value));
    const auto found=std::find(events.begin(),events.end(),event);
    if(found==events.end()) return 0;
    SetEvent(*found);const BOOL closed=CloseHandle(*found);events.erase(found);
    return closed?1:0;
}
