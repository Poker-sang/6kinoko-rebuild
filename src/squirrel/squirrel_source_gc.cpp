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
        Class* object, void*, SQCollectable** chain) { \
        auto* head = *chain; \
        object->Class::Mark(&head); \
        *chain = head; \
    } \
    extern "C" void __fastcall kinoko_sq_##name##_finalize(Class* object, void*) { \
        object->Class::Finalize(); \
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

extern "C" void kinoko_sq_mark_value(const HSQOBJECT* value, SQCollectable** chain) {
    if (!value || !chain || !value->_unVal.pRefCounted) return;
    // The original object pair supplies its type. Qualifying this first call
    // also supports the existing zero-vtable synthetic GC-link contract.
    switch (value->_type) {
    case OT_CLOSURE: kinoko_sq_closure_mark(reinterpret_cast<SQClosure*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_NATIVECLOSURE: kinoko_sq_nativeclosure_mark(reinterpret_cast<SQNativeClosure*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_USERDATA: kinoko_sq_userdata_mark(reinterpret_cast<SQUserData*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_ARRAY: kinoko_sq_array_mark(reinterpret_cast<SQArray*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_GENERATOR: kinoko_sq_generator_mark(reinterpret_cast<SQGenerator*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_THREAD: kinoko_sq_vm_mark(reinterpret_cast<SQVM*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_TABLE: kinoko_sq_table_mark(reinterpret_cast<SQTable*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_INSTANCE: kinoko_sq_instance_mark(reinterpret_cast<SQInstance*>(value->_unVal.pRefCounted), nullptr, chain); break;
    case OT_CLASS: kinoko_sq_class_mark(reinterpret_cast<SQClass*>(value->_unVal.pRefCounted), nullptr, chain); break;
    default: break; // Strings/prototypes/weakrefs are not collectable nodes.
    }
}
extern "C" void kinoko_sq_finalize_object(SQCollectable* object, int32_t type) {
    if (!object) return;
    switch (type) {
    case OT_CLOSURE: kinoko_sq_closure_finalize(static_cast<SQClosure*>(object), nullptr); break;
    case OT_NATIVECLOSURE: kinoko_sq_nativeclosure_finalize(static_cast<SQNativeClosure*>(object), nullptr); break;
    case OT_USERDATA: kinoko_sq_userdata_finalize(static_cast<SQUserData*>(object), nullptr); break;
    case OT_ARRAY: kinoko_sq_array_finalize(static_cast<SQArray*>(object), nullptr); break;
    case OT_GENERATOR: kinoko_sq_generator_finalize(static_cast<SQGenerator*>(object), nullptr); break;
    case OT_THREAD: kinoko_sq_vm_finalize(static_cast<SQVM*>(object), nullptr); break;
    case OT_TABLE: kinoko_sq_table_finalize(static_cast<SQTable*>(object), nullptr); break;
    case OT_INSTANCE: kinoko_sq_instance_finalize(static_cast<SQInstance*>(object), nullptr); break;
    case OT_CLASS: kinoko_sq_class_finalize(static_cast<SQClass*>(object), nullptr); break;
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
