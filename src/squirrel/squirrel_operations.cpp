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

extern "C" {
int32_t function_498440_this(int32_t source);
int32_t function_48d390_this(int32_t array, int32_t shared_state, int32_t size);
int32_t function_491400(int32_t source, int32_t shared_state);
int32_t function_497850(int32_t object, int32_t method, int32_t nargs, int32_t result);
int32_t function_491820(int32_t value);
int32_t function_4948a0(int32_t object, int32_t key, int32_t value, int32_t raw, int32_t root);
int32_t function_499a20(int32_t vm, const char *format, ...);
void retdec_trace(const char *message);
void retdec_trace_i32(const char *message, int32_t value);
}

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

// Original 494990 / SQVM::Clone. Keep the current game's allocator/instance
// adapters and script dispatcher; C++ owns all temporary values and assignment.
extern "C" int32_t kinoko_sq_clone(int32_t vm, int32_t source, int32_t target) {
    auto &machine = at<SQVM>(vm);
    Output output(machine, target);
    SQObjectPtr self = at<SQObjectPtr>(source);
    SQObjectPtr temporary, cloned;
    switch (type(self)) {
    case OT_TABLE: {
        retdec_trace("494990:clone-table");
        retdec_trace_i32("494990:clone-source", address(_table(self)));
        const auto result = function_498440_this(address(_table(self)));
        retdec_trace_i32("494990:clone-result", result);
        cloned = &at<SQTable>(result);
        break;
    }
    case OT_INSTANCE:
        cloned = &at<SQInstance>(function_491400(address(_instance(self)),
                                                address(machine._sharedstate)));
        break;
    case OT_ARRAY: {
        // The mixed VM identifies collectables by the reconstructed vtable.
        // Keep that constructor, then use source vector copy/owned assignment.
        auto *copy = static_cast<SQArray *>(std::malloc(sizeof(SQArray)));
        if (!copy) return false;
        function_48d390_this(address(copy), address(_array(self)->_sharedstate),
                            _array(self)->Size());
        copy->_values.copy(_array(self)->_values);
        output.get() = copy;
        return true;
    }
    default:
        return false;
    }
    if (_delegable(cloned)->_delegate) {
        function_491820(address(&cloned));
        function_491820(address(&self));
        // Original Clone ignores the _cloned return status.
        function_497850(address(_delegable(cloned)), MT_CLONED, 2, address(&temporary));
    }
    output.get() = cloned;
    return true;
}

// Original 494DA0 / SQVM::FOREACH_OP. Preserve the original next-index and
// jump protocol; use source Next methods and the current game metamethod path.
extern "C" int32_t kinoko_sq_foreach(int32_t vm, int32_t object, int32_t key,
                                      int32_t value, int32_t iterator,
                                      int32_t arg2, int32_t exitpos, int32_t *jump) {
    auto &machine = at<SQVM>(vm);
    Output outkey(machine, key), outvalue(machine, value), refpos(machine, iterator);
    SQObjectPtr self = at<SQObjectPtr>(object);
    SQInteger next;
    switch (type(self)) {
    case OT_TABLE:
        retdec_trace_i32("494da0:table", address(_table(self)));
        next = _table(self)->Next(false, refpos.get(), outkey.get(), outvalue.get());
        retdec_trace_i32("494da0:table-next", next);
        break;
    case OT_ARRAY:
        next = _array(self)->Next(refpos.get(), outkey.get(), outvalue.get());
        break;
    case OT_STRING:
        next = _string(self)->Next(refpos.get(), outkey.get(), outvalue.get());
        break;
    case OT_CLASS:
        next = _class(self)->Next(refpos.get(), outkey.get(), outvalue.get());
        break;
    case OT_USERDATA:
    case OT_INSTANCE: {
        if (!_delegable(self)->_delegate) return false;
        SQObjectPtr index;
        function_491820(address(&self));
        function_491820(address(&refpos.get()));
        if (!function_497850(address(_delegable(self)), MT_NEXTI, 2, address(&index))) {
            function_499a20(vm, "_nexti failed");
            return false;
        }
        refpos.get() = outkey.get() = index;
        if (type(index) == OT_NULL) {
            *jump = exitpos;
            return true;
        }
        SQObjectPtr result;
        if (!function_4948a0(address(&self), address(&index), address(&result), 0, 0)) {
            function_499a20(vm, "_nexti returned an invalid idx");
            return false;
        }
        outvalue.get() = result;
        *jump = 1;
        return true;
    }
    case OT_GENERATOR:
        if (_generator(self)->_state == SQGenerator::eDead) {
            *jump = exitpos;
            return true;
        }
        if (_generator(self)->_state == SQGenerator::eSuspended) {
            const SQInteger idx = type(refpos.get()) == OT_INTEGER ? _integer(refpos.get()) + 1 : 0;
            outkey.get() = idx;
            refpos.get() = idx;
            kinoko_sq_generator_resume(address(_generator(self)), vm, arg2 + 1);
            *jump = 0;
            return true;
        }
        [[fallthrough]];
    default:
        function_499a20(vm, "cannot iterate %s", GetTypeName(self));
        return false;
    }
    if (next == -1) *jump = exitpos;
    else {
        refpos.get() = next;
        *jump = 1;
    }
    return true;
}
