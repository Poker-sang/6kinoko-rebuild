#pragma once
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoCriticalSection KinokoCriticalSection;
typedef struct KinokoCriticalSectionMethods {
    KinokoCriticalSection* (__fastcall *destroy)(KinokoCriticalSection*, void*, unsigned);
} KinokoCriticalSectionMethods;
/* Original Common::CCriticalSection: vptr followed by the Win32 lock. */
struct KinokoCriticalSection {
    const KinokoCriticalSectionMethods* methods;
    CRITICAL_SECTION native;
};
extern const KinokoCriticalSectionMethods kinoko_critical_section_methods;
KinokoCriticalSection* kinoko_critical_section_construct(KinokoCriticalSection* self);
void kinoko_critical_section_destruct(KinokoCriticalSection* self);
KinokoCriticalSection* __fastcall kinoko_critical_section_delete(
    KinokoCriticalSection* self, void* unused_edx, unsigned flags);
extern KinokoCriticalSection kinoko_graphics_lock;
#ifdef __cplusplus
}
#endif
