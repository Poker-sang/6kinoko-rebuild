#include "kinoko/legacy_string.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "squserdata.h"

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;

bool context_has(const int32_t* context, SQInteger count) {
    return context && context[1] && context[0] >= count &&
        sq_gettop(pointer<SQVM>(context[1])) >= count;
}
int32_t userdata_variable(HSQUIRRELVM vm) {
    if (sq_gettype(vm, -1) != OT_USERDATA ||
        sq_getsize(vm, -1) < static_cast<SQInteger>(sizeof(Variable))) return 0;
    SQUserPointer payload = nullptr;
    if (SQ_FAILED(sq_getuserdata(vm, -1, &payload, nullptr))) return 0;
    return address(payload);
}
int32_t table_access(int32_t vm_address, bool write) {
    auto* vm = pointer<SQVM>(vm_address);
    if (!vm) return -1;
    function_4a8db0(vm_address);
    int32_t context[2] = {static_cast<int32_t>(sq_gettop(vm)), vm_address};
    if (context[0] < 1 || sq_gettype(vm, 1) != OT_TABLE) return -1;
    int32_t metadata = 0;
    const auto status = write ? retdec_get_var_info(&metadata, context) :
        function_4aa5e0(context, &metadata);
    if (status) return status;
    if (!metadata) return -1;
    const auto source = load<Variable>(metadata).offset;
    return write ? retdec_set_var_value(context, metadata, source) :
        retdec_get_var_value(context, metadata, source);
}
int32_t instance_access(int32_t vm_address, bool write) {
    auto* vm = pointer<SQVM>(vm_address);
    if (!vm) return -1;
    function_4a8db0(vm_address);
    int32_t context[2] = {static_cast<int32_t>(sq_gettop(vm)), vm_address};
    int32_t metadata = 0, source = 0;
    if (!retdec_resolve_instance_var(vm_address, context[0], &metadata, &source)) return -1;
    return write ? retdec_set_var_value(context, metadata, source) :
        retdec_get_var_value(context, metadata, source);
}
} // namespace

extern "C" int32_t function_4aa5e0(int32_t* context, int32_t* output) {
    if (!context || !output || !context[1]) return -1;
    auto* vm = pointer<SQVM>(context[1]);
    StackTop stack(vm);
    const char* name = nullptr;
    if (context_has(context, 2)) sq_getstring(vm, 2, &name);
    const auto key = variable_key(name);
    if (context_has(context, 1)) sq_push(vm, 1); else sq_pushnull(vm);
    sq_pushstring(vm, key.data(), -1);
    if (SQ_SUCCEEDED(sq_rawget(vm, -2))) {
        const auto payload = userdata_variable(vm);
        if (payload) { *output = payload; return 0; }
    }
    return sq_throwerror(vm, "getVarInfo: Could not retrieve UserData");
}

extern "C" int32_t retdec_get_var_info(int32_t* output, int32_t* context) {
    if (output) *output = 0;
    if (!context_has(context, 1)) return -1;
    auto* vm = pointer<SQVM>(context[1]);
    const char* name = nullptr;
    if (context_has(context, 2) && SQ_FAILED(sq_getstring(vm, 2, &name))) return -1;
    if (sq_gettype(vm, 1) != OT_TABLE) return -1;
    StackTop stack(vm);
    HSQOBJECT object{}, key{};
    sq_getstackobj(vm, 1, &object);
    sq_pushobject(vm, object);
    const auto text = variable_key(name);
    sq_pushstring(vm, text.data(), -1);
    sq_getstackobj(vm, -1, &key);
    // This path intentionally uses source SQTable::Get rather than sq_rawget:
    // a missing table setter must NOT replace the VM's existing last error.
    SQObjectPtr value;
    if (!_table(object)->Get(key, value) || type(value) != OT_USERDATA ||
        _userdata(value)->_size < static_cast<SQInteger>(sizeof(Variable))) return -1;
    if (output) *output = address(_userdataval(value));
    return 0;
}

extern "C" int32_t retdec_get_var_value(int32_t* context, int32_t metadata, int32_t source) {
    if (!context || !context[1] || !metadata) return -1;
    auto* vm = pointer<SQVM>(context[1]);
    const auto info = load<Variable>(metadata);
    const bool immediate = (info.flags & Constant) != 0;
    switch (info.category) {
    case 0: case 1: case 2: case 3:
        return static_cast<int32_t>(upstream::sqplus_read_scalar(vm, info,
            pointer<const void>(source), source));
    case 4:
        if (!source) return -1;
        sq_pushstring(vm, pointer<const char>(immediate ? source : load<int32_t>(source)), -1);
        return 1;
    case 5:
        if (!source) return -1;
        sq_pushstring(vm, pointer<const char>(add_address(source, 1)), -1);
        return 1;
    case 8:
        if (!source) return -1;
        // This is the original 24-byte MSVC string record, NOT std::string
        // from the current toolchain. Preserve its inline/heap discriminator.
        sq_pushstring(vm, kinoko::legacy::StringView(pointer<void>(source)).data(), -1);
        return 1;
    default: return -1;
    }
}

extern "C" int32_t retdec_set_var_value(int32_t* context, int32_t metadata, int32_t destination) {
    if (!context_has(context, 3) || !metadata || !destination) return -1;
    auto* vm = pointer<SQVM>(context[1]);
    const auto info = load<Variable>(metadata);
    if (info.flags & (ReadOnly | Constant)) return -1;
    return static_cast<int32_t>(upstream::sqplus_write_scalar(vm, info,
        pointer<void>(destination)));
}

extern "C" int32_t retdec_resolve_instance_var(int32_t vm_address, int32_t top,
    int32_t* output_metadata, int32_t* output_source) {
    if (output_metadata) *output_metadata = 0;
    if (output_source) *output_source = 0;
    auto* vm = pointer<SQVM>(vm_address);
    if (!vm || top < 2 || sq_gettop(vm) < 2 || !output_metadata || !output_source ||
        sq_gettype(vm, 1) != OT_INSTANCE) return 0;
    int32_t context[2] = {top, vm_address};
    int32_t metadata = 0;
    if (function_4aa5e0(context, &metadata) || !metadata) return 0;
    const auto info = load<Variable>(metadata);
    SQUserPointer native = nullptr;
    sq_getinstanceup(vm, 1, &native, nullptr);
    if (info.flags & (Constant | Static)) *output_source = info.offset;
    else {
        if (!native) return 0;
        *output_source = add_address(address(native), info.offset);
    }
    *output_metadata = metadata;
    return 1;
}

extern "C" int32_t function_4aab60(int32_t vm) { return table_access(vm, false); }
extern "C" int32_t function_4aabd0(int32_t vm) { return instance_access(vm, false); }
extern "C" int32_t function_4aaf30(int32_t vm) { return table_access(vm, true); }
extern "C" int32_t function_4aafa0(int32_t vm) { return instance_access(vm, true); }
