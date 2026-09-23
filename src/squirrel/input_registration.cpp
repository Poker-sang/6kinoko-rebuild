#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "script_registration_host.hpp"
#include "kinoko/input_devices.h"
#include "kinoko/direct_input.h"
#include <array>

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
template<class Function> int32_t entry(Function function) {
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(function));
}
// Explicit boundary to shared SqPlus wrappers that still carry integer slots.
KinokoInputManager* input_receiver(int32_t self) { return reinterpret_cast<KinokoInputManager*>(static_cast<intptr_t>(self)); }
int32_t save_config(int32_t self, const char* path) { return kinoko_input_save_config(input_receiver(self), path); }
int32_t load_config(int32_t self, const char* path) { return kinoko_input_load_config(input_receiver(self), path); }
int32_t set_assignment(int32_t self, int32_t device, int32_t field, int32_t value) {
    return kinoko_input_set_assignment(input_receiver(self), device, field, value);
}
int32_t wait_assignment(int32_t self, int32_t device, int32_t field) {
    return kinoko_input_wait_assignment(input_receiver(self), device, field);
}
int32_t get_assignment(int32_t self, int32_t device, int32_t field) {
    return kinoko_input_get_assignment(input_receiver(self), device, field);
}
struct InputMethod { const char* name; int32_t target; int32_t wrapper; };
const InputMethod methods[] = {
    {"Save", entry(save_config), entry(function_46ce70)},
    {"Load", entry(load_config), entry(function_46ce70)},
    {"SetAssign", entry(set_assignment), entry(function_46cec0)},
    {"WaitAssign", entry(wait_assignment), entry(function_46cf10)},
    {"GetAssign", entry(get_assignment), entry(function_46cf60)},
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
void construct_input_class(int32_t state[12]) {
    state[0] = address(current_vm());
    state[1] = address("Input");
    kinoko_sqplus_object_initialize((void *)(state + 2));
    state[5] = 0;
    kinoko_sqplus_object_new_table((void *)(intptr_t)(state + 6));
    kinoko_sqplus_object_new_table((void *)(intptr_t)(state + 9));
    auto* vm = current_vm();
    const auto top = sq_gettop(vm);
    int32_t temporary[3]{}, nested[3]{};
    kinoko_sqplus_object_initialize((void *)(temporary));
    if (kinoko_sqplus_create_class((struct SQVM *)(intptr_t)(state[0]), (void *)(temporary), kinoko_input_binding_type(), (const char *)(intptr_t)(state[1]), (const char *)(intptr_t)(0))) {
        kinoko_sqplus_object_initialize((void *)(nested));
        ObjectView(temporary).push(vm);
        kinoko_sqplus_object_capture((void *)(nested), -1);
        sq_pop(vm, 1);
        (int32_t)(intptr_t)(kinoko_sqplus_setup_hierarchy(nested));
    }
    sq_settop(vm, top);
    kinoko_sqplus_object_assign((void *)(state + 2), (const void *)(temporary));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(temporary)));
}
} // namespace

extern "C" int32_t kinoko_register_input_class(void) {
    int32_t root[3]{}, state[12]{}, temporary[3]{};
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(root), kinoko_sqplus_root_object());
    construct_input_class(state);
    auto* vm = pointer<SQVM>(state[0]);
    for (const auto& method : methods) {
        ObjectView(state + 2).push(vm);
        sq_pushstring(vm, method.name, -1);
        std::memcpy(sq_newuserdata(vm, 4), &method.target, 4);
        sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(pointer(method.wrapper)), 1);
        sq_newslot(vm, -3, SQFalse);
        sq_pop(vm, 1);
    }
    auto* descriptor = kinoko_input_binding_type();
    for (const auto& field : fields) {
        auto bind = field.boolean ? kinoko_sqplus_bind_boolean : kinoko_sqplus_bind_integer;
        bind(state + 2, descriptor, field.offset, const_cast<char*>(field.name), 0);
    }
    kinoko_sqplus_object_assign(const_cast<void *>(kinoko_input_script_symbols()->input_class), (const void *)((int32_t*)(intptr_t)(kinoko_sqplus_object_get_value((void *)(root), (void *)(temporary), "Input"))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(temporary)));
    for (int offset : {9, 6, 2}) (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(state + offset)));
    return (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(root)));
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
    retdec_trace("46e6f0:begin");
    retdec_trace_i32("46e6f0:this", static_cast<int32_t>(reinterpret_cast<uintptr_t>(manager)));
    retdec_trace_i32("46e6f0:g644", static_cast<int32_t>(reinterpret_cast<uintptr_t>(host.vm)));
    const auto* input_class = static_cast<const int32_t*>(host.input_class);
    retdec_trace_i32("46e6f0:g629-type", input_class[0]);
    retdec_trace_i32("46e6f0:g629-data", input_class[1]);
    std::array<int32_t, 3> instance{};
    kinoko_sqplus_object_new_instance(instance.data(), host.input_class);
    retdec_trace_i32("46e6f0:instance-type", instance[1]);
    retdec_trace_i32("46e6f0:instance-data", instance[2]);
    kinoko_sqplus_object_assign(manager->script_object, instance.data());
    kinoko_sqplus_object_destroy(instance.data());
    kinoko_sqplus_object_set_instance(manager->script_object, manager);
    kinoko_sqplus_object_raw_set_name(host.root, "input", manager->script_object);
    retdec_trace("46e6f0:done");
    return 1;
}
