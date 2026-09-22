#pragma once
#include <windows.h>
extern "C" CRITICAL_SECTION g676;
namespace kinoko::graphics {
// Original recursive lock; a successful BeginScene deliberately holds it
// until EndScene, so those two entry points use explicit acquisition/release.
class Lock final {
public:
    Lock() { EnterCriticalSection(&g676); }
    ~Lock() { LeaveCriticalSection(&g676); }
    Lock(const Lock&)=delete;
    Lock& operator=(const Lock&)=delete;
};
}
