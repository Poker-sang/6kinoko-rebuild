#include "kinoko/critical_section.h"
#pragma once
#include <windows.h>

namespace kinoko::graphics {
// Original recursive lock; a successful BeginScene deliberately holds it
// until EndScene, so those two entry points use explicit acquisition/release.
class Lock final {
public:
    Lock() { EnterCriticalSection(&kinoko_graphics_lock.native); }
    ~Lock() { LeaveCriticalSection(&kinoko_graphics_lock.native); }
    Lock(const Lock&)=delete;
    Lock& operator=(const Lock&)=delete;
};
}
