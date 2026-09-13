#include "kinoko/squirrel_value_bridge.h"
#include <cstddef>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqarray.h"

namespace {
static_assert(sizeof(SQObjectPtr) == 8);
static_assert(offsetof(SQArray, _values) == 24);
static_assert(offsetof(SQVM, _stack) == 24 && offsetof(SQVM, _stackbase) == 52);

SQVM *machine(int32_t address) {
    return reinterpret_cast<SQVM *>(static_cast<uintptr_t>(static_cast<uint32_t>(address)));
}
}

// Original 48DD10 -> 48DAC0: push the last value, then pop/shrink the same array.
// Use the supplied 2.2.2 API so ownership, vector shrinking and errors stay together.
extern "C" int32_t kinoko_sq_array_pop_api(int32_t vm, int32_t index, int32_t push_value) {
    return sq_arraypop(machine(vm), index, push_value);
}

// sqbaselib.cpp::array_pop / original 4A25A0 returns SQ_ERROR on failure.
extern "C" int32_t kinoko_sq_array_pop(int32_t vm) {
    return SQ_SUCCEEDED(sq_arraypop(machine(vm), 1, SQTrue)) ? 1 : SQ_ERROR;
}

// sqbaselib.cpp::array_top / original 4A30D0. Keep the explicit VM receiver.
extern "C" int32_t kinoko_sq_array_top(int32_t vm) {
    auto *v = machine(vm);
    auto *array = _array(stack_get(v, 1));
    if (array->Size() > 0) {
        v->Push(array->Top());
        return 1;
    }
    return sq_throwerror(v, _SC("top() on a empty array"));
}
