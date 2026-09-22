#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/legacy_abi.h"

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;

bool exact_arguments(HSQUIRRELVM vm, SQInteger index, SQObjectType expected, int count) {
    if (!vm || count <= 0) return false;
    // Require every index before calling an unchecked source getter. Use a
    // wide addition to make even a malformed INT_MAX index deterministic.
    for (int i = 0; i < count; ++i) {
        const auto current = static_cast<int64_t>(index) + i;
        if (current > INT32_MAX || current < INT32_MIN ||
            !valid_index(vm, static_cast<SQInteger>(current)) ||
            sq_gettype(vm, static_cast<SQInteger>(current)) != expected) return false;
    }
    return true;
}
int32_t argument_error(HSQUIRRELVM vm) {
    return vm ? sq_throwerror(vm, "Incorrect function argument") : -1;
}
struct MethodTarget { void* receiver; const Method* payload; };
static_assert(sizeof(MethodTarget)==8);
struct ResolvedMethod { void* receiver = nullptr; void* function = nullptr; int32_t offset = 0; };
bool resolve(SQVM* vm, ResolvedMethod& output) {
    MethodTarget result{};
    kinoko_sqplus_resolve_method(&result, vm);
    if (!result.receiver || !result.payload) return false;
    const auto method=load<Method>(result.payload);
    output={result.receiver,pointer(method.function),method.receiver_offset};
    return true;
}
int32_t instance_error(SQVM* vm) {
    return vm ? sq_throwerror(vm, "Invalid Instance Type") : -1;
}
void* receiver(const ResolvedMethod& method) {
    return static_cast<unsigned char*>(method.receiver)+method.offset;
}
} // namespace

extern "C" int32_t kinoko_sqplus_argument_integer(struct SQVM * vm_address, int32_t index) {
    auto* vm = static_cast<SQVM *>(vm_address);
    SQInteger value = 0;
    if (!valid_index(vm, index)) { argument_error(vm); return 0; }
    sq_getinteger(vm, index, &value);
    // The generated exception stub is a no-op. Never return an uninitialized
    // stack word after a failed conversion; preserve the source error instead.
    return value;
}
extern "C" long double  kinoko_sqplus_argument_float(struct SQVM * vm_address, int32_t index) {
    auto* vm = static_cast<SQVM *>(vm_address);
    SQFloat value = 0;
    if (!valid_index(vm, index)) { argument_error(vm); return 0; }
    sq_getfloat(vm, index, &value);
    return value;
}
extern "C" int32_t kinoko_sqplus_argument_object(int32_t* target, int32_t unused, struct SQVM * vm) {
    return kinoko_sqplus_argument_object_at(target, unused, vm, 2);
}
extern "C" int32_t kinoko_sqplus_argument_object_at(int32_t* target, int32_t, struct SQVM * vm, int32_t index) {
    return retdec_squirrel_pair_from_stack(address(vm), index, target) ? address(target) : 0;
}
extern "C" int32_t kinoko_sqplus_call_integer(void * object, void * method, int32_t offset, struct SQVM * vm_address, int32_t index) {
    auto* vm = static_cast<SQVM *>(vm_address);
    if (!exact_arguments(vm, index, OT_INTEGER, 1)) return argument_error(vm);
    const auto value = kinoko_sqplus_argument_integer(vm_address, index);
    if (method) retdec_call_thiscall1(static_cast<unsigned char*>(object)+offset, method, value);
    return 0;
}
extern "C" int32_t kinoko_sqplus_call_rectangle(void * object, void * method, int32_t offset, struct SQVM * vm_address, int32_t index) {
    auto* vm = static_cast<SQVM *>(vm_address);
    if (!exact_arguments(vm, index, OT_FLOAT, 4)) return argument_error(vm);
    int32_t bits[4];
    for (int i = 3; i >= 0; --i) {
        const auto value = static_cast<float>(kinoko_sqplus_argument_float(vm_address, index + i));
        bits[i] = load<int32_t>(&value);
    }
    const auto result = retdec_call_thiscall4_result(static_cast<unsigned char*>(object)+offset,
        method, bits[0], bits[1], bits[2], bits[3]);
    // Preserve the original low-byte BOOL result, not result != 0.
    sq_pushbool(vm, result & 255);
    return 1;
}
extern "C" int32_t kinoko_sqplus_call_move(void * object, void * method, int32_t offset, struct SQVM * vm_address, int32_t index) {
    auto* vm = static_cast<SQVM *>(vm_address);
    if (!exact_arguments(vm, index, OT_FLOAT, 2)) return argument_error(vm);
    const auto second = static_cast<float>(kinoko_sqplus_argument_float(vm_address, index + 1));
    const auto first = static_cast<float>(kinoko_sqplus_argument_float(vm_address, index));
    if (method) retdec_call_thiscall2_result(static_cast<unsigned char*>(object)+offset, method,
        load<int32_t>(&first), load<int32_t>(&second));
    return 0;
}

extern "C" void * kinoko_sqplus_resolve_method(void * output_address, struct SQVM * vm_address) {
    if (!output_address || !vm_address) return 0;
    auto* vm = static_cast<SQVM *>(vm_address);
    MethodTarget result{};
    store(output_address, result);
    const auto top = sq_gettop(vm);
    if (top < 1) return output_address;
    SQUserPointer native = nullptr;
    if (SQ_SUCCEEDED(sq_getinstanceup(vm, 1, &native, nullptr))) result.receiver = native;
    SQUserPointer payload = nullptr, tag = nullptr;
    // Captured method data follows ALL caller arguments, not a fixed slot.
    if (SQ_FAILED(sq_getuserdata(vm, top, &payload, &tag)) || tag || !payload ||
        sq_getsize(vm, top) < static_cast<SQInteger>(sizeof(Method))) {
        store(output_address, result);
        return output_address;
    }
    result.payload = static_cast<const Method*>(payload);
    HSQOBJECT self{};
    sq_getstackobj(vm, 1, &self);
    SQUserPointer actual_type = nullptr;
    sq_getobjtypetag(&self, &actual_type);
    auto* expected_type = kinoko_native_binding_type(-1);
    if (actual_type != expected_type) {
        StackTop stack(vm);
        sq_pushobject(vm, self);
        sq_pushstring(vm, "__ot", -1);
        result.receiver = nullptr;
        if (SQ_SUCCEEDED(sq_get(vm, -2))) {
            sq_pushinteger(vm, address(expected_type));
            if (SQ_SUCCEEDED(sq_get(vm, -2))) {
                SQUserPointer mapped = nullptr;
                if (SQ_SUCCEEDED(sq_getuserpointer(vm, -1, &mapped))) result.receiver = mapped;
            }
        }
    }
    // No VM-offset inspection, nor diagnostic reads beyond the 8-byte Method.
    store(output_address, result);
    return output_address;
}
extern "C" int32_t kinoko_sqplus_resolve_method_compat(struct SQVM * vm) {
    static int32_t legacy_result[2];
    return (int32_t)(intptr_t)(kinoko_sqplus_resolve_method(legacy_result, vm));
}
extern "C" int32_t kinoko_sqplus_void_method(struct SQVM * vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    retdec_call_thiscall0(receiver(method), method.function);
    return 0;
}
extern "C" int32_t kinoko_sqplus_object_method(struct SQVM * vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    if (sq_gettop(static_cast<SQVM *>(vm)) < 3) return argument_error(static_cast<SQVM *>(vm));
    int32_t arguments[3]{};
    if (!kinoko_sqplus_argument_object_at(arguments, 0, vm, 2)) return argument_error(static_cast<SQVM *>(vm));
    // This owning 12-byte value is passed by value. The original native
    // callee consumes it; a caller-side RAII release would be a double release.
    retdec_call_thiscall3_result(receiver(method), method.function,
        arguments[0], arguments[1], arguments[2]);
    return 0;
}
extern "C" int32_t kinoko_sqplus_integer_method(struct SQVM * vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    return kinoko_sqplus_call_integer(method.receiver, method.function, method.offset, vm, 2);
}
extern "C" int32_t kinoko_sqplus_integer_result_method(struct SQVM * vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    sq_pushinteger(static_cast<SQVM *>(vm), retdec_call_thiscall0_result(receiver(method), method.function));
    return 1;
}
extern "C" int32_t kinoko_sqplus_rectangle_method(struct SQVM * vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    return kinoko_sqplus_call_rectangle(method.receiver, method.function, method.offset, vm, 2);
}
extern "C" int32_t kinoko_sqplus_move_method(struct SQVM * vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    return kinoko_sqplus_call_move(method.receiver, method.function, method.offset, vm, 2);
}
