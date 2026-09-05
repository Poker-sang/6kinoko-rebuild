#include "kinoko/squirrel_vm_bridge.h"

#include <stdint.h>
#include <stdlib.h>

#include "sqpcheader.h"
#include "sqfuncproto.h"
#include "sqvm.h"
#include "sqclosure.h"

extern "C" void retdec_trace(const char *message);

namespace {

static_assert(sizeof(void *) == 4,
              "The Squirrel bridge must be compiled for Win32.");
static_assert(sizeof(SQObject) == 8,
              "The bridge expects the original 8-byte SQObject layout.");
static_assert(sizeof(SQObjectPtr) == 8,
              "The bridge expects the original 8-byte SQObjectPtr layout.");
static_assert(sizeof(SQVM) == 168,
              "The bridge expects the Squirrel 2.2.2 SQVM layout.");

uintptr_t raw_address(int32_t value) {
    return static_cast<uintptr_t>(static_cast<uint32_t>(value));
}

bool plausible_address(uintptr_t value) {
    return value != 0 && (value & (sizeof(uint32_t) - 1)) == 0 &&
           value >= 0x10000u && value < 0x70000000u;
}

bool explicit_execution_enabled() {
    const char *value = std::getenv("KINOKO_SQUIRREL_CPP_EXECUTE");
    return value != nullptr && value[0] == '1' && value[1] == '\0';
}

template <typename T>
bool valid_vector_storage(sqvector<T> &values) {
    if (values.size() > values.capacity())
        return false;
    return values.size() == 0 || plausible_address(
        reinterpret_cast<uintptr_t>(values._vals));
}

bool valid_vm_shape(SQVM *vm, int32_t target, int32_t nargs,
                    int32_t stackbase) {
    if (vm == nullptr || vm->_sharedstate == nullptr ||
        vm->_stack._vals == nullptr)
        return false;

    const SQUnsignedInteger stack_size = vm->_stack.size();
    if (stack_size == 0 || stack_size > 0x01000000u ||
        stack_size > vm->_stack.capacity())
        return false;
    if (!valid_vector_storage(vm->_vargsstack))
        return false;
    if (vm->_top < 0 || static_cast<SQUnsignedInteger>(vm->_top) > stack_size)
        return false;
    if (vm->_stackbase < 0 || vm->_stackbase > vm->_top)
        return false;
    if (nargs < 0 || nargs > vm->_top)
        return false;
    if (stackbase < 0 || stackbase > vm->_top)
        return false;
    if (target < -1 || target > vm->_top)
        return false;

    if (vm->_callsstacksize < 0 || vm->_alloccallsstacksize < 0 ||
        vm->_callsstacksize > vm->_alloccallsstacksize ||
        vm->_alloccallsstacksize > 0x00010000 ||
        vm->_callstackdata.size() <
            static_cast<SQUnsignedInteger>(vm->_alloccallsstacksize) ||
        vm->_callstackdata.size() > vm->_callstackdata.capacity())
        return false;
    if (vm->_alloccallsstacksize != 0 &&
        (!valid_vector_storage(vm->_callstackdata) ||
         !plausible_address(reinterpret_cast<uintptr_t>(vm->_callsstack)) ||
         vm->_callsstack != &vm->_callstackdata[0]))
        return false;
    if (vm->_callsstacksize == 0) {
        if (vm->ci != nullptr)
            return false;
    } else {
        const uintptr_t expected_ci = reinterpret_cast<uintptr_t>(
            vm->_callsstack + (vm->_callsstacksize - 1));
        if (vm->ci == nullptr ||
            reinterpret_cast<uintptr_t>(vm->ci) != expected_ci)
            return false;
    }
    if (vm->_nnativecalls < 0 || vm->_nnativecalls > 100)
        return false;
    return true;
}

bool valid_closure_shape(const SQObjectPtr &closure) {
    if (closure._type != OT_CLOSURE || closure._unVal.pClosure == nullptr)
        return false;
    const SQClosure *raw_closure = closure._unVal.pClosure;
    if (!plausible_address(reinterpret_cast<uintptr_t>(raw_closure)))
        return false;
    if (raw_closure->_function._type != OT_FUNCPROTO ||
        raw_closure->_function._unVal.pFunctionProto == nullptr)
        return false;

    const SQFunctionProto *proto =
        raw_closure->_function._unVal.pFunctionProto;
    if (!plausible_address(reinterpret_cast<uintptr_t>(proto)) ||
        proto->_ninstructions <= 0 || proto->_ninstructions > 0x01000000 ||
        !plausible_address(reinterpret_cast<uintptr_t>(proto->_instructions)))
        return false;
    if (proto->_nliterals < 0 || proto->_nliterals > 0x01000000 ||
        (proto->_nliterals != 0 &&
         !plausible_address(reinterpret_cast<uintptr_t>(proto->_literals))))
        return false;
    if (proto->_stacksize < 0 || proto->_stacksize > 0x01000000)
        return false;
    return true;
}

} // namespace

extern "C" int32_t retdec_squirrel_execute_cpp(
    int32_t vm_value,
    int32_t closure_value,
    int32_t target,
    int32_t nargs,
    int32_t stackbase,
    int32_t outres_value,
    int32_t raiseerror,
    int32_t execution_type) {
    static int32_t trace_count;
    /* Mixed C/C++ object ownership is intentionally opt-in while the
       Squirrel migration is being staged.  A CMake experiment must not
       silently change the startup path used by the known-good build. */
    if (!explicit_execution_enabled())
        return RETDEC_SQUIRREL_CPP_UNSUPPORTED;

    if (trace_count < 128) {
        retdec_trace("cpp-execute:enabled");
        ++trace_count;
    }

    const uintptr_t vm_address = raw_address(vm_value);
    const uintptr_t closure_address = raw_address(closure_value);
    const uintptr_t outres_address = raw_address(outres_value);

    /* The caller supplies addresses from a 32-bit decompilation.  Refuse the
       handoff before dereferencing anything if the shape is not plausible. */
    if (!plausible_address(vm_address) ||
        !plausible_address(closure_address) ||
        !plausible_address(outres_address))
        return RETDEC_SQUIRREL_CPP_UNSUPPORTED;

    SQVM *vm = reinterpret_cast<SQVM *>(vm_address);
    SQObjectPtr *closure = reinterpret_cast<SQObjectPtr *>(closure_address);
    SQObjectPtr *outres = reinterpret_cast<SQObjectPtr *>(outres_address);

    if (!valid_vm_shape(vm, target, nargs, stackbase)) {
        if (trace_count < 128)
            retdec_trace("cpp-execute:unsupported-vm-shape");
        return RETDEC_SQUIRREL_CPP_UNSUPPORTED;
    }
    if (!valid_closure_shape(*closure)) {
        if (trace_count < 128)
            retdec_trace("cpp-execute:unsupported-closure-shape");
        return RETDEC_SQUIRREL_CPP_UNSUPPORTED;
    }
    if (execution_type < SQVM::ET_CALL ||
        execution_type > SQVM::ET_RESUME_VM)
        return RETDEC_SQUIRREL_CPP_UNSUPPORTED;

    const SQVM::ExecutionType et =
        static_cast<SQVM::ExecutionType>(execution_type);
    if (trace_count < 128)
        retdec_trace("cpp-execute:accepted");
    const bool result = vm->Execute(
        *closure,
        static_cast<SQInteger>(target),
        static_cast<SQInteger>(nargs),
        static_cast<SQInteger>(stackbase),
        *outres,
        raiseerror != 0 ? SQTrue : SQFalse,
        et);
    if (trace_count < 128)
        retdec_trace(result ? "cpp-execute:success" : "cpp-execute:failure");
    return result ? 1 : 0;
}
