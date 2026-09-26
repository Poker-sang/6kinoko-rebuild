#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "script_registration_host.hpp"
#include "kinoko/input_devices.h"
#include "kinoko/direct_input.h"
#include <array>

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
template<class Function> void* entry(Function function) {
    return reinterpret_cast<void*>(function);
}
// Native input callbacks borrow the actual manager pointer.
KinokoInputManager* input_receiver(void* self) { return static_cast<KinokoInputManager*>(self); }
int32_t save_config(void* self, const char* path) { return kinoko_input_save_config(input_receiver(self), path); }
int32_t load_config(void* self, const char* path) { return kinoko_input_load_config(input_receiver(self), path); }
int32_t set_assignment(void* self, int32_t device, int32_t field, int32_t value) {
    return kinoko_input_set_assignment(input_receiver(self), device, field, value);
}
int32_t wait_assignment(void* self, int32_t device, int32_t field) {
    return kinoko_input_wait_assignment(input_receiver(self), device, field);
}
int32_t get_assignment(void* self, int32_t device, int32_t field) {
    return kinoko_input_get_assignment(input_receiver(self), device, field);
}
struct InputMethod { const char* name; void* target; SQFUNCTION wrapper; };
const InputMethod methods[] = {
    {"Save", entry(save_config), kinoko_input_save_entry},
    {"Load", entry(load_config), kinoko_input_save_entry},
    {"SetAssign", entry(set_assignment), kinoko_input_assign_entry},
    {"WaitAssign", entry(wait_assignment), kinoko_input_wait_entry},
    {"GetAssign", entry(get_assignment), kinoko_input_get_entry},
};
struct Field { const char* name; int32_t offset; bool boolean; };
// Original 46D950 order: s0 follows s9; button/key names intentionally alias.
constexpr Field fields[] = {
    {"x", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, x)), false},
    {"y", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, y)), false},
    {"br0", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[0])), true},
    {"br1", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[1])), true},
    {"br2", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[2])), true},
    {"br3", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[3])), true},
    {"b0", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[0])), false},
    {"b1", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[1])), false},
    {"b2", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[2])), false},
    {"b3", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[3])), false},
    {"b4", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[4])), false},
    {"kr0", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[0])), true},
    {"kr1", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[1])), true},
    {"kr2", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[2])), true},
    {"kr3", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, released[3])), true},
    {"k0", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[0])), false},
    {"k1", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[1])), false},
    {"k2", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[2])), false},
    {"k3", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[3])), false},
    {"k4", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[4])), false},
    {"k5", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, buttons[5])), false},
    {"s1", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[1])), false},
    {"s2", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[2])), false},
    {"s3", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[3])), false},
    {"s4", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[4])), false},
    {"s5", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[5])), false},
    {"s6", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[6])), false},
    {"s7", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[7])), false},
    {"s8", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[8])), false},
    {"s9", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[9])), false},
    {"s0", static_cast<int32_t>(offsetof(KinokoInputManager, published) + offsetof(KinokoInputPublishedState, digits[0])), false},
};

// Original 46CFB0 class builder: 48-byte SqPlus class binding storage.
void construct_input_class(ClassBindingStorage& state) {
    state.vm = current_vm();
    state.name = "Input";
    kinoko_sqplus_object_initialize(&state.klass);
    state.parent = nullptr;
    kinoko_sqplus_object_new_table(&state.members);
    kinoko_sqplus_object_new_table(&state.methods);
    auto* vm = current_vm();
    const auto top = sq_gettop(vm);
    ObjectStorage temporary{}, nested{};
    kinoko_sqplus_object_initialize(&temporary);
    if (kinoko_sqplus_create_class(state.vm, &temporary, kinoko_input_binding_type(), state.name, nullptr)) {
        kinoko_sqplus_object_initialize(&nested);
        ObjectView(&temporary).push(vm);
        kinoko_sqplus_object_capture(&nested, -1);
        sq_pop(vm, 1);
        kinoko_sqplus_setup_hierarchy(&nested);
    }
    sq_settop(vm, top);
    kinoko_sqplus_object_assign(&state.klass, &temporary);
    kinoko_sqplus_object_destroy(&temporary);
}
} // namespace

extern "C" int32_t kinoko_register_input_class(void) {
    ObjectStorage root{}, temporary{};
    ClassBindingStorage state{};
    kinoko_sqplus_object_copy_construct(&root, kinoko_sqplus_root_object());
    construct_input_class(state);
    auto* vm = state.vm;
    for (const auto& method : methods) {
        ObjectView(&state.klass).push(vm);
        sq_pushstring(vm, method.name, -1);
        std::memcpy(sq_newuserdata(vm, sizeof method.target), &method.target, sizeof method.target);
        sq_newclosure(vm, method.wrapper, 1);
        sq_newslot(vm, -3, SQFalse);
        sq_pop(vm, 1);
    }
    auto* descriptor = kinoko_input_binding_type();
    for (const auto& field : fields) {
        auto bind = field.boolean ? kinoko_sqplus_bind_boolean : kinoko_sqplus_bind_integer;
        bind(reinterpret_cast<int32_t*>(&state.klass), descriptor, field.offset, const_cast<char*>(field.name), 0);
    }
    kinoko_sqplus_object_assign(const_cast<void*>(kinoko_input_script_symbols()->input_class),
        kinoko_sqplus_object_get_value(&root, &temporary, "Input"));
    kinoko_sqplus_object_destroy(&temporary);
    kinoko_sqplus_object_destroy(&state.methods);
    kinoko_sqplus_object_destroy(&state.members);
    kinoko_sqplus_object_destroy(&state.klass);
    return address(kinoko_sqplus_object_destroy(&root));
}

// Original 46E6F0 receives the 0x513CC0 CInputManager in ECX. The host
// retains the 0x600-byte allocation; these symbols borrow its SqPlus globals.
extern "C" int32_t kinoko_input_initialize_script_instance(KinokoInputManager* manager) {
    if (!manager) return 0;
    static bool devices_constructed = false;
    const auto& host = *kinoko_input_script_symbols();
    if (!devices_constructed) {
        std::memset(manager, 0, host.manager_storage_bytes);
        const std::array<int32_t, 3> empty = {
            static_cast<int32_t>(reinterpret_cast<uintptr_t>(host.object_vtable)),
            host.null_type, host.null_data};
        std::memcpy(manager->script_object, empty.data(), sizeof(empty));
        kinoko_input_manager_construct_devices(manager, static_cast<uint32_t>(kinoko_input_snapshot.controller_count));
        devices_constructed = true;
    }
    kinoko_trace("46e6f0:begin");
    kinoko_trace_i32("46e6f0:this", static_cast<int32_t>(reinterpret_cast<uintptr_t>(manager)));
    kinoko_trace_i32("46e6f0:g644", static_cast<int32_t>(reinterpret_cast<uintptr_t>(host.vm)));
    const auto* input_class = static_cast<const int32_t*>(host.input_class);
    kinoko_trace_i32("46e6f0:g629-type", input_class[0]);
    kinoko_trace_i32("46e6f0:g629-data", input_class[1]);
    std::array<int32_t, 3> instance{};
    kinoko_sqplus_object_new_instance(instance.data(), host.input_class);
    kinoko_trace_i32("46e6f0:instance-type", instance[1]);
    kinoko_trace_i32("46e6f0:instance-data", instance[2]);
    kinoko_sqplus_object_assign(manager->script_object, instance.data());
    kinoko_sqplus_object_destroy(instance.data());
    kinoko_sqplus_object_set_instance(manager->script_object, manager);
    kinoko_sqplus_object_raw_set_name(host.root, "input", manager->script_object);
    kinoko_trace("46e6f0:done");
    return 1;
}
