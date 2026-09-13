#include "kinoko/squirrel_value_bridge.h"
#include <cstddef>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqclosure.h"
#include "sqarray.h"

namespace {
static_assert(sizeof(SQGenerator) == 120);
static_assert(offsetof(SQGenerator, _stack) == 32);
static_assert(offsetof(SQGenerator, _ci) == 56);
static_assert(offsetof(SQGenerator, _state) == 116);
static_assert(sizeof(SQVM::CallInfo) == 48);
static_assert(offsetof(SQVM, ci) == 132);

template <typename T> T *pointer(int32_t address) {
    return reinterpret_cast<T *>(static_cast<uintptr_t>(static_cast<uint32_t>(address)));
}
}

// Original 490370/490630 match the supplied 2.2.2 generator implementation.
// In particular Resume saves the CALLER's top before installing the new frame.
extern "C" int32_t kinoko_sq_generator_yield(int32_t generator, int32_t vm) {
    return pointer<SQGenerator>(generator)->Yield(pointer<SQVM>(vm));
}

extern "C" int32_t kinoko_sq_generator_resume(int32_t generator, int32_t vm, int32_t target) {
    return pointer<SQGenerator>(generator)->Resume(pointer<SQVM>(vm), target);
}

extern "C" void kinoko_sq_generator_kill(int32_t generator) {
    pointer<SQGenerator>(generator)->Kill();
}

// Original 4A36D0 / sqbaselib.cpp::array_remove. Retain the result through
// vector compaction, then push it before the local reference is released.
extern "C" int32_t kinoko_sq_array_remove(int32_t vm) {
    auto *v = pointer<SQVM>(vm);
    const SQObjectPtr &index = stack_get(v, 2);
    if (!sq_isnumeric(index))
        return sq_throwerror(v, _SC("wrong type"));
    auto *array = _array(stack_get(v, 1));
    SQObjectPtr removed;
    const SQInteger position = tointeger(index);
    if (!array->Get(position, removed))
        return sq_throwerror(v, _SC("idx out of range"));
    array->Remove(position);
    v->Push(removed);
    return 1;
}
