#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/legacy_abi.h"
#include <cstdio>
#include <string>
#include <utility>

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;

struct ClassBindingStorage {
    SQVM* vm;
    const char* name;
    ObjectStorage klass;
    const char* parent;
    ObjectStorage members;
    ObjectStorage methods;
};
static_assert(sizeof(ClassBindingStorage) == 48);
static_assert(offsetof(ClassBindingStorage, klass) == 8);
static_assert(offsetof(ClassBindingStorage, members) == 24);
static_assert(offsetof(ClassBindingStorage, methods) == 36);

int32_t bind_variable(int32_t* object, int32_t* instance_type, int32_t offset,
                      const char* name, int32_t flags, int32_t category, int32_t size) {
    const auto payload = (int32_t)(intptr_t)(kinoko_sqplus_create_variable(object, name));
    // Existing slots are never overwritten with replacement userdata. A wrong
    // type/short payload is an invalid binding, not permission to corrupt it.
    if (!payload) return 0;
    Variable info{};
    kinoko_sqplus_initialize_variable(reinterpret_cast<int32_t*>(&info), offset, category, address(instance_type), kinoko_native_binding_type(category), size, flags);
    store(payload, info);
    return kinoko_sqplus_install_variable_handlers(object);
}
} // namespace

extern "C" void * kinoko_sqplus_new_string(void * output, const char * text) {
    auto* vm = current_vm();
    ObjectView result(output);
    result.initialize(kinoko_squirrel_object_vtable());
    result.write(upstream::sqplus_new_string(vm, static_cast<const char *>(text)));
    return output;
}

extern "C" void * kinoko_sqplus_bind_function(void * output, void * native, const char * name, const char * mask_address) {
    auto* vm = current_vm();
    ObjectView result(output);
    result.initialize(kinoko_squirrel_object_vtable());
    const bool bound = upstream::sqplus_bind_function(vm,
        reinterpret_cast<SQFUNCTION>(native), static_cast<const char *>(name),
        static_cast<const char *>(mask_address), [](void* storage, HSQOBJECT value) {
            ObjectView(storage).write(value);
        }, output);
    if (!bound) {
        result.release(vm); result.reset();
        return 0;
    }
    return output;
}

extern "C" void * kinoko_sqplus_bind_object_function(int32_t* output, void * object, void * native, const char* name, const char* mask) {
    auto* vm = current_vm();
    ObjectView(object).push(vm);
    auto* result = kinoko_sqplus_bind_function(output, native, name, mask);
    sq_pop(vm, 1);
    return result;
}

extern "C" int32_t kinoko_sqplus_create_class(struct SQVM * vm_address, void * output, int32_t* type, const char * name, const char * parent) {
    auto* vm = static_cast<SQVM *>(vm_address);
    StackTop stack(vm);
    ObjectView result(output);
    auto value = result.value();
    const bool created = upstream::sqplus_create_class(vm, value, type,
        static_cast<const char *>(name), static_cast<const char *>(parent));
    result.write(value);
    // The original reports successful class creation, not newslot's status.
    return created;
}

extern "C" int32_t kinoko_sqplus_install_variable_handlers(void * object) {
    upstream::sqplus_variable_handlers(current_vm(), ObjectView(object).value(),
        reinterpret_cast<SQFUNCTION>(&kinoko_sqplus_instance_set), reinterpret_cast<SQFUNCTION>(&kinoko_sqplus_instance_get));
    return 1;
}

extern "C" void* kinoko_sqplus_setup_hierarchy(int32_t* root_object) {
    const ObjectView root(root_object);
    const auto value = root.value();
    root.reset(); // transfer this by-value argument's owned reference
    upstream::sqplus_setup_hierarchy(current_vm(), value);
    return (void*)(intptr_t)(root.payload_address());
}

extern "C" void * kinoko_sqplus_create_variable(void * object, const char * name_address) {
    return upstream::sqplus_create_variable(current_vm(), ObjectView(object).value(), name_address);
}

extern "C" int32_t* kinoko_sqplus_initialize_variable(int32_t* output, int32_t offset, int32_t category, int32_t instance_type, int32_t* value_type, int32_t size, int32_t flags) {
    const Variable info{offset, category, instance_type, address(value_type),
        static_cast<uint16_t>(size), static_cast<uint16_t>(flags)};
    upstream::sqplus_variable_metadata(current_vm(),
        ObjectView((int32_t)(intptr_t)(kinoko_sqplus_root_object())).value(), info, output);
    return output;
}

extern "C" int32_t kinoko_sqplus_bind_integer(int32_t* object, int32_t* type,
                                    int32_t offset, char* name, int32_t flags) {
    return bind_variable(object, type, offset, name, flags, 0, 4);
}
extern "C" int32_t kinoko_sqplus_bind_float(int32_t* object, int32_t* type,
                                    int32_t offset, char* name, int32_t flags) {
    return bind_variable(object, type, offset, name, flags, 2, 4);
}
extern "C" int32_t kinoko_sqplus_bind_boolean(int32_t* object, int32_t* type,
                                    int32_t offset, char* name, int32_t flags) {
    return bind_variable(object, type, offset, name, flags, 3, 1);
}

extern "C" void * kinoko_sqplus_create_actor_class(int32_t* output, struct SQVM * vm_address, const char * name, const char * parent) {
    auto* vm = static_cast<SQVM *>(vm_address);
    StackTop stack(vm);
    ObjectView result(output);
    result.initialize(kinoko_squirrel_object_vtable());
    if (kinoko_sqplus_create_class(vm_address, output, kinoko_native_binding_type(-1), name, parent)) {
        ObjectStorage copy{};
        ObjectView copy_view(&copy);
        copy_view.initialize(kinoko_squirrel_object_vtable());
        copy_view.assign(vm, result.value());
        (int32_t)(intptr_t)(kinoko_sqplus_setup_hierarchy(reinterpret_cast<int32_t*>(&copy)));
    }
    return output;
}

extern "C" void * kinoko_sqplus_construct_class_binding(void * output, const char* name, const char * parent) {
    if (!output) return 0;
    auto* vm = current_vm();
    const kinoko::native::RecordView<ClassBindingStorage> state(output);
    state.set(&ClassBindingStorage::vm, vm);
    state.set(&ClassBindingStorage::name, name);
    state.set(&ClassBindingStorage::parent, parent);
    ObjectView klass(state.bytes(&ClassBindingStorage::klass));
    ObjectView members(state.bytes(&ClassBindingStorage::members));
    ObjectView methods(state.bytes(&ClassBindingStorage::methods));
    for (auto view : {klass, members, methods}) view.initialize(kinoko_squirrel_object_vtable());
    new_table(vm, members); new_table(vm, methods);
    Object temporary(vm);
    kinoko_sqplus_create_actor_class(pointer<int32_t>(temporary.location()), vm, name, parent);
    klass.assign(vm, temporary.view().value());
    return output;
}

extern "C" int32_t* kinoko_sqplus_define_actor_class(int32_t* output, const char* name, int32_t parent) {
    if (!output) return nullptr;
    ClassBindingStorage state{};
    kinoko_sqplus_construct_class_binding(&state, name, pointer<const char>(parent));
    auto* vm = current_vm();
    ObjectView(output).assign(vm, ObjectView(&state.klass).value());
    for (auto* object : {&state.klass, &state.members, &state.methods}) ObjectView(object).release(vm);
    return output;
}

extern "C" void kinoko_sqplus_register_actor_method(struct SQVM * vm_address, int32_t* object, const char* name, void * native, void * wrapper, int32_t slot_flags) {
    auto* vm = static_cast<SQVM *>(vm_address);
    ObjectView(object).push(vm);
    sq_pushstring(vm, name, -1);
    const Method method{address(native), 0};
    if (auto* payload = sq_newuserdata(vm, sizeof(method))) store(payload, method);
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(wrapper), 1);
    // This last argument is sq_newslot's static flag, NOT a parameter count.
    sq_newslot(vm, -3, slot_flags);
    sq_pop(vm, 1);
}
