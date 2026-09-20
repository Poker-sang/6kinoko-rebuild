#include "kinoko/scene_queue.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include <windows.h>
#include <list>
#include <stdexcept>
extern "C" {
extern int32_t g863,g864,g867,g870,g871;
extern CRITICAL_SECTION g869;
}
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
std::list<int32_t> retired_scenes;
struct SceneLock {
    SceneLock() { EnterCriticalSection(&g869); }
    ~SceneLock() { LeaveCriticalSection(&g869); }
};
int32_t call(int32_t object,int slot,int32_t argument) {
    const auto table=field<int32_t>(object);
    return retdec_call_thiscall1_result(pointer<void>(object),pointer<void>(field<int32_t>(table+4*slot)),argument);
}
}
extern "C" void kinoko_initialize_scene_queue(void) { retired_scenes.clear(); }
extern "C" int32_t function_40da60(void) {
    if(!g864) return 0;
    int32_t previous;
    {
        SceneLock lock;
        previous=g863;g863=g864;g864=0;
    }
    if(previous) {
        call(previous,5,g870);
        {
            // The standard list's cross-thread mutations need synchronization.
            // Reuse the scene lock, never hold it during user callbacks.
            SceneLock lock;
            if(retired_scenes.size()==0x3ffffffeu) throw std::length_error("list<T> too long");
            retired_scenes.push_back(previous);
        }
        SetEvent(pointer<void>(g867));
    }
    int32_t result=0;
    if(g863) result=call(g863,4,g871);
    g871=g870;
    return result;
}
extern "C" void kinoko_destroy_retired_scenes(void) {
    for(;;) {
        int32_t object;
        {
            SceneLock lock;
            if(retired_scenes.empty()) return;
            object=retired_scenes.front();
        }
        // Single consumer, insertion only at the back. Preserve original
        // deleting-destructor-before-node-removal order (40E377-40E3AC).
        if(object) call(object,0,1);
        {
            SceneLock lock;
            retired_scenes.pop_front();
        }
    }
}
