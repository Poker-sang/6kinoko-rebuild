#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "script_registration_host.hpp"

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
    state[0] = address(g644);
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
    kinoko_sqplus_object_assign((void *)(g629), (const void *)((int32_t*)(intptr_t)(kinoko_sqplus_object_get_value((void *)(root), (void *)(temporary), "Input"))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(temporary)));
    for (int offset : {9, 6, 2}) (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(state + offset)));
    return (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(root)));
}
