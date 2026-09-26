#include "kinoko/squirrel_value_bridge.h"
#include <cstddef>
#include <cstdlib>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqclass.h"
#include "sqarray.h"
#include "sqstring.h"
#include "sqclosure.h"


namespace {
template<class T> T &at(int32_t p) {
    return *reinterpret_cast<T *>(static_cast<uintptr_t>(static_cast<uint32_t>(p)));
}
template<class T> int32_t address(T *p) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p));
}
static_assert(sizeof(SQObjectPtr) == 8 && sizeof(SQVM::CallInfo) == 48);
static_assert(offsetof(SQVM::CallInfo, _vargs) == 44);
static_assert(offsetof(SQVM, _vargsstack) == 36);

// A script metamethod can grow the VM stack. Keep a slot index across it,
// while allowing callers to supply ordinary local output objects as well.
class Output {
public:
    Output(SQVM &vm, SQObjectPtr* p) : vm_(vm), pointer_(p) {
        const auto begin = reinterpret_cast<uintptr_t>(vm._stack._vals);
        const auto raw = reinterpret_cast<uintptr_t>(pointer_);
        if (raw >= begin && raw < begin + vm._stack.size() * sizeof(SQObjectPtr) &&
            (raw - begin) % sizeof(SQObjectPtr) == 0)
            index_ = static_cast<int>((raw - begin) / sizeof(SQObjectPtr));
    }
    SQObjectPtr &get() const { return index_ < 0 ? *pointer_ : vm_._stack[index_]; }
private:
    SQVM &vm_;
    SQObjectPtr *pointer_;
    int index_ = -1;
};
}

// Original 491500 / SQVM::GETVARGV_OP. The source operation preserves numeric
// conversion, the negative bound, diagnostic text and acquire-before-release.
extern "C" int32_t kinoko_sq_get_vararg(SQVM* vm, SQObjectPtr* target,
                                         SQObjectPtr* index, void* call_info) {
    return (*vm).GETVARGV_OP((*target), (*index),
                                   &(*reinterpret_cast<SQVM::CallInfo*>(call_info)));
}

// VM operations use the same source implementation as the bytecode interpreter.
extern "C" int32_t kinoko_sq_clone(SQVM* vm, SQObjectPtr* source, SQObjectPtr* target) {
    auto &machine = (*vm);
    Output output(machine, target);
    SQObjectPtr self = (*source), result;
    if (!machine.Clone(self, result)) return false;
    output.get() = result;
    return true;
}

extern "C" int32_t kinoko_sq_foreach(SQVM* vm, SQObjectPtr* object, SQObjectPtr* key,
    SQObjectPtr* value, SQObjectPtr* iterator, int32_t arg2, int32_t exitpos, int32_t *jump) {
    auto &machine = (*vm);
    return machine.FOREACH_OP((*object), (*key),
        (*value), (*iterator), arg2, exitpos, *jump);
}
