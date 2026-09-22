#include "kinoko/critical_section.h"
#include <cstddef>
#include <cstdlib>

static_assert(sizeof(KinokoCriticalSection) == 28);
static_assert(offsetof(KinokoCriticalSection, native) == 4);
extern "C" const KinokoCriticalSectionMethods kinoko_critical_section_methods{
    kinoko_critical_section_delete
};
extern "C" KinokoCriticalSection* kinoko_critical_section_construct(KinokoCriticalSection* self) {
    self->methods = &kinoko_critical_section_methods;
    InitializeCriticalSection(&self->native);
    return self;
}
extern "C" void kinoko_critical_section_destruct(KinokoCriticalSection* self) {
    self->methods = &kinoko_critical_section_methods;
    DeleteCriticalSection(&self->native);
}
extern "C" KinokoCriticalSection* __fastcall kinoko_critical_section_delete(
    KinokoCriticalSection* self, void*, unsigned flags) {
    kinoko_critical_section_destruct(self);
    // Host original operator-delete adapter uses the CRT allocation family.
    if (flags & 1u) std::free(self);
    return self; // Original scalar-deleting destructor returns its receiver.
}
