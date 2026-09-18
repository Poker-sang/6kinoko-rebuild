#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"

extern "C" void retdec_trace_i32(const char*, int32_t);

namespace {
using namespace kinoko::script;
int32_t payload_word(const void* payload) noexcept {
    int32_t result;
    std::memcpy(&result, payload, sizeof(result));
    return result;
}
template<class T, class Getter>
int32_t argument(HSQUIRRELVM vm, SQInteger index, SQObjectType expected,
                 T* output, Getter get) {
    if (!output || sq_gettype(vm, index) != expected) {
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

extern "C" int32_t retdec_native_target_from_userdata(int32_t vm_address) {
    static int trace_count;
    auto* vm = pointer<SQVM>(vm_address);
    if (!vm) return 0;
    const auto top = sq_gettop(vm);
    if (top <= 0) return 0;
    // CallNative appends the captured userdata AFTER all user arguments.
    // Initialize before querying: failed conversion does not write the output.
    SQUserPointer payload = nullptr;
    const auto status = sq_getuserdata(vm, -1, &payload, nullptr);
    if (trace_count < 96) {
        retdec_trace_i32("native-userdata:vm", vm_address);
        retdec_trace_i32("native-userdata:top", top);
        retdec_trace_i32("native-userdata:lookup", status);
        retdec_trace_i32("native-userdata:payload", address(payload));
        const auto bits = static_cast<uint32_t>(address(payload));
        if (SQ_SUCCEEDED(status) && bits >= 0x10000u && bits < 0x7f000000u)
            retdec_trace_i32("native-userdata:value", payload_word(payload));
        ++trace_count;
    }
    return SQ_SUCCEEDED(status) && payload ? payload_word(payload) : 0;
}
extern "C" int32_t retdec_native_callback_from_stack(int32_t vm_address) {
    auto* vm = pointer<SQVM>(vm_address);
    if (!vm || sq_gettop(vm) <= 0) return 0;
    SQUserPointer payload = nullptr, tag = nullptr;
    if (SQ_FAILED(sq_getuserdata(vm, sq_gettop(vm), &payload, &tag)) || tag || !payload)
        return 0;
    return payload_word(payload);
}
extern "C" int32_t retdec_native_string_arg(int32_t vm_address, int32_t index, int32_t* output) {
    static int trace_count;
    auto* vm = pointer<SQVM>(vm_address);
    if (trace_count < 64) {
        retdec_trace_i32("native-string-arg:vm", vm_address);
        retdec_trace_i32("native-string-arg:index", index);
        retdec_trace_i32("native-string-arg:type", sq_gettype(vm, index));
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
extern "C" int32_t retdec_native_integer_arg(int32_t vm, int32_t index, int32_t* output) {
    static_assert(sizeof(SQInteger) == sizeof(int32_t));
    SQInteger value = 0;
    const auto result = argument(pointer<SQVM>(vm), index, OT_INTEGER,
                                 output ? &value : nullptr, sq_getinteger);
    if (result) std::memcpy(output, &value, sizeof(value));
    return result;
}
extern "C" int32_t retdec_native_float_arg(int32_t vm, int32_t index, float* output) {
    static_assert(sizeof(SQFloat) == sizeof(float));
    return argument(pointer<SQVM>(vm), index, OT_FLOAT, output, sq_getfloat);
}
extern "C" int32_t retdec_native_value_pair(int32_t vm_address, int32_t index, int32_t* output) {
    auto* vm = pointer<SQVM>(vm_address);
    // sq_getstackobj does NOT validate indices in Squirrel 2.2.2.
    if (!output || !vm || index < 1 || index > sq_gettop(vm)) return 0;
    HSQOBJECT value;
    sq_getstackobj(vm, index, &value);
    std::memcpy(output, &value, sizeof(value));
    return 1;
}
extern "C" int32_t retdec_squirrel_pair_from_stack(int32_t vm_address, int32_t index, int32_t* output) {
    auto* vm = pointer<SQVM>(vm_address);
    if (!output || !vm || index < 1 || index > sq_gettop(vm)) return 0;
    ObjectView destination(output);
    destination.initialize(kinoko_squirrel_object_vtable());
    destination.capture(vm, index);
    return 1;
}
