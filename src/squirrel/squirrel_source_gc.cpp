#include "kinoko/squirrel_source_runtime.h"
#include <cstddef>
#include <typeinfo>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqclass.h"
#include "squserdata.h"

namespace {
template<class T> T* pointer(int32_t value) noexcept {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(static_cast<uint32_t>(value)));
}
int32_t address(const void* value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
static_assert(sizeof(SQSharedState) == 180);
static_assert(sizeof(SQVM) == 168 && sizeof(SQArray) == 36);
static_assert(offsetof(SQCollectable, _uiRef) == 4);
static_assert(offsetof(SQCollectable, _next) == 12);
static_assert(offsetof(SQCollectable, _prev) == 16);
static_assert(offsetof(SQCollectable, _sharedstate) == 20);
static_assert(offsetof(SQSharedState, _gc_chain) == 68);
}

// MSVC x86 __fastcall receives this in ECX; unused EDX replaces no stack
// argument. The remaining chain pointer has the original __thiscall layout.
// Qualified calls avoid redispatching to the recovered slot, and contain no
// inline assembly or reconstructed member-offset traversal.
#define KINOKO_GC_ADAPTER(name, Class) \
    extern "C" void __fastcall kinoko_sq_##name##_mark( \
        int32_t object, void*, int32_t* chain) { \
        auto* head = pointer<SQCollectable>(*chain); \
        pointer<Class>(object)->Class::Mark(&head); \
        *chain = address(head); \
    } \
    extern "C" void __fastcall kinoko_sq_##name##_finalize(int32_t object, void*) { \
        pointer<Class>(object)->Class::Finalize(); \
    }
KINOKO_GC_ADAPTER(closure, SQClosure)
KINOKO_GC_ADAPTER(nativeclosure, SQNativeClosure)
KINOKO_GC_ADAPTER(userdata, SQUserData)
KINOKO_GC_ADAPTER(array, SQArray)
KINOKO_GC_ADAPTER(generator, SQGenerator)
KINOKO_GC_ADAPTER(vm, SQVM)
KINOKO_GC_ADAPTER(table, SQTable)
KINOKO_GC_ADAPTER(instance, SQInstance)
KINOKO_GC_ADAPTER(class, SQClass)
#undef KINOKO_GC_ADAPTER

extern "C" void kinoko_sq_mark_value(const int32_t* value, int32_t* chain) {
    if (!value || !chain || !value[1]) return;
    // The original object pair supplies its type. Qualifying this first call
    // also supports the existing zero-vtable synthetic GC-link contract.
    switch (value[0]) {
    case OT_CLOSURE: kinoko_sq_closure_mark(value[1], nullptr, chain); break;
    case OT_NATIVECLOSURE: kinoko_sq_nativeclosure_mark(value[1], nullptr, chain); break;
    case OT_USERDATA: kinoko_sq_userdata_mark(value[1], nullptr, chain); break;
    case OT_ARRAY: kinoko_sq_array_mark(value[1], nullptr, chain); break;
    case OT_GENERATOR: kinoko_sq_generator_mark(value[1], nullptr, chain); break;
    case OT_THREAD: kinoko_sq_vm_mark(value[1], nullptr, chain); break;
    case OT_TABLE: kinoko_sq_table_mark(value[1], nullptr, chain); break;
    case OT_INSTANCE: kinoko_sq_instance_mark(value[1], nullptr, chain); break;
    case OT_CLASS: kinoko_sq_class_mark(value[1], nullptr, chain); break;
    default: break; // Strings/prototypes/weakrefs are not collectable nodes.
    }
}
extern "C" void kinoko_sq_finalize_object(int32_t object, int32_t type) {
    if (!object) return;
    switch (type) {
    case OT_CLOSURE: kinoko_sq_closure_finalize(object, nullptr); break;
    case OT_NATIVECLOSURE: kinoko_sq_nativeclosure_finalize(object, nullptr); break;
    case OT_USERDATA: kinoko_sq_userdata_finalize(object, nullptr); break;
    case OT_ARRAY: kinoko_sq_array_finalize(object, nullptr); break;
    case OT_GENERATOR: kinoko_sq_generator_finalize(object, nullptr); break;
    case OT_THREAD: kinoko_sq_vm_finalize(object, nullptr); break;
    case OT_TABLE: kinoko_sq_table_finalize(object, nullptr); break;
    case OT_INSTANCE: kinoko_sq_instance_finalize(object, nullptr); break;
    case OT_CLASS: kinoko_sq_class_finalize(object, nullptr); break;
    default: break;
    }
}
extern "C" int32_t kinoko_sq_collect(SQSharedState* state, SQVM* vm) {
    return state->CollectGarbage(vm);
}
extern "C" int32_t kinoko_sq_source_object_type(SQCollectable* object) {
    // Only source-constructed objects reach this path. Recovered vtables have
    // no RTTI and must be classified by the caller BEFORE requesting typeid.
    if (!object) return 0;
    const auto& type = typeid(*object);
    if (type == typeid(SQClosure)) return OT_CLOSURE;
    if (type == typeid(SQNativeClosure)) return OT_NATIVECLOSURE;
    if (type == typeid(SQUserData)) return OT_USERDATA;
    if (type == typeid(SQArray)) return OT_ARRAY;
    if (type == typeid(SQGenerator)) return OT_GENERATOR;
    if (type == typeid(SQVM)) return OT_THREAD;
    if (type == typeid(SQTable)) return OT_TABLE;
    if (type == typeid(SQInstance)) return OT_INSTANCE;
    if (type == typeid(SQClass)) return OT_CLASS;
    return 0;
}
