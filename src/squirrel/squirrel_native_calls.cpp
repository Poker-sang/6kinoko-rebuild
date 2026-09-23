#include "kinoko/squirrel_native_calls.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/legacy_abi.h"
#include <array>

extern "C" {
extern char g560;
void retdec_trace_i32(const char*, int32_t);
}

namespace {
using namespace kinoko::script;
constexpr const char* argument_error = "Incorrect function argument";
constexpr const char* conversion_error = "sq_get*() failed (type error)";
int32_t error(HSQUIRRELVM vm, const char* message) {
    return vm ? sq_throwerror(vm, message) : -1;
}
bool index_exists(HSQUIRRELVM vm, int64_t index) {
    if (!vm || index == 0) return false;
    const auto top = sq_gettop(vm);
    return index > 0 ? index <= top : index >= -static_cast<int64_t>(top);
}
bool strict_type(HSQUIRRELVM vm, int64_t index, SQObjectType expected) {
    return index_exists(vm, index) && sq_gettype(vm, static_cast<SQInteger>(index)) == expected;
}
template<size_t N>
SQRESULT integers(HSQUIRRELVM vm, int32_t first, std::array<SQInteger, N>& values) {
    for (size_t i = 0; i < N; ++i) {
        const int64_t index = static_cast<int64_t>(first) + static_cast<int64_t>(i);
        if (!strict_type(vm, index, OT_INTEGER)) return error(vm, argument_error);
        if (SQ_FAILED(sq_getinteger(vm, static_cast<SQInteger>(index), &values[i])))
            return error(vm, conversion_error);
    }
    return SQ_OK;
}
SQRESULT string_argument(HSQUIRRELVM vm, int32_t index, const SQChar*& value) {
    if (!strict_type(vm, index, OT_STRING)) return error(vm, argument_error);
    if (SQ_FAILED(sq_getstring(vm, index, &value))) return error(vm, conversion_error);
    return SQ_OK;
}
bool pair_argument(HSQUIRRELVM vm, int64_t index, HSQOBJECT& value) {
    // This recovered helper accepts positive argument indices only.
    return index > 0 && index_exists(vm, index) &&
        SQ_SUCCEEDED(sq_getstackobj(vm, static_cast<SQInteger>(index), &value));
}
// One pointer-sized word in captured userdata. Some families allow a tag;
// the cdecl binding family deliberately accepts only untagged captures.
SQUserPointer capture(HSQUIRRELVM vm, bool untagged) {
    if (!vm || sq_gettop(vm) < 1 || sq_gettype(vm, -1) != OT_USERDATA ||
        sq_getsize(vm, -1) < static_cast<SQInteger>(sizeof(int32_t))) return nullptr;
    SQUserPointer payload = nullptr, tag = nullptr;
    if (SQ_FAILED(sq_getuserdata(vm, -1, &payload, &tag)) || (untagged && tag)) return nullptr;
    return payload;
}
int32_t word(const void* bytes) {
    int32_t result = 0;
    if (bytes) std::memcpy(&result, bytes, sizeof(result));
    return result;
}
int32_t native_self(HSQUIRRELVM vm) {
    SQUserPointer self = nullptr;
    if (!index_exists(vm, 1) || SQ_FAILED(sq_getinstanceup(vm, 1, &self, nullptr))) return 0;
    return address(self);
}
int32_t target(HSQUIRRELVM vm) { return word(capture(vm, true)); }

// These POD records are passed by value in the original Win32 ABI. The callee
// CONSUMES their external handles, so they must not have an RAII destructor.
ObjectStorage transfer(HSQUIRRELVM vm, HSQOBJECT value) {
    sq_addref(vm, &value);
    return {static_cast<uint32_t>(kinoko_squirrel_object_vtable()), value};
}
using PairCallback = int32_t (__cdecl *)(int32_t, ObjectStorage, ObjectStorage);
int32_t call_pair(HSQUIRRELVM vm, int32_t callback, int32_t first,
                 HSQOBJECT closure, HSQOBJECT environment) {
    // Match construction order as well as stack argument order.
    const auto environment_argument = transfer(vm, environment);
    const auto closure_argument = transfer(vm, closure);
    reinterpret_cast<PairCallback>(pointer(callback))(first, closure_argument, environment_argument);
    return 0;
}
void kinoko_native_capture_receiver(HSQUIRRELVM vm, int32_t* pair);
using ExplicitReceiver = int32_t (*)(int32_t, int32_t, int32_t, int32_t);
int32_t kinoko_native_call_binding(int32_t id, ExplicitReceiver invoke) {
    int32_t pair[2]{};
    kinoko_native_capture_receiver(pointer<SQVM>(id), pair);
    auto vm = pointer<SQVM>(id);
    if (!pair[0] || !pair[1]) return error(vm, "Invalid Instance Type");
    return invoke(pair[0], word(pointer(pair[1])), id, 2);
}
int32_t kinoko_native_property_dispatch(int32_t id, bool write) {
    auto vm = pointer<SQVM>(id);
    // [instance, key, (value), captured lookup table]. Never index a short frame.
    if (!vm || sq_gettop(vm) < (write ? 4 : 3)) return error(vm, "Member Variable not found");
    return upstream::sqrat_property_dispatch(vm, write, static_cast<SQBool>(g560),
        [](HSQUIRRELVM machine, SQInteger count, SQBool result, SQBool raiseerror) -> SQRESULT {
            return kinoko_sq_call(address(machine), count, result, raiseerror);
        });
}
}

extern "C" int32_t function_41e260(int32_t vm) { return kinoko_native_property_dispatch(vm, false); }
extern "C" int32_t function_41e2c0(int32_t vm) { return kinoko_native_property_dispatch(vm, true); }
namespace {
int32_t kinoko_native_weakref(HSQUIRRELVM vm) {
    if (!vm || sq_gettop(vm) < 1) return -1;
    return upstream::sqrat_weakref(vm);
}
}
extern "C" int32_t function_431650(int32_t id) {
    return kinoko_native_weakref(pointer<SQVM>(id));
}
namespace {
int32_t kinoko_native_invoke_integer_member(HSQUIRRELVM machine) {
    auto vm=machine;
    if (!vm || sq_gettop(vm) < 3) return 0;
    const auto method = word(capture(vm, false));
    const auto self = native_self(vm);
    SQInteger argument = 0;
    if (!method || !self || SQ_FAILED(sq_getinteger(vm, 2, &argument))) return 0;
    retdec_call_thiscall1(pointer(self), pointer(method), argument);
    return 0;
}
} // namespace
extern "C" int32_t function_445730(int32_t id) {
    return kinoko_native_invoke_integer_member(pointer<SQVM>(id));
}
namespace {
int32_t kinoko_native_invoke_nullary_member(HSQUIRRELVM machine) {
    auto vm=machine;
    retdec_trace_i32("450950:zero-wrapper-entry", address(machine));
    if (!vm || sq_gettop(vm) < 2) return 0;
    const auto method = word(capture(vm, false));
    const auto self = native_self(vm);
    if (!method || !self) return 0;
    retdec_trace_i32("450950:zero-wrapper-method", method);
    retdec_trace_i32("450950:zero-wrapper-instance", self);
    retdec_call_thiscall0(pointer(self), pointer(method));
    return 0;
}
} // namespace
extern "C" int32_t function_4552e0(int32_t id) {
    return kinoko_native_invoke_nullary_member(pointer<SQVM>(id));
}
namespace {
int32_t kinoko_native_invoke_draw_member(HSQUIRRELVM machine) {
    auto vm=machine;
    if (!vm || sq_gettop(vm) < 11) return -1;
    const auto method = word(capture(vm, false));
    const auto self = native_self(vm);
    if (!method || !self) return -1;
    SQInteger x=0, y=0, width=0, height=0, sx=0, sy=0, blend=0;
    SQFloat alpha=0;
    SQUserPointer resource=nullptr;
    // Preserve both the default-zero conversions and their observable order.
    sq_getfloat(vm, 10, &alpha);
    sq_getinteger(vm, 9, &blend); sq_getinteger(vm, 8, &sy);
    sq_getinteger(vm, 7, &sx); sq_getinstanceup(vm, 6, &resource, nullptr);
    sq_getinteger(vm, 5, &height); sq_getinteger(vm, 4, &width);
    sq_getinteger(vm, 3, &y); sq_getinteger(vm, 2, &x);
    const auto result = kinoko_call_draw_method(pointer(self), pointer(method),
        x, y, width, height, address(resource), sx, sy, blend, alpha);
    sq_pushinteger(vm, result);
    return 1;
}
} // namespace
extern "C" int32_t function_4555a0(int32_t id) {
    return kinoko_native_invoke_draw_member(pointer<SQVM>(id));
}
namespace {
int32_t kinoko_native_invoke_string_callback(int32_t self, int32_t callback, HSQUIRRELVM machine, int32_t first) {
    auto vm=machine; const SQChar* value=nullptr;
    if (SQ_FAILED(string_argument(vm, first, value))) return -1;
    if (!callback) return error(vm, "Invalid native function");
    using Function=int32_t (__cdecl *)(int32_t, const SQChar*);
    reinterpret_cast<Function>(pointer(callback))(self, value);
    return 0;
}
} // namespace
extern "C" int32_t function_46b490(int32_t self, int32_t callback, int32_t id, int32_t first) {
    return kinoko_native_invoke_string_callback(self, callback, pointer<SQVM>(id), first);
}
namespace {
int32_t kinoko_native_invoke_three_integer_callback(int32_t self, int32_t callback, HSQUIRRELVM machine, int32_t first) {
    auto vm=machine; std::array<SQInteger,3> values{};
    if (SQ_FAILED(integers(vm, first, values))) return -1;
    if (!callback) return error(vm, "Invalid native function");
    using Function=int32_t (__cdecl *)(int32_t,int32_t,int32_t,int32_t);
    reinterpret_cast<Function>(pointer(callback))(self, values[0], values[1], values[2]);
    return 0;
}
} // namespace
extern "C" int32_t function_46b500(int32_t self, int32_t callback, int32_t id, int32_t first) {
    return kinoko_native_invoke_three_integer_callback(self, callback, pointer<SQVM>(id), first);
}
namespace {
int32_t kinoko_native_call_two(int32_t self, int32_t callback, int32_t id, int32_t first, bool boolean) {
    auto vm=pointer<SQVM>(id); std::array<SQInteger,2> values{};
    if (SQ_FAILED(integers(vm, first, values))) return -1;
    if (!callback) return error(vm, "Invalid native function");
    using Function=int32_t (__cdecl *)(int32_t,int32_t,int32_t);
    const auto result=reinterpret_cast<Function>(pointer(callback))(self,values[0],values[1]);
    if (boolean) sq_pushbool(vm,result!=0); else sq_pushinteger(vm,result);
    return 1;
}
}
extern "C" int32_t function_46b610(int32_t self,int32_t target,int32_t vm,int32_t first) {
    return kinoko_native_call_two(self,target,vm,first,true);
}
extern "C" int32_t function_46b6f0(int32_t self,int32_t target,int32_t vm,int32_t first) {
    return kinoko_native_call_two(self,target,vm,first,false);
}
namespace {
void kinoko_native_capture_receiver(HSQUIRRELVM vm, int32_t* pair) {
    if (!pair) return;
    const int32_t result[2]={native_self(vm),address(capture(vm,true))};
    std::memcpy(pair,result,sizeof(result));
}
}
extern "C" void function_46c6b0_pair(int32_t id,int32_t* pair) {
    kinoko_native_capture_receiver(pointer<SQVM>(id), pair);
}
extern "C" int32_t function_46ce70(int32_t vm) { return kinoko_native_call_binding(vm,function_46b490); }
extern "C" int32_t function_46cec0(int32_t vm) { return kinoko_native_call_binding(vm,function_46b500); }
extern "C" int32_t function_46cf10(int32_t vm) { return kinoko_native_call_binding(vm,function_46b610); }
extern "C" int32_t function_46cf60(int32_t vm) { return kinoko_native_call_binding(vm,function_46b6f0); }
namespace {
int32_t kinoko_native_invoke_string_only_callback(int32_t callback, HSQUIRRELVM machine, int32_t index) {
    auto vm=machine; const SQChar* value=nullptr;
    if (SQ_FAILED(string_argument(vm,index,value))) return -1;
    if (callback) reinterpret_cast<void (__cdecl *)(const SQChar*)>(pointer(callback))(value);
    return 0;
}
} // namespace
extern "C" int32_t function_4716b0(int32_t callback,int32_t id,int32_t index) {
    return kinoko_native_invoke_string_only_callback(callback, pointer<SQVM>(id), index);
}
namespace {
int32_t kinoko_native_invoke_two_integer_callback(int32_t callback, HSQUIRRELVM machine, int32_t index) {
    auto vm=machine;
    if (!strict_type(vm,index,OT_INTEGER) || !strict_type(vm,static_cast<int64_t>(index)+1,OT_INTEGER))
        return error(vm,argument_error);
    SQInteger first=0,second=0;
    if (SQ_FAILED(sq_getinteger(vm,index+1,&second)) || SQ_FAILED(sq_getinteger(vm,index,&first)))
        return error(vm,conversion_error);
    if (callback) reinterpret_cast<void (__cdecl *)(int32_t,int32_t)>(pointer(callback))(first,second);
    return 0;
}
} // namespace
extern "C" int32_t function_471a60(int32_t callback,int32_t id,int32_t index) {
    return kinoko_native_invoke_two_integer_callback(callback, pointer<SQVM>(id), index);
}
namespace {
int32_t kinoko_native_invoke_string_pair_callback(int32_t callback, HSQUIRRELVM machine, int32_t index) {
    auto vm=machine; const SQChar* name=nullptr; HSQOBJECT closure{},environment{};
    if (!callback || !strict_type(vm,index,OT_STRING) ||
        SQ_FAILED(sq_getstring(vm,index,&name)) ||
        !pair_argument(vm,static_cast<int64_t>(index)+1,closure) ||
        !pair_argument(vm,static_cast<int64_t>(index)+2,environment)) return error(vm,argument_error);
    return call_pair(vm,callback,address(name),closure,environment);
}
} // namespace
extern "C" int32_t function_471720(int32_t callback,int32_t id,int32_t index) {
    return kinoko_native_invoke_string_pair_callback(callback, pointer<SQVM>(id), index);
}
namespace {
int32_t kinoko_native_invoke_integer_pair_callback(HSQUIRRELVM machine) {
    auto vm=machine; const auto callback=target(vm);
    SQInteger key=0; HSQOBJECT closure{},environment{};
    if (!callback || !strict_type(vm,2,OT_INTEGER) || SQ_FAILED(sq_getinteger(vm,2,&key)) ||
        !pair_argument(vm,3,closure) || !pair_argument(vm,4,environment)) return error(vm,argument_error);
    return call_pair(vm,callback,key,closure,environment);
}
} // namespace
extern "C" int32_t function_471d30(int32_t id) {
    return kinoko_native_invoke_integer_pair_callback(pointer<SQVM>(id));
}
namespace {
int32_t kinoko_native_invoke_string_object_callback(HSQUIRRELVM machine) {
    auto vm=machine; const auto callback=target(vm);
    const SQChar* name=nullptr; HSQOBJECT environment{};
    if (!callback || !strict_type(vm,2,OT_STRING) || SQ_FAILED(sq_getstring(vm,2,&name)) ||
        !pair_argument(vm,3,environment)) return error(vm,argument_error);
    using Function=int32_t (__cdecl *)(const SQChar*,ObjectStorage);
    reinterpret_cast<Function>(pointer(callback))(name,transfer(vm,environment));
    return 0;
}
} // namespace
extern "C" int32_t function_471e50(int32_t id) {
    return kinoko_native_invoke_string_object_callback(pointer<SQVM>(id));
}
extern "C" int32_t function_471f70(int32_t id) { return kinoko_native_invoke_string_pair_callback(target(pointer<SQVM>(id)),pointer<SQVM>(id),2); }
namespace {
int32_t kinoko_native_invoke_integer_result_callback(HSQUIRRELVM machine) {
    auto vm=machine; if (!vm) return -1;
    const auto callback=target(vm);
    const auto result=callback ? reinterpret_cast<int32_t (__cdecl *)(void)>(pointer(callback))() : 0;
    sq_pushinteger(vm,result); return 1;
}
} // namespace
extern "C" int32_t function_472030(int32_t id) {
    return kinoko_native_invoke_integer_result_callback(pointer<SQVM>(id));
}

namespace {
// 470DF0 accepts any existing Squirrel stack value. NULL is false; bool,
// integer and float use their native conversion; other types are true.
int32_t kinoko_native_truthy(HSQUIRRELVM vm, int32_t index) {
    if (!index_exists(vm, index)) return error(vm, argument_error);
    switch (sq_gettype(vm, index)) {
    case OT_NULL: return 0;
    case OT_BOOL: {
        SQBool value = SQFalse;
        return SQ_FAILED(sq_getbool(vm, index, &value))
            ? error(vm, conversion_error) : value != SQFalse;
    }
    case OT_INTEGER: {
        SQInteger value = 0;
        return SQ_FAILED(sq_getinteger(vm, index, &value))
            ? error(vm, conversion_error) : value != 0;
    }
    case OT_FLOAT: {
        SQFloat value = 0;
        return SQ_FAILED(sq_getfloat(vm, index, &value))
            ? error(vm, conversion_error) : value != 0;
    }
    default: return 1;
    }
}

int32_t kinoko_native_no_arguments(HSQUIRRELVM vm) {
    const auto callback = target(vm);
    if (callback) reinterpret_cast<int32_t (__cdecl *)(void)>(pointer(callback))();
    return 0;
}

int32_t kinoko_native_string_object(int32_t callback, HSQUIRRELVM vm, int32_t index) {
    if (!strict_type(vm, index, OT_STRING)) return error(vm, argument_error);
    HSQOBJECT object{};
    if (!pair_argument(vm, static_cast<int64_t>(index) + 1, object))
        return error(vm, argument_error);
    auto argument = transfer(vm, object); // The callback consumes this reference.
    const SQChar *value = nullptr;
    if (SQ_FAILED(sq_getstring(vm, index, &value))) {
        sq_release(vm, &argument.value);
        return error(vm, conversion_error);
    }
    if (!callback) {
        sq_release(vm, &argument.value);
        return 0;
    }
    using Callback = int32_t (__cdecl *)(const SQChar *, ObjectStorage);
    const auto result = reinterpret_cast<Callback>(pointer(callback))(value, argument);
    sq_pushbool(vm, static_cast<unsigned char>(result) != 0);
    return 1;
}

int32_t kinoko_native_string_bool(int32_t callback, HSQUIRRELVM vm, int32_t index) {
    const SQChar *value = nullptr;
    if (SQ_FAILED(string_argument(vm, index, value))) return -1;
    if (callback) {
        using Callback = int32_t (__cdecl *)(const SQChar *);
        const auto result = reinterpret_cast<Callback>(pointer(callback))(value);
        sq_pushbool(vm, static_cast<unsigned char>(result) != 0);
    }
    return 1;
}

int32_t kinoko_native_string_integer_truth(int32_t callback, HSQUIRRELVM vm,
                                    int32_t index, bool three_integers) {
    const int count = three_integers ? 3 : 2;
    if (!strict_type(vm, index, OT_STRING)) return error(vm, argument_error);
    for (int offset = 1; offset <= count; ++offset)
        if (!strict_type(vm, static_cast<int64_t>(index) + offset, OT_INTEGER))
            return error(vm, argument_error);
    const auto truth_index = static_cast<int64_t>(index) + count + 1;
    if (!index_exists(vm, truth_index)) return error(vm, argument_error);
    const auto truth = kinoko_native_truthy(vm, static_cast<int32_t>(truth_index));
    if (truth < 0) return truth;
    std::array<SQInteger, 3> values{};
    // Original 471880/471960 converts numeric arguments right-to-left.
    for (int offset = count; offset >= 1; --offset)
        if (SQ_FAILED(sq_getinteger(vm, index + offset, &values[offset - 1])))
            return error(vm, conversion_error);
    const SQChar *name = nullptr;
    if (SQ_FAILED(sq_getstring(vm, index, &name)))
        return error(vm, conversion_error);
    if (callback) {
        if (three_integers) {
            using Callback = void (__cdecl *)(const SQChar *, int32_t, int32_t, int32_t, int32_t);
            reinterpret_cast<Callback>(pointer(callback))(
                name, values[0], values[1], values[2], truth);
        } else {
            using Callback = void (__cdecl *)(const SQChar *, int32_t, int32_t, int32_t);
            reinterpret_cast<Callback>(pointer(callback))(
                name, values[0], values[1], truth);
        }
    }
    return 0;
}

int32_t kinoko_native_two_floats(HSQUIRRELVM vm) {
    const auto callback = target(vm);
    if (!callback) return 0;
    if (!strict_type(vm, 2, OT_FLOAT) || !strict_type(vm, 3, OT_FLOAT)) {
        error(vm, argument_error); return 0;
    }
    SQFloat first = 0, second = 0;
    if (SQ_FAILED(sq_getfloat(vm, 2, &first)) ||
        SQ_FAILED(sq_getfloat(vm, 3, &second))) {
        error(vm, conversion_error); return 0;
    }
    reinterpret_cast<int32_t (__cdecl *)(SQFloat, SQFloat)>(pointer(callback))(first, second);
    return 0;
}

int32_t kinoko_native_one_integer(HSQUIRRELVM vm) {
    const auto callback = target(vm);
    if (!callback) return 0;
    if (!strict_type(vm, 2, OT_INTEGER)) {
        error(vm, argument_error); return 0;
    }
    SQInteger value = 0;
    if (SQ_FAILED(sq_getinteger(vm, 2, &value))) {
        error(vm, conversion_error); return 0;
    }
    reinterpret_cast<int32_t (__cdecl *)(int32_t)>(pointer(callback))(value);
    return 0;
}
int32_t kinoko_native_string_object_from_stack(HSQUIRRELVM vm) {
    return kinoko_native_string_object(target(vm), vm, 2);
}
int32_t kinoko_native_string_bool_from_stack(HSQUIRRELVM vm) {
    return kinoko_native_string_bool(target(vm), vm, 2);
}
int32_t kinoko_native_string_two_int_truth_from_stack(HSQUIRRELVM vm) {
    return kinoko_native_string_integer_truth(target(vm), vm, 2, false);
}
int32_t kinoko_native_string_three_int_truth_from_stack(HSQUIRRELVM vm) {
    return kinoko_native_string_integer_truth(target(vm), vm, 2, true);
}
int32_t kinoko_native_string_callback_from_stack(HSQUIRRELVM vm) {
    return kinoko_native_invoke_string_only_callback(target(vm), vm, 2);
}
int32_t kinoko_native_two_int_callback_from_stack(HSQUIRRELVM vm) {
    return kinoko_native_invoke_two_integer_callback(target(vm), vm, 2);
}
} // namespace

extern "C" int32_t function_470df0(int32_t vm, int32_t index) {
    return kinoko_native_truthy(pointer<SQVM>(vm), index);
}
extern "C" int32_t function_470ee0(int32_t vm) {
    return kinoko_native_no_arguments(pointer<SQVM>(vm));
}
extern "C" int32_t function_471160(int32_t callback, int32_t vm, int32_t index) {
    return kinoko_native_string_object(callback, pointer<SQVM>(vm), index);
}
extern "C" int32_t function_471330(int32_t callback, int32_t vm, int32_t index) {
    return kinoko_native_string_bool(callback, pointer<SQVM>(vm), index);
}
extern "C" int32_t function_471880(int32_t callback, int32_t vm, int32_t index) {
    return kinoko_native_string_integer_truth(callback, pointer<SQVM>(vm), index, false);
}
extern "C" int32_t function_471960(int32_t callback, int32_t vm, int32_t index) {
    return kinoko_native_string_integer_truth(callback, pointer<SQVM>(vm), index, true);
}
extern "C" int32_t function_471bc0(int32_t vm) { return kinoko_native_no_arguments(pointer<SQVM>(vm)); }
extern "C" int32_t function_471c10(int32_t vm) {
    return kinoko_native_string_object_from_stack(pointer<SQVM>(vm));
}
extern "C" int32_t function_471d90(int32_t vm) {
    return kinoko_native_string_bool_from_stack(pointer<SQVM>(vm));
}
extern "C" int32_t function_471eb0(int32_t vm) {
    return kinoko_native_two_floats(pointer<SQVM>(vm));
}
extern "C" int32_t function_471f10(int32_t vm) {
    return kinoko_native_string_callback_from_stack(pointer<SQVM>(vm));
}
extern "C" int32_t function_471fd0(int32_t vm) {
    return kinoko_native_one_integer(pointer<SQVM>(vm));
}
extern "C" int32_t function_472080(int32_t vm) {
    return kinoko_native_string_two_int_truth_from_stack(pointer<SQVM>(vm));
}
extern "C" int32_t function_4720e0(int32_t vm) {
    return kinoko_native_string_three_int_truth_from_stack(pointer<SQVM>(vm));
}
extern "C" int32_t function_472140(int32_t vm) {
    return kinoko_native_two_int_callback_from_stack(pointer<SQVM>(vm));
}
