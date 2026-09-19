#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/script_registration.h"
#include "script_registration_host.hpp"

namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
template<class Function> int32_t entry(Function function) {
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(function));
}
struct InputMethod { const char* name; int32_t target; int32_t wrapper; };
const InputMethod methods[] = {
    {"Save", entry(function_46b7c0), entry(function_46ce70)},
    {"Load", entry(function_46b880), entry(function_46ce70)},
    {"SetAssign", entry(function_46bbe0), entry(function_46cec0)},
    {"WaitAssign", entry(function_46bc90), entry(function_46cf10)},
    {"GetAssign", entry(function_46be40), entry(function_46cf60)},
};
struct Field { const char* name; int32_t offset; bool boolean; };
// Original 46D950 order: s0 follows s9; button/key names intentionally alias.
constexpr Field fields[] = {
    {"x", 1436, false},
    {"y", 1440, false},
    {"br0", 1468, true},
    {"br1", 1469, true},
    {"br2", 1470, true},
    {"br3", 1471, true},
    {"b0", 1444, false},
    {"b1", 1448, false},
    {"b2", 1452, false},
    {"b3", 1456, false},
    {"b4", 1460, false},
    {"kr0", 1468, true},
    {"kr1", 1469, true},
    {"kr2", 1470, true},
    {"kr3", 1471, true},
    {"k0", 1444, false},
    {"k1", 1448, false},
    {"k2", 1452, false},
    {"k3", 1456, false},
    {"k4", 1460, false},
    {"k5", 1464, false},
    {"s1", 1476, false},
    {"s2", 1480, false},
    {"s3", 1484, false},
    {"s4", 1488, false},
    {"s5", 1492, false},
    {"s6", 1496, false},
    {"s7", 1500, false},
    {"s8", 1504, false},
    {"s9", 1508, false},
    {"s0", 1472, false},
};

// Original 46CFB0 class builder: 48-byte SqPlus class binding storage.
void construct_input_class(int32_t state[12]) {
    state[0] = address(g644);
    state[1] = address("Input");
    function_4a94e0_this(address(state + 2));
    state[5] = 0;
    function_4a91c0_this(state + 6);
    function_4a91c0_this(state + 9);
    auto* vm = current_vm();
    const auto top = sq_gettop(vm);
    int32_t temporary[3]{}, nested[3]{};
    function_4a94e0_this(address(temporary));
    if (function_4aa540(state[0], address(temporary), kinoko_input_binding_type(), state[1], 0)) {
        function_4a94e0_this(address(nested));
        ObjectView(temporary).push(vm);
        function_4a9660_this(address(nested), -1);
        sq_pop(vm, 1);
        function_45f640(nested);
    }
    sq_settop(vm, top);
    function_4a95c0_this(address(state + 2), address(temporary));
    function_4a9d70_this(address(temporary));
}
} // namespace

extern "C" int32_t function_46d950(void) {
    int32_t root[3]{}, state[12]{}, temporary[3]{};
    function_4a9500_this(root, function_4a8cc0());
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
        auto bind = field.boolean ? function_460a60 : function_460920;
        bind(state + 2, descriptor, field.offset, const_cast<char*>(field.name), 0);
    }
    function_4a95c0_this(address(g629), address(function_4aa3a0_this(address(root), address(temporary), "Input")));
    function_4a9d70_this(address(temporary));
    for (int offset : {9, 6, 2}) function_4a9d70_this(address(state + offset));
    return function_4a9d70_this(address(root));
}
