#include "kinoko/legacy_string.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "squserdata.h"

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;

// The two recovered words are a stack count and a borrowed VM, not two
// integers. memcpy access also supports existing unaligned C fixtures.
struct VariableContext { int32_t count; SQVM* vm; };
static_assert(sizeof(VariableContext)==8);
VariableContext context_value(const void* storage) { return load<VariableContext>(storage); }
bool context_has(const void* context, SQInteger count) {
    if (!context) return false;
    const auto state=context_value(context);
    return state.vm && state.count>=count && sq_gettop(state.vm)>=count;
}
int32_t table_access(SQVM* vm, bool write) {
    if (!vm) return -1;
    kinoko_sqplus_select_vm(vm);
    VariableContext context{static_cast<int32_t>(sq_gettop(vm)),vm};
    if (context.count < 1 || sq_gettype(vm, 1) != OT_TABLE) return -1;
    void* metadata = nullptr;
    const auto status = write ? kinoko_sqplus_find_table_variable(&metadata, &context) :
        kinoko_sqplus_find_variable(&context, &metadata);
    if (status) return status;
    if (!metadata) return -1;
    const auto source = load<Variable>(metadata).offset;
    return write ? kinoko_sqplus_write_variable(&context, metadata, (void*)(uintptr_t)(source)) :
        kinoko_sqplus_read_variable(&context, metadata, source);
}
int32_t instance_access(SQVM* vm, bool write) {
    if (!vm) return -1;
    kinoko_sqplus_select_vm(vm);
    VariableContext context{static_cast<int32_t>(sq_gettop(vm)),vm};
    void* metadata = nullptr; int32_t source = 0;
    if (!kinoko_sqplus_resolve_instance_variable(vm, context.count, &metadata, &source)) return -1;
    return write ? kinoko_sqplus_write_variable(&context, metadata, (void*)(uintptr_t)(source)) :
        kinoko_sqplus_read_variable(&context, metadata, source);
}
} // namespace

extern "C" int32_t  kinoko_sqplus_find_variable(const void* context, void** output) {
    if (!context || !output || !context_value(context).vm) return -1;
    void* value = nullptr;
    const auto status = upstream::sqplus_variable_info(context_value(context).vm, value);
    if (status == SQ_OK) *output = value;
    return status;
}

extern "C" int32_t  kinoko_sqplus_find_table_variable(void** output, const void* context) {
    if (output) *output = 0;
    if (!context_has(context, 1)) return -1;
    auto* vm = context_value(context).vm;
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
    if (output) *output = _userdataval(value);
    return 0;
}

extern "C" int32_t  kinoko_sqplus_read_variable(const void* context, const void* metadata, int32_t source) {
    if (!context || !context_value(context).vm || !metadata) return -1;
    auto* vm = context_value(context).vm;
    const auto info = load<Variable>(metadata);
    const bool immediate = (info.flags & Constant) != 0;
    switch (info.category) {
    case 0: case 1: case 2: case 3:
        return static_cast<int32_t>(upstream::sqplus_read_scalar(vm, info,
            pointer<const void>(source), source));
    case 4:
        if (!source) return -1;
        return upstream::sqplus_read_text(vm, pointer<const char>(immediate ? source : load<int32_t>(source)));
    case 5:
        if (!source) return -1;
        return upstream::sqplus_read_text(vm, pointer<const char>(add_address(source, 1)));
    case 8:
        if (!source) return -1;
        // This is the original 24-byte MSVC string record, NOT std::string
        // from the current toolchain. Preserve its inline/heap discriminator.
        return upstream::sqplus_read_text(vm, kinoko::legacy::StringView(pointer<void>(source)).data());
    default: return -1;
    }
}

extern "C" int32_t  kinoko_sqplus_write_variable(const void* context, const void* metadata, void* destination) {
    if (!context_has(context, 3) || !metadata || !destination) return -1;
    auto* vm = context_value(context).vm;
    const auto info = load<Variable>(metadata);
    if (info.flags & (ReadOnly | Constant)) return -1;
    return static_cast<int32_t>(upstream::sqplus_write_scalar(vm, info,
        destination));
}

extern "C" int32_t  kinoko_sqplus_resolve_instance_variable(struct SQVM* vm_address, int32_t top, void** output_metadata, int32_t* output_source) {
    if (output_metadata) *output_metadata = 0;
    if (output_source) *output_source = 0;
    auto* vm = vm_address;
    if (!vm || top < 2 || sq_gettop(vm) < 2 || !output_metadata || !output_source ||
        sq_gettype(vm, 1) != OT_INSTANCE) return 0;
    VariableContext context{top,vm};
    void* metadata = nullptr;
    if (kinoko_sqplus_find_variable(&context, &metadata) || !metadata) return 0;
    const auto info = load<Variable>(metadata);
    HSQOBJECT instance{};
    sq_getstackobj(vm, 1, &instance);
    SQUserPointer storage = nullptr;
    if (!upstream::sqplus_instance_storage(vm, instance, info, storage)) return 0;
    *output_source = address(storage);
    *output_metadata = metadata;
    return 1;
}

extern "C" int32_t  kinoko_sqplus_table_get(struct SQVM * vm) { return table_access(vm, false); }
extern "C" int32_t  kinoko_sqplus_instance_get(struct SQVM * vm) { return instance_access(vm, false); }
extern "C" int32_t  kinoko_sqplus_table_set(struct SQVM * vm) { return table_access(vm, true); }
extern "C" int32_t  kinoko_sqplus_instance_set(struct SQVM * vm) { return instance_access(vm, true); }
