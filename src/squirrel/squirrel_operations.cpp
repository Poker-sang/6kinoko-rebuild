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
    Output(SQVM &vm, int32_t p) : vm_(vm), pointer_(&at<SQObjectPtr>(p)) {
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
extern "C" int32_t kinoko_sq_get_vararg(int32_t vm, int32_t target,
                                         int32_t index, int32_t call_info) {
    return at<SQVM>(vm).GETVARGV_OP(at<SQObjectPtr>(target), at<SQObjectPtr>(index),
                                   &at<SQVM::CallInfo>(call_info));
}

// VM operations use the same source implementation as the bytecode interpreter.
extern "C" int32_t kinoko_sq_clone(int32_t vm, int32_t source, int32_t target) {
    auto &machine = at<SQVM>(vm);
    Output output(machine, target);
    SQObjectPtr self = at<SQObjectPtr>(source), result;
    if (!machine.Clone(self, result)) return false;
    output.get() = result;
    return true;
}

extern "C" int32_t kinoko_sq_foreach(int32_t vm, int32_t object, int32_t key,
    int32_t value, int32_t iterator, int32_t arg2, int32_t exitpos, int32_t *jump) {
    auto &machine = at<SQVM>(vm);
    return machine.FOREACH_OP(at<SQObjectPtr>(object), at<SQObjectPtr>(key),
        at<SQObjectPtr>(value), at<SQObjectPtr>(iterator), arg2, exitpos, *jump);
}
