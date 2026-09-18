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
struct ResolvedMethod { int32_t receiver = 0; Method method{}; };
bool resolve(int32_t vm_address, ResolvedMethod& output) {
    int32_t result[2]{};
    function_460540_this(address(result), vm_address);
    if (!result[0] || !result[1]) return false;
    output.receiver = result[0];
    output.method = load<Method>(result[1]);
    return true;
}
int32_t instance_error(int32_t vm_address) {
    auto* vm = pointer<SQVM>(vm_address);
    return vm ? sq_throwerror(vm, "Invalid Instance Type") : -1;
}
void* receiver(const ResolvedMethod& method) {
    return pointer(add_address(method.receiver, method.method.receiver_offset));
}
} // namespace

extern "C" int32_t function_45f560(int32_t vm_address, int32_t index) {
    auto* vm = pointer<SQVM>(vm_address);
    SQInteger value = 0;
    if (!valid_index(vm, index)) { argument_error(vm); return 0; }
    sq_getinteger(vm, index, &value);
    // The generated exception stub is a no-op. Never return an uninitialized
    // stack word after a failed conversion; preserve the source error instead.
    return value;
}
extern "C" long double function_45f5a0(int32_t vm_address, int32_t index) {
    auto* vm = pointer<SQVM>(vm_address);
    SQFloat value = 0;
    if (!valid_index(vm, index)) { argument_error(vm); return 0; }
    sq_getfloat(vm, index, &value);
    return value;
}
extern "C" int32_t function_45f5e0(int32_t* target, int32_t unused, int32_t vm) {
    return function_45f5e0_at(target, unused, vm, 2);
}
extern "C" int32_t function_45f5e0_at(int32_t* target, int32_t, int32_t vm, int32_t index) {
    return retdec_squirrel_pair_from_stack(vm, index, target) ? address(target) : 0;
}
extern "C" int32_t function_45f850(int32_t object, int32_t method, int32_t offset,
                                    int32_t vm_address, int32_t index) {
    auto* vm = pointer<SQVM>(vm_address);
    if (!exact_arguments(vm, index, OT_INTEGER, 1)) return argument_error(vm);
    const auto value = function_45f560(vm_address, index);
    if (method) retdec_call_thiscall1(pointer(add_address(object, offset)), pointer(method), value);
    return 0;
}
extern "C" int32_t function_45f8c0(int32_t object, int32_t method, int32_t offset,
                                    int32_t vm_address, int32_t index) {
    auto* vm = pointer<SQVM>(vm_address);
    if (!exact_arguments(vm, index, OT_FLOAT, 4)) return argument_error(vm);
    int32_t bits[4];
    for (int i = 3; i >= 0; --i) {
        const auto value = static_cast<float>(function_45f5a0(vm_address, index + i));
        bits[i] = load<int32_t>(&value);
    }
    const auto result = retdec_call_thiscall4_result(pointer(add_address(object, offset)),
        pointer(method), bits[0], bits[1], bits[2], bits[3]);
    // Preserve the original low-byte BOOL result, not result != 0.
    sq_pushbool(vm, result & 255);
    return 1;
}
extern "C" int32_t function_45f9d0(int32_t object, int32_t method, int32_t offset,
                                    int32_t vm_address, int32_t index) {
    auto* vm = pointer<SQVM>(vm_address);
    if (!exact_arguments(vm, index, OT_FLOAT, 2)) return argument_error(vm);
    const auto second = static_cast<float>(function_45f5a0(vm_address, index + 1));
    const auto first = static_cast<float>(function_45f5a0(vm_address, index));
    if (method) retdec_call_thiscall2_result(pointer(add_address(object, offset)), pointer(method),
        load<int32_t>(&first), load<int32_t>(&second));
    return 0;
}

extern "C" int32_t function_460540_this(int32_t output_address, int32_t vm_address) {
    if (!output_address || !vm_address) return 0;
    auto* vm = pointer<SQVM>(vm_address);
    std::array<int32_t, 2> result{};
    store(output_address, result);
    const auto top = sq_gettop(vm);
    if (top < 1) return output_address;
    SQUserPointer native = nullptr;
    if (SQ_SUCCEEDED(sq_getinstanceup(vm, 1, &native, nullptr))) result[0] = address(native);
    SQUserPointer payload = nullptr, tag = nullptr;
    // Captured method data follows ALL caller arguments, not a fixed slot.
    if (SQ_FAILED(sq_getuserdata(vm, top, &payload, &tag)) || tag || !payload ||
        sq_getsize(vm, top) < static_cast<SQInteger>(sizeof(Method))) {
        store(output_address, result);
        return output_address;
    }
    result[1] = address(payload);
    HSQOBJECT self{};
    sq_getstackobj(vm, 1, &self);
    SQUserPointer actual_type = nullptr;
    sq_getobjtypetag(&self, &actual_type);
    auto* expected_type = kinoko_native_binding_type(-1);
    if (actual_type != expected_type) {
        StackTop stack(vm);
        sq_pushobject(vm, self);
        sq_pushstring(vm, "__ot", -1);
        result[0] = 0;
        if (SQ_SUCCEEDED(sq_get(vm, -2))) {
            sq_pushinteger(vm, address(expected_type));
            if (SQ_SUCCEEDED(sq_get(vm, -2))) {
                SQUserPointer mapped = nullptr;
                if (SQ_SUCCEEDED(sq_getuserpointer(vm, -1, &mapped))) result[0] = address(mapped);
            }
        }
    }
    // No VM-offset inspection, nor diagnostic reads beyond the 8-byte Method.
    store(output_address, result);
    return output_address;
}
extern "C" int32_t function_460540(int32_t vm) {
    static int32_t legacy_result[2];
    return function_460540_this(address(legacy_result), vm);
}
extern "C" int32_t function_460b00(int32_t vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    retdec_call_thiscall0(receiver(method), pointer(method.method.function));
    return 0;
}
extern "C" int32_t function_460b50(int32_t vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    if (sq_gettop(pointer<SQVM>(vm)) < 3) return argument_error(pointer<SQVM>(vm));
    int32_t arguments[3]{};
    if (!function_45f5e0_at(arguments, 0, vm, 2)) return argument_error(pointer<SQVM>(vm));
    // This owning 12-byte value is passed by value. The original native
    // callee consumes it; a caller-side RAII release would be a double release.
    retdec_call_thiscall3_result(receiver(method), pointer(method.method.function),
        arguments[0], arguments[1], arguments[2]);
    return 0;
}
extern "C" int32_t function_460bc0(int32_t vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    return function_45f850(method.receiver, method.method.function, method.method.receiver_offset, vm, 2);
}
extern "C" int32_t function_460c10(int32_t vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    sq_pushinteger(pointer<SQVM>(vm), retdec_call_thiscall0_result(receiver(method), pointer(method.method.function)));
    return 1;
}
extern "C" int32_t function_460c70(int32_t vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    return function_45f8c0(method.receiver, method.method.function, method.method.receiver_offset, vm, 2);
}
extern "C" int32_t function_460cc0(int32_t vm) {
    ResolvedMethod method;
    if (!resolve(vm, method)) return instance_error(vm);
    return function_45f9d0(method.receiver, method.method.function, method.method.receiver_offset, vm, 2);
}
