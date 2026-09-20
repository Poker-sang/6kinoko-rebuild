#include "kinoko/sqrat_object_bridge.h"
#include "squirrel_bridge_test_support.hpp"
#include <cstdlib>

extern "C" {
char g560 = 0;
int32_t kinoko_sqrat_object_vtable(void) { return 0x12121212; }
int32_t kinoko_sqrat_root_vtable(void) { return 0x34343434; }
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
}
namespace {
using namespace bridge_test;
int released = 0, capture_calls = 0, error_handler_calls = 0;
SQInteger release(SQUserPointer, SQInteger) { ++released; return 0; }
SQInteger captured(HSQUIRRELVM vm) {
    require(receiver == address(vm), "native receiver follows actual child VM");
    require(sq_gettop(vm) == 3, "one user argument plus receiver plus capture");
    ++capture_calls;
    SQUserPointer payload = nullptr;
    if (sq_gettype(vm, -1) == OT_USERDATA) {
        require(SQ_SUCCEEDED(sq_getuserdata(vm, -1, &payload, nullptr)), "capture userdata");
        sq_pushinteger(vm, load<int32_t>(payload) + get_integer(vm, 2));
    } else sq_pushinteger(vm, get_integer(vm, -1) + get_integer(vm, 2));
    return 1;
}
SQInteger plain(HSQUIRRELVM vm) { require(sq_gettop(vm) == 1, "zero capture count"); sq_pushinteger(vm, 99); return 1; }
SQInteger handler(HSQUIRRELVM vm) { require(receiver == address(vm), "error handler VM receiver"); ++error_handler_calls; return 0; }

void virtual_entries(HSQUIRRELVM vm) {
    Top restore(vm);
    Pair value(vm);
    sq_newuserdata(vm, 8); sq_setreleasehook(vm, -1, release); value.capture();
    auto object = wrapper(vm, value);
    std::array<unsigned char, 12> output{};
    // Invoke exactly as the original vtable caller: ECX=this, hidden output
    // pointer on the stack, and callee cleanup. Neither getter acquires a ref.
    using CopySlot = int32_t (__thiscall*)(void*, int32_t);
    using ReferenceSlot = int32_t (__thiscall*)(void*);
    using DeleteSlot = int32_t (__thiscall*)(void*, int32_t);
    const auto copy = reinterpret_cast<CopySlot>(&kinoko_sqrat_copy_object);
    const auto reference = reinterpret_cast<ReferenceSlot>(&kinoko_sqrat_object_reference);
    const auto destroy = reinterpret_cast<DeleteSlot>(&kinoko_sqrat_delete_object);
    require(copy(object.data(), address(output.data() + 1)) == address(output.data() + 1), "const virtual returns hidden output address");
    require(load<HSQOBJECT>(output.data() + 1)._unVal.pUserData == value.get()._unVal.pUserData, "const virtual copies borrowed object");
    require(reference(object.data()) == address(object.data() + 2), "reference virtual addresses host pair");
    const auto before = released;
    const auto borrowed = object;
    require(destroy(object.data(), 0) == address(object.data()), "borrowed destructor returns receiver");
    require(released == before && object[2] == borrowed[2] && object[3] == borrowed[3] && object[4] == 0, "borrowed destructor does not release or overwrite pair");
    require(object[0] == kinoko_sqrat_object_vtable(), "destructor restores base vtable");
    // Transfer the sole external owner to a real heap record; the deleting
    // slot must release it once and use the recovered allocation family.
    auto* heap = static_cast<int32_t*>(std::malloc(sizeof(object)));
    require(heap != nullptr, "heap wrapper allocation");
    std::memcpy(heap, object.data(), sizeof(object)); heap[4] = 1;
    sq_resetobject(reinterpret_cast<HSQOBJECT*>(value.data()));
    const auto heap_address = address(heap);
    require(destroy(heap, 1) == heap_address && released == before + 1, "owning deleting destructor releases exactly once");
}

void ownership(HSQUIRRELVM vm) {
    Top restore(vm);
    const auto base = sq_gettop(vm);
    for (int misalignment = 0; misalignment < 4; ++misalignment) {
        std::array<unsigned char, 32> bytes; bytes.fill(0xa7);
        auto* object = bytes.data() + 4 + misalignment;
        require(retdec_sqrat_root_construct(address(object), address(vm)) == address(object), "construct returns object");
        require(load<int32_t>(object) == kinoko_sqrat_root_vtable(), "root vtable identity");
        require(load<int32_t>(object + 4) == address(vm), "stored owning VM");
        require(load<HSQOBJECT>(object + 8)._type == OT_TABLE && object[16] == 1, "root pair and ownership");
        top(vm, base, "root construction balances stack");
        retdec_sqrat_object_release(address(object));
        retdec_sqrat_object_release(address(object));
        require(load<int32_t>(object) == kinoko_sqrat_object_vtable(), "base vtable after release");
        require(load<int32_t>(object + 4) == address(vm), "release preserves VM field");
        require(load<HSQOBJECT>(object + 8)._type == OT_NULL && load<int32_t>(object + 12) == 0 && object[16] == 0, "idempotent null release");
        for (int i = 0; i < 32; ++i) if (i < 4 + misalignment || i >= 21 + misalignment)
            require(bytes[i] == 0xa7, "unaligned wrapper padding/canaries untouched");
    }
    Pair table(vm), fetched(vm), userdata(vm);
    require(retdec_sqrat_new_table(address(vm), table.data()) == 1, "externally owned table");
    auto object = wrapper(vm, table);
    sq_newuserdata(vm, 8); sq_setreleasehook(vm, -1, release); userdata.capture();
    const auto before = released;
    require(retdec_sqrat_set_pair(address(vm), table.data(), "value", userdata.data()), "newslot userdata");
    retdec_sqrat_release_pair(address(vm), userdata.data());
    require(retdec_sqrat_get(address(object.data()), "value", fetched.id()), "get externally owned pair");
    require(retdec_sqrat_set_int(address(vm), table.data(), "value", 0), "replace table-owned userdata");
    require(released == before, "returned pair survives removal from table");
    retdec_sqrat_release_pair(address(vm), fetched.data());
    retdec_sqrat_release_pair(address(vm), fetched.data());
    require(released == before + 1, "external value released exactly once");
    require(!retdec_sqrat_get(address(object.data()), "missing", fetched.id()), "missing lookup");
    require(fetched.get()._type == OT_NULL && fetched.words[1] == 0, "missing lookup resets output");
    top(vm, base, "ownership stack");
    // A depleted stack must not be padded out by the recovered trim helper.
    retdec_sqrat_trim_stack(address(vm), base + 2); top(vm, base, "trim does not grow");
    sq_pushinteger(vm, 123); retdec_sqrat_trim_stack(address(vm), base); top(vm, base, "trim pops excess");
}
void setters(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm);
    Pair root(vm); sq_pushroottable(vm); root.capture();
    require(retdec_sqrat_set_int(address(vm), root.data(), "bridge_i", -123), "set integer");
    require(retdec_sqrat_set_bool(address(vm), root.data(), "bridge_b", 7), "set true");
    require(retdec_sqrat_set_string(address(vm), root.data(), "bridge_s", nullptr), "null text becomes empty string");
    require(retdec_sqrat_raw_set_int(address(vm), root.data(), "bridge_i", 51), "raw integer");
    require(retdec_sqrat_raw_set_float(address(vm), root.data(), "bridge_f", -1.25f), "raw float");
    require(retdec_sqrat_raw_set_bool(address(vm), root.data(), "bridge_b", 0), "raw false");
    require(retdec_sqrat_raw_set_string(address(vm), root.data(), "bridge_s", "hello"), "raw string");
    evaluate(vm, "if (bridge_i != 51 || bridge_f != -1.25 || bridge_b || bridge_s != \"hello\") throw \"setters\";");
    Pair instance(vm);
    evaluate(vm, "bridge_reads <- 0;\nclass BridgeClass { value = 0; function _get(k) { ::bridge_reads++; return 77; } }\nreturn BridgeClass();", &instance);
    require(retdec_sqrat_raw_set_int(address(vm), instance.data(), "value", 7), "raw instance existing field");
    require(!retdec_sqrat_raw_set_int(address(vm), instance.data(), "missing", 1), "raw instance unknown field fails");
    // 2.2.2 sq_newslot on an instance returns SQ_OK without publishing a slot.
    // The old diagnostic readback triggered _get even though the setter did not.
    require(retdec_sqrat_set_int(address(vm), instance.data(), "pl", 42), "preserve instance newslot return");
    evaluate(vm, "if (bridge_reads != 0) throw \"diagnostics invoked _get\";");
    require(retdec_sqrat_set_pair(address(vm), root.data(), "bridge_instance", instance.data()), "publish instance");
    evaluate(vm, "if (bridge_instance.value != 7) throw \"raw field\";");
    Pair not_object(vm); sq_pushinteger(vm, 3); not_object.capture();
    require(!retdec_sqrat_raw_set_string(address(vm), not_object.data(), "key", "text"), "wrong receiver fails");
    require(!retdec_sqrat_set_native_closure(address(vm), root.data(), "bad", address(reinterpret_cast<void*>(captured)), not_object.data(), 2), "reject more captures than supplied");
    require(!retdec_sqrat_set_native_closure(address(vm), root.data(), "bad", address(reinterpret_cast<void*>(captured)), nullptr, 1), "reject missing capture");
    top(vm, base, "all setter success and failure stacks restored");
}
void delegates(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm);
    Pair first(vm), second(vm), null(vm), wrong(vm), result(vm), userdata(vm);
    require(retdec_sqrat_new_table(address(vm), first.data()), "first delegate table");
    require(retdec_sqrat_new_table(address(vm), second.data()), "second delegate table");
    retdec_sqrat_set_int(address(vm), second.data(), "inherited", 246);
    require(retdec_sqrat_set_delegate(address(vm), first.data(), second.data()), "table delegate source method");
    auto object = wrapper(vm, first);
    require(retdec_sqrat_get(address(object.data()), "inherited", result.id()), "inherited lookup");
    require(result.get()._type == OT_INTEGER && result.words[1] == 246, "delegate value");
    sq_throwerror(vm, "unchanged-error");
    require(!retdec_sqrat_set_delegate(address(vm), second.data(), first.data()), "reject delegate cycle");
    sq_getlasterror(vm); require(get_string(vm) == "unchanged-error", "cycle rejection preserves last error"); sq_pop(vm, 1);
    sq_pushinteger(vm, 1); wrong.capture();
    require(!retdec_sqrat_set_delegate(address(vm), first.data(), wrong.data()), "wrong delegate type");
    require(!retdec_sqrat_set_delegate(address(vm), wrong.data(), second.data()), "wrong receiver type");
    sq_newuserdata(vm, 4); userdata.capture();
    require(retdec_sqrat_set_delegate(address(vm), userdata.data(), second.data()), "userdata delegate source method");
    require(retdec_sqrat_set_delegate(address(vm), first.data(), null.data()), "clear table delegate");
    require(retdec_sqrat_set_delegate(address(vm), userdata.data(), null.data()), "clear userdata delegate");
    top(vm, base, "delegate paths balance stack");
}
void closures(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm);
    Pair root(vm), capture(vm); sq_pushroottable(vm); root.capture();
    sq_pushinteger(vm, 40); capture.capture();
    require(retdec_sqrat_set_native_closure(address(vm), root.data(), "bridge_capture", address(reinterpret_cast<void*>(captured)), capture.data(), 1), "register one captured pair");
    require(retdec_sqrat_set_native_closure(address(vm), root.data(), "bridge_plain", address(reinterpret_cast<void*>(plain)), nullptr, 0), "register no captures");
    require(retdec_sqrat_set_offset_closure(address(vm), root.data(), "bridge_offset", 30, address(reinterpret_cast<void*>(captured))), "register offset userdata");
    auto object = wrapper(vm, root);
    std::array<unsigned char, 8> source{}; store<int32_t>(source.data() + 1, 70);
    require(function_415550_this(address(object.data()), address("bridge_registered"), address(source.data() + 1), 4, address(reinterpret_cast<void*>(captured)), 0x100) == address(vm), "registration returns VM and masks static byte");
    evaluate(vm, "if (bridge_capture(2) != 42 || bridge_plain() != 99 || bridge_offset(12) != 42 || bridge_registered(3) != 73) throw \"closures\";");
    const auto before = capture_calls;
    Pair child(vm); sq_newthread(vm, 16); child.capture();
    HSQUIRRELVM thread = child.get()._unVal.pThread;
    evaluate(thread, "if (bridge_capture(2) != 42) throw \"child\";");
    require(capture_calls == before + 1, "child VM executes registered callback");
    Pair klass(vm), klass_function(vm);
    evaluate(vm, "return class {};", &klass);
    auto class_object = wrapper(vm, klass);
    require(function_415550_this(address(class_object.data()), address("static_capture"), address(source.data() + 1), 4, address(reinterpret_cast<void*>(captured)), 0x101) == address(vm), "class static flag byte");
    require(retdec_sqrat_set_pair(address(vm), root.data(), "BridgeStatic", klass.data()), "publish static class");
    evaluate(vm, "if (BridgeStatic.static_capture(2) != 72) throw \"static slot\";");
    top(vm, base, "closure registration stack");
}
void callback(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm);
    Pair environment(vm), closure(vm), failing(vm);
    sq_pushroottable(vm); environment.capture();
    require(SQ_SUCCEEDED(sq_compilebuffer(vm, "bridge_callback <- 123;", static_cast<SQInteger>(std::strlen("bridge_callback <- 123;")), "callback", SQFalse)), "compile callback");
    closure.capture();
    std::array<int32_t, 5> words{address(vm), environment.words[0], environment.words[1], closure.words[0], closure.words[1]};
    require(function_415810_this(address(words.data())) == address(vm), "call helper returns VM");
    top(vm, base, "successful callback stack");
    evaluate(vm, "if (bridge_callback != 123) throw \"callback env\";");
    const char* script = "throw \"callback-failure\";";
    require(SQ_SUCCEEDED(sq_compilebuffer(vm, script, static_cast<SQInteger>(std::strlen(script)), "failure", SQFalse)), "compile failing callback"); failing.capture();
    words[3] = failing.words[0]; words[4] = failing.words[1];
    sq_newclosure(vm, handler, 0); sq_seterrorhandler(vm);
    const int errors = error_handler_calls;
    g560 = 0;
    require(function_415810_this(address(words.data())) == address(vm), "failing call still returns VM");
    require(error_handler_calls == errors, "zero error handler flag");
    sq_getlasterror(vm); require(get_string(vm) == "callback-failure", "failed callback preserves source error"); sq_pop(vm, 1);
    g560 = 1; function_415810_this(address(words.data())); g560 = 0;
    require(error_handler_calls == errors + 1, "original handler flag honored");
    require(function_415810_this(0) == -1, "null callback guard");
    top(vm, base, "failure callback stack");
    sq_pushnull(vm); sq_seterrorhandler(vm);
}
}
int main() {
    try {
        bridge_test::Machine machine;
        for (int repeat = 0; repeat < 8; ++repeat) {
            virtual_entries(machine.get());
            ownership(machine.get()); setters(machine.get()); delegates(machine.get());
            closures(machine.get()); callback(machine.get());
        }
        std::puts("Sqrat source object/registration/callback contracts passed (8 repetitions)");
    } catch (const std::exception& e) { std::fprintf(stderr, "%s\n", e.what()); return 1; }
}
