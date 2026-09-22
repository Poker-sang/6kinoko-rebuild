#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"

extern "C" void retdec_trace_i32(const char*, int32_t);

namespace {
using namespace kinoko::script;
bool valid_index(HSQUIRRELVM vm, SQInteger index) noexcept {
    if (!vm || index == 0) return false;
    const auto top = sq_gettop(vm);
    return index > 0 ? index <= top : index >= -top;
}
int32_t payload_word(const void* payload) noexcept {
    int32_t result;
    std::memcpy(&result, payload, sizeof(result));
    return result;
}
template<class T, class Getter>
int32_t argument(HSQUIRRELVM vm, SQInteger index, SQObjectType expected,
                 T* output, Getter get) {
    if (!vm) return 0;
    if (!output || !valid_index(vm, index) || sq_gettype(vm, index) != expected) {
        sq_throwerror(vm, "Incorrect function argument");
        return 0;
    }
    T result{};
    if (SQ_FAILED(get(vm, index, &result))) {
        sq_throwerror(vm, "sq_get*() failed (type error)");
        return 0;
    }
    std::memcpy(output, &result, sizeof(result));
    return 1;
}
} // namespace

extern "C" void* kinoko_native_target_from_userdata(struct SQVM * vm_address) {
    static int trace_count;
    auto* vm = vm_address;
    if (!vm) return (void*)(intptr_t)(0);
    const auto top = sq_gettop(vm);
    if (top <= 0) return (void*)(intptr_t)(0);
    // CallNative appends the captured userdata AFTER all user arguments.
    // Initialize before querying: failed conversion does not write the output.
    SQUserPointer payload = nullptr;
    const auto status = sq_getuserdata(vm, -1, &payload, nullptr);
    const bool readable = SQ_SUCCEEDED(status) && payload &&
        sq_getsize(vm, -1) >= static_cast<SQInteger>(sizeof(int32_t));
    if (trace_count < 96) {
        retdec_trace_i32("native-userdata:vm", address(vm_address));
        retdec_trace_i32("native-userdata:top", top);
        retdec_trace_i32("native-userdata:lookup", status);
        retdec_trace_i32("native-userdata:payload", address(payload));
        const auto bits = static_cast<uint32_t>(address(payload));
        if (readable && bits >= 0x10000u && bits < 0x7f000000u)
            retdec_trace_i32("native-userdata:value", payload_word(payload));
        ++trace_count;
    }
    return (void*)(intptr_t)(readable ? payload_word(payload) : 0);
}
extern "C" void* kinoko_native_callback_from_stack(struct SQVM * vm_address) {
    auto* vm = vm_address;
    if (!vm || sq_gettop(vm) <= 0) return (void*)(intptr_t)(0);
    SQUserPointer payload = nullptr, tag = nullptr;
    if (SQ_FAILED(sq_getuserdata(vm, sq_gettop(vm), &payload, &tag)) || tag || !payload ||
        sq_getsize(vm, -1) < static_cast<SQInteger>(sizeof(int32_t)))
        return (void*)(intptr_t)(0);
    return (void*)(intptr_t)(payload_word(payload));
}
extern "C" int32_t kinoko_native_string_arg(struct SQVM * vm_address, int32_t index, int32_t* output) {
    static int trace_count;
    auto* vm = vm_address;
    if (trace_count < 64) {
        retdec_trace_i32("native-string-arg:vm", address(vm_address));
        retdec_trace_i32("native-string-arg:index", index);
        retdec_trace_i32("native-string-arg:type", valid_index(vm, index) ? sq_gettype(vm, index) : OT_NULL);
        ++trace_count;
    }
    const SQChar* text = nullptr;
    const auto result = argument(vm, index, OT_STRING, output ? &text : nullptr, sq_getstring);
    if (result) {
        const int32_t bits = address(text);
        std::memcpy(output, &bits, sizeof(bits));
    }
    return result;
}
extern "C" int32_t kinoko_native_integer_arg(struct SQVM * vm, int32_t index, int32_t* output) {
    static_assert(sizeof(SQInteger) == sizeof(int32_t));
    SQInteger value = 0;
    const auto result = argument(vm, index, OT_INTEGER,
                                 output ? &value : nullptr, sq_getinteger);
    if (result) std::memcpy(output, &value, sizeof(value));
    return result;
}
extern "C" int32_t kinoko_native_float_arg(struct SQVM * vm, int32_t index, float* output) {
    static_assert(sizeof(SQFloat) == sizeof(float));
    return argument(vm, index, OT_FLOAT, output, sq_getfloat);
}
extern "C" int32_t kinoko_native_value_pair(struct SQVM * vm_address, int32_t index, int32_t* output) {
    auto* vm = vm_address;
    // sq_getstackobj does NOT validate indices in Squirrel 2.2.2.
    if (!output || !vm || index < 1 || index > sq_gettop(vm)) return 0;
    HSQOBJECT value;
    sq_getstackobj(vm, index, &value);
    std::memcpy(output, &value, sizeof(value));
    return 1;
}
extern "C" int32_t kinoko_squirrel_pair_from_stack(struct SQVM * vm_address, int32_t index, int32_t* output) {
    auto* vm = vm_address;
    if (!output || !vm || index < 1 || index > sq_gettop(vm)) return 0;
    ObjectView destination(output);
    destination.initialize(kinoko_squirrel_object_vtable());
    destination.capture(vm, index);
    return 1;
}
