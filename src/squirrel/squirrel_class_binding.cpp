#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/legacy_abi.h"
#include <cstdio>
#include <string>
#include <utility>

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;

struct ClassBindingStorage {
    int32_t vm;
    int32_t name;
    ObjectStorage klass;
    int32_t parent;
    ObjectStorage members;
    ObjectStorage methods;
};
static_assert(sizeof(ClassBindingStorage) == 48);
static_assert(offsetof(ClassBindingStorage, klass) == 8);
static_assert(offsetof(ClassBindingStorage, members) == 24);
static_assert(offsetof(ClassBindingStorage, methods) == 36);

int32_t bind_variable(int32_t* object, int32_t* instance_type, int32_t offset,
                      const char* name, int32_t flags, int32_t category, int32_t size) {
    const auto payload = function_45fab0(address(object), address(name));
    // Existing slots are never overwritten with replacement userdata. A wrong
    // type/short payload is an invalid binding, not permission to corrupt it.
    if (!payload) return 0;
    Variable info{};
    function_45f3e0_this(reinterpret_cast<int32_t*>(&info), offset, category,
        address(instance_type), kinoko_native_binding_type(category), size, flags);
    store(payload, info);
    return function_45f4f0(address(object));
}
} // namespace

extern "C" int32_t function_4a9250(int32_t output, int32_t text) {
    auto* vm = current_vm();
    ObjectView result(output);
    result.initialize(kinoko_squirrel_object_vtable());
    result.write(upstream::sqplus_new_string(vm, pointer<const char>(text)));
    return output;
}

extern "C" int32_t function_4a9370(int32_t output, int32_t native,
                                    int32_t name, int32_t mask_address) {
    auto* vm = current_vm();
    ObjectView result(output);
    result.initialize(kinoko_squirrel_object_vtable());
    const bool bound = upstream::sqplus_bind_function(vm,
        reinterpret_cast<SQFUNCTION>(pointer(native)), pointer<const char>(name),
        pointer<const char>(mask_address), [](void* storage, HSQOBJECT value) {
            ObjectView(storage).write(value);
        }, pointer(output));
    if (!bound) {
        result.release(vm); result.reset();
        return 0;
    }
    return output;
}

extern "C" int32_t function_4a9490(int32_t* output, int32_t object,
                                    int32_t native, char* name, char* mask) {
    auto* vm = current_vm();
    ObjectView(object).push(vm);
    const auto result = function_4a9370(address(output), native, address(name), address(mask));
    sq_pop(vm, 1);
    return result;
}

extern "C" int32_t function_4aa540(int32_t vm_address, int32_t output,
                                    int32_t* type, int32_t name, int32_t parent) {
    auto* vm = pointer<SQVM>(vm_address);
    StackTop stack(vm);
    ObjectView result(output);
    auto value = result.value();
    const bool created = upstream::sqplus_create_class(vm, value, type,
        pointer<const char>(name), pointer<const char>(parent));
    result.write(value);
    // The original reports successful class creation, not newslot's status.
    return created;
}

extern "C" int32_t function_45f4f0(int32_t object) {
    auto* vm = current_vm();
    if (has_slot(vm, ObjectView(object), "_set")) return 1;
    int32_t temporary[3];
    function_4a9490(temporary, object, address(reinterpret_cast<void*>(&function_4aafa0)),
        const_cast<char*>("_set"), const_cast<char*>("sn|b|s|x"));
    function_4a9d70_this(address(temporary));
    function_4a9490(temporary, object, address(reinterpret_cast<void*>(&function_4aabd0)),
        const_cast<char*>("_get"), const_cast<char*>("s"));
    return function_4a9d70_this(address(temporary));
}

extern "C" int32_t function_45f640(int32_t* root_object) {
    const ObjectView root(root_object);
    const auto value = root.value();
    root.reset(); // transfer this by-value argument's owned reference
    upstream::sqplus_setup_hierarchy(current_vm(), value);
    return root.payload_address();
}

extern "C" int32_t function_45fab0(int32_t object, int32_t name_address) {
    auto* vm = current_vm();
    const auto key = variable_key(pointer<const char>(name_address));
    auto lookup = [&]() -> std::pair<bool, int32_t> {
        StackTop stack(vm);
        ObjectView(object).push(vm); sq_pushstring(vm, key.data(), -1);
        if (SQ_FAILED(sq_get(vm, -2))) return {false, 0};
        SQUserPointer payload = nullptr;
        if (SQ_FAILED(sq_getuserdata(vm, -1, &payload, nullptr)) ||
            sq_getsize(vm, -1) < static_cast<SQInteger>(sizeof(Variable)))
            return {true, 0};
        return {true, address(payload)};
    };
    auto found = lookup();
    if (!found.first) {
        function_4a9950(object, address(key.data()), sizeof(Variable), 0);
        found = lookup();
    }
    return found.second;
}

extern "C" int32_t* function_45f3e0_this(int32_t* output, int32_t offset,
    int32_t category, int32_t instance_type, int32_t* value_type,
    int32_t size, int32_t flags) {
    const Variable info{offset, category, instance_type, address(value_type),
        static_cast<uint16_t>(size), static_cast<uint16_t>(flags)};
    store(output, info);
    auto* vm = current_vm();
    Object types(vm);
    ObjectView root(function_4a8cc0());
    get_slot(vm, root, "__SqTypes", types.view());
    if (types.view().value()._type == OT_NULL) {
        new_table(vm, types.view());
        raw_store(vm, root, "__SqTypes", types.view());
    }
    int32_t name = 0;
    if (value_type) {
        const auto vtable = load<int32_t>(value_type);
        if (vtable) name = retdec_call_thiscall0_result(value_type,
            pointer(load<int32_t>(add_address(vtable, 4))));
    }
    // Integer descriptor identity is the key, never a string pointer.
    function_4a9730_this(types.location(), address(value_type), name);
    return output;
}

extern "C" int32_t function_460920(int32_t* object, int32_t* type,
                                    int32_t offset, char* name, int32_t flags) {
    return bind_variable(object, type, offset, name, flags, 0, 4);
}
extern "C" int32_t function_4609c0(int32_t* object, int32_t* type,
                                    int32_t offset, char* name, int32_t flags) {
    return bind_variable(object, type, offset, name, flags, 2, 4);
}
extern "C" int32_t function_460a60(int32_t* object, int32_t* type,
                                    int32_t offset, char* name, int32_t flags) {
    return bind_variable(object, type, offset, name, flags, 3, 1);
}

extern "C" int32_t function_4607e0(int32_t* output, int32_t vm_address,
                                    int32_t name, int32_t parent) {
    auto* vm = pointer<SQVM>(vm_address);
    StackTop stack(vm);
    ObjectView result(output);
    result.initialize(kinoko_squirrel_object_vtable());
    if (function_4aa540(vm_address, address(output), kinoko_native_binding_type(-1), name, parent)) {
        ObjectStorage copy{};
        ObjectView copy_view(&copy);
        copy_view.initialize(kinoko_squirrel_object_vtable());
        copy_view.assign(vm, result.value());
        function_45f640(reinterpret_cast<int32_t*>(&copy));
    }
    return address(output);
}

extern "C" int32_t function_460d10_this(int32_t output, const char* name, int32_t parent) {
    if (!output) return 0;
    auto* vm = current_vm();
    store(output, address(vm));
    store(add_address(output, 4), address(name));
    store(add_address(output, 20), parent);
    ObjectView klass(add_address(output, 8));
    ObjectView members(add_address(output, 24));
    ObjectView methods(add_address(output, 36));
    for (auto view : {klass, members, methods}) view.initialize(kinoko_squirrel_object_vtable());
    new_table(vm, members); new_table(vm, methods);
    Object temporary(vm);
    function_4607e0(pointer<int32_t>(temporary.location()), address(vm), address(name), parent);
    klass.assign(vm, temporary.view().value());
    return output;
}

extern "C" int32_t* function_460d10_actor(int32_t* output, const char* name, int32_t parent) {
    if (!output) return nullptr;
    ClassBindingStorage state{};
    function_460d10_this(address(&state), name, parent);
    auto* vm = current_vm();
    ObjectView(output).assign(vm, ObjectView(&state.klass).value());
    for (auto* object : {&state.klass, &state.members, &state.methods}) ObjectView(object).release(vm);
    return output;
}

extern "C" void function_460e00_register_actor_method(int32_t vm_address,
    int32_t* object, const char* name, int32_t native, int32_t wrapper, int32_t slot_flags) {
    auto* vm = pointer<SQVM>(vm_address);
    ObjectView(object).push(vm);
    sq_pushstring(vm, name, -1);
    const Method method{native, 0};
    if (auto* payload = sq_newuserdata(vm, sizeof(method))) store(payload, method);
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(pointer(wrapper)), 1);
    // This last argument is sq_newslot's static flag, NOT a parameter count.
    sq_newslot(vm, -3, slot_flags);
    sq_pop(vm, 1);
}
