#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_object.h"
#include "kinoko/squirrel_source_runtime.h"
#include "sqpcheader.h"
#include "sqvm.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

using namespace kinoko::script;

// These are the game's narrow host services, not mocks for Squirrel. Every
// object/VM operation below executes the actual vendored 2.2.2 implementation.
extern "C" {
char* g644 = nullptr;
int32_t kinoko_squirrel_object_vtable(void) { return 0x12345678; }
int32_t kinoko_native_void_type(void) { return 0x13572468; }
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
void _3f__3f_3_40_YAXPAX_40_Z(int32_t* p) { std::free(p); }
}

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message); // Runs in Release too.
}
int32_t exchange_vm(int32_t vm) {
    const auto previous = address(g644);
    g644 = pointer<char>(vm);
    return previous;
}
class Machine final {
public:
    Machine() : vm_(pointer<SQVM>(kinoko_sq_open(64))) {
        require(vm_ != nullptr, "open VM");
        g644 = reinterpret_cast<char*>(vm_);
        kinoko_sq_set_context_exchange(exchange_vm);
    }
    ~Machine() {
        kinoko_sq_set_context_exchange(nullptr);
        sq_close(vm_);
        g644 = nullptr;
    }
    HSQUIRRELVM get() const { return vm_; }
    Machine(const Machine&) = delete;
    Machine& operator=(const Machine&) = delete;
private:
    HSQUIRRELVM vm_;
};
class HostObject final {
public:
    HostObject() { function_4a94e0_this(id()); }
    ~HostObject() { function_4a9d70_this(id()); }
    HostObject(const HostObject&) = delete;
    HostObject& operator=(const HostObject&) = delete;
    int32_t id() { return address(words.data()); }
    ObjectView view() { return ObjectView(words.data()); }
    void capture(HSQUIRRELVM vm) { view().capture(vm, -1); sq_pop(vm, 1); }
    void table() { function_4a91c0_this(words.data()); }
    void integer(HSQUIRRELVM vm, int value) { sq_pushinteger(vm, value); capture(vm); }
    void string(HSQUIRRELVM vm, const char* value) { sq_pushstring(vm, value, -1); capture(vm); }
    std::array<int32_t, 3> words{};
};
int userdata_releases = 0, instance_releases = 0, constructor_calls = 0;
SQInteger release_userdata(SQUserPointer, SQInteger) { ++userdata_releases; return 0; }
SQInteger release_instance(SQUserPointer, SQInteger) { ++instance_releases; return 0; }
SQInteger constructor(HSQUIRRELVM) { ++constructor_calls; return 0; }
void push_owned_userdata(HSQUIRRELVM vm) {
    sq_newuserdata(vm, 32);
    sq_setreleasehook(vm, -1, release_userdata);
}
void require_top(HSQUIRRELVM vm, SQInteger top, const char* name) {
    require(sq_gettop(vm) == top, name);
}

void ownership(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    const int released = userdata_releases;
    HostObject first, copy, replacement;
    require(function_4a96c0_this(first.id()) == 1, "default null object");
    require(first.words[0] == kinoko_squirrel_object_vtable(), "original vtable identity");
    push_owned_userdata(vm);
    require(function_4a9660_this(first.id(), -1) == OT_USERDATA, "capture returns type");
    sq_pop(vm, 1);
    require(function_4a9500_this(copy.words.data(), first.id()) == copy.words.data(), "copy construction result");
    require(function_4a95c0_this(first.id(), first.id()) == first.id(), "self-assignment result");
    require(userdata_releases == released, "self-assignment must retain before release");
    require(function_4a9570_this(first.id()) == first.id() + 4, "reset returns payload address");
    require(userdata_releases == released, "copy keeps userdata alive");
    push_owned_userdata(vm);
    replacement.capture(vm);
    require(function_4a95c0_this(replacement.id(), copy.id()) == replacement.id(), "assign object result");
    require(userdata_releases == released + 1, "assignment releases previous value exactly once");
    function_4a9570_this(copy.id());
    require(userdata_releases == released + 1, "second owner keeps userdata alive");
    function_4a9570_this(replacement.id());
    require(userdata_releases == released + 2, "last external reference releases userdata");
    require(function_4a9d70_this(replacement.id()) == replacement.id() + 4, "destructor result");
    require(function_4a94e0_this(0) == 0 && function_4a9570_this(0) == 0, "null constructor/reset");
    require(function_4a9d70_this(0) == 0 && function_4a9660_this(0, 1) == 0, "null destroy/capture");
    require_top(vm, top, "ownership preserves VM stack");

    // Real external references through deliberately unaligned legacy storage.
    for (int offset = 0; offset < 4; ++offset) {
        std::array<unsigned char, 20> buffer;
        buffer.fill(0xa5);
        const int32_t slot = address(buffer.data() + 1 + offset);
        require(retdec_msvc_0_Init_locks_std__QAE_XZ5_this(slot) == slot, "legacy default-constructor alias");
        sq_pushstring(vm, "unaligned-owner", -1);
        HSQOBJECT borrowed;
        sq_getstackobj(vm, -1, &borrowed);
        require(function_4a9540_this(slot, borrowed._type, data_bits(borrowed)) == slot, "pair constructor");
        sq_pop(vm, 1);
        require(function_4a96d0(slot) == 15, "unaligned object uses actual string length");
        function_4a9d70_this(slot);
        for (int i = 0; i < 20; ++i)
            if (i < 1 + offset || i >= 13 + offset) require(buffer[i] == 0xa5, "wrapper canary");
    }
    require_top(vm, top, "unaligned owner stack balance");
}

void tables_arrays_and_iteration(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    HostObject table, key, value, result, array;
    table.table();
    key.integer(vm, 2);
    value.integer(vm, 37);
    require(function_4a97b0_this(table.id(), key.id(), value.id()) == 1, "object-key rawset");
    require(function_4a9a40_this(table.id(), 2) == 37, "integer getter");
    require(function_4a9a40_this(table.id(), -999) == 0, "missing integer key returns zero");
    // Regression: these keys must never be dereferenced by trace argument evaluation.
    require(function_4a9730_this(table.id(), 1, address("text")) == 1, "small integer string key");
    require(std::string(pointer<const char>(function_4a9ac0_this(table.id(), 1))) == "text", "string getter");
    require(function_4a9ac0_this(table.id(), 2) == 0, "wrong string type returns null");
    require(function_4a9840_this(table.id(), "answer", value.id()) == 1, "string-key rawset");
    require(function_4aa3a0_this(table.id(), result.id(), "answer") == result.words.data(), "named lookup result pointer");
    require(result.words[1] == OT_INTEGER && result.words[2] == 37, "named lookup copies entire pair");
    function_4a9570_this(result.id());
    function_4aa3a0_this(table.id(), result.id(), "missing");
    require(result.words[1] == OT_NULL, "missing named lookup initializes null");
    require(function_4aa1a0(table.id(), "answer") == 1 && function_4aa1a0(table.id(), "absent") == 0, "slot existence");
    require(function_4a96d0(table.id()) == 3, "table size");
    require(function_4a9a30_this(table.id()) == OT_TABLE, "type getter");

    int native = 77;
    key.integer(vm, 3);
    sq_pushuserpointer(vm, &native);
    value.capture(vm);
    function_4a97b0_this(table.id(), key.id(), value.id());
    require(function_4aa000_this(table.id(), 3) == address(&native), "user-pointer getter");
    require(function_4aa000_this(table.id(), 2) == 0, "wrong user-pointer type");

    require(function_4a92e0_this(array.words.data(), 0) == array.words.data(), "array constructor");
    for (int i = 0; i < 4; ++i) {
        value.integer(vm, i + 10);
        require(function_4a9600_this(array.id(), value.id()) == address(vm), "append returns VM address");
    }
    require(function_4a96d0(array.id()) == 4, "array size");
    require(function_4a99f0(array.id()) == 1, "array reverse");
    require(function_4a9a40_this(array.id(), 0) == 13 && function_4a9a40_this(array.id(), 3) == 10, "reverse contents");
    require(function_4a9600_this(table.id(), value.id()) == 0, "append rejected for nonarray");
    require_top(vm, top, "balanced object operations");

    require(function_4a9c10_this(array.id()) == 1, "begin iteration");
    require_top(vm, top + 2, "iteration keeps container and iterator");
    int count = 0, sum = 0;
    while (function_4a9c60(key.words.data(), value.words.data())) {
        require_top(vm, top + 2, "next retains iterator only");
        require(key.words[2] == count, "array iteration order");
        sum += value.words[2];
        require(++count <= 4, "iteration terminates");
    }
    require(count == 4 && sum == 46, "iteration values");
    require_top(vm, top + 2, "failed next preserves iterator stack");
    require(function_4a9d50() == address(vm), "end iteration returns VM");
    value.integer(vm, 9);
    require(function_4a9c10_this(value.id()) == 0, "integer not iterable");
    require_top(vm, top, "end iteration balances stack");
}

void userdata_delegates_and_types(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    HostObject table, delegate, captured_delegate, null_object, klass, instance, scalar;
    table.table(); delegate.table();
    int tag = 9, out = 0, out_tag = 0;
    require(function_4a9950(delegate.id(), address("payload"), 20, address(&tag)) == 1, "create named userdata");
    require(function_4aa080(delegate.id(), address("payload"), address(&out), address(&out_tag)) == 1, "get named userdata");
    require(out != 0 && out_tag == address(&tag), "userdata and tag outputs");
    require(function_4a9f60(table.id(), delegate.id()) == 1, "set delegate");
    require(function_4aa080(table.id(), address("payload"), address(&out), 0) == 1, "normal lookup follows delegate");
    require(retdec_function_4aa110_this(table.id(), "payload", &out, 0) == 0, "raw lookup does not follow delegate");
    require(retdec_function_4aa110_this(delegate.id(), "payload", &out, 0) == 1, "raw userdata lookup");
    require(function_4aa210_this(table.id(), captured_delegate.id()) == captured_delegate.words.data(), "delegate result address");
    require(captured_delegate.words[1] == OT_TABLE && captured_delegate.words[2] == delegate.words[2], "delegate external reference");
    scalar.integer(vm, 5);
    function_4a9840_this(table.id(), "scalar", scalar.id());
    out = 123; out_tag = 456;
    require(function_4aa080(table.id(), address("scalar"), address(&out), address(&out_tag)) == 1, "lookup-success result despite conversion failure");
    require(out == 123 && out_tag == 456, "failed userdata conversion leaves outputs unchanged");
    require(function_4aa080(table.id(), address("absent"), address(&out), 0) == 0 && out == 123, "missing userdata output unchanged");
    require(function_4a9f60(table.id(), scalar.id()) == 0, "invalid delegate type rejected");
    require(function_4a9f60(table.id(), null_object.id()) == 1, "clear delegate");

    sq_newclass(vm, SQFalse);
    sq_settypetag(vm, -1, &tag);
    klass.capture(vm);
    require(function_4a90c0_this(instance.id(), klass.id()) == instance.id(), "create instance without constructor");
    require(instance.words[1] == OT_INSTANCE && function_4a96d0(klass.id()) == 0, "instance type and restricted size");
    require(function_4a9bb0_this(instance.id(), address(&out)) == 1, "set instance native pointer");
    require(function_4a9b40_this(instance.id(), address(&tag)) == address(&out), "get instance by type tag");
    int different_tag = 0;
    require(function_4a9b40_this(instance.id(), address(&different_tag)) == 0, "wrong tag returns null");
    sq_getlasterror(vm);
    require(sq_gettype(vm, -1) == OT_NULL, "wrong instance tag clears error as before");
    sq_pop(vm, 1);
    require(function_4a9d30_this(instance.id(), &out_tag) == 1 && out_tag == address(&tag), "instance type tag");
    require(function_4a9d30_this(klass.id(), &out_tag) == 1 && out_tag == address(&tag), "class type tag");
    out_tag = 88;
    require(function_4a9d30_this(scalar.id(), &out_tag) == 0 && out_tag == 88, "failed object tag leaves output");
    function_4a9570_this(instance.id());
    function_4a90c0_this(instance.id(), scalar.id());
    require(instance.words[1] == OT_NULL, "invalid class produces null wrapper");
    sq_getlasterror(vm);
    require(sq_gettype(vm, -1) == OT_STRING, "source factory preserves invalid-class error");
    sq_pop(vm, 1);
    require_top(vm, top, "userdata/delegate/type operations balanced");
}

SQInteger host_callback(HSQUIRRELVM vm) {
    if (g644 != reinterpret_cast<char*>(vm)) return sq_throwerror(vm, "wrong host VM");
    HostObject table;
    table.table();
    if (function_4a9730_this(table.id(), 1, address("child")) != 1)
        return sq_throwerror(vm, "child rawset failed");
    sq_pushinteger(vm, function_4a96d0(table.id()));
    return 1;
}
void threads(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    sq_pushroottable(vm);
    sq_pushstring(vm, "host_callback", -1);
    sq_newclosure(vm, host_callback, 0);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "register host callback");
    sq_pop(vm, 1);
    HostObject thread_owner, weak;
    auto* child = sq_newthread(vm, 32);
    sq_weakref(vm, -1);
    weak.capture(vm);
    require(function_4a9e30_this(thread_owner.id(), address(child)) == thread_owner.id(), "thread assignment result");
    require(thread_owner.words[1] == OT_THREAD && thread_owner.words[2] == address(child), "thread type pair");
    sq_pop(vm, 1); // External owner must now keep the child alive.
    require(SQ_SUCCEEDED(sq_compilebuffer(child, "return host_callback();", 23, "host-thread", SQFalse)), "child compile");
    sq_pushroottable(child);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(child), 1, SQTrue, SQFalse)), "child invokes host wrapper on child VM");
    SQInteger result = 0;
    require(SQ_SUCCEEDED(sq_getinteger(child, -1, &result)) && result == 1, "child result");
    require(g644 == reinterpret_cast<char*>(vm), "host receiver restored");
    sq_settop(child, 0);
    // Exercise the old stack-reservation path while retaining the input thread.
    while (static_cast<SQUnsignedInteger>(vm->_top) < vm->_stack.size()) sq_pushinteger(vm, 17);
    const auto full_top = sq_gettop(vm);
    const auto capacity = vm->_stack.size();
    function_4a9e30_this(thread_owner.id(), address(child));
    require_top(vm, full_top, "thread assignment after stack growth");
    require(vm->_stack.size() > capacity, "thread push reserves source VM stack");
    sq_settop(vm, top);
    require(function_4a9e30_this(thread_owner.id(), 0) == thread_owner.id(), "clear thread wrapper");
    weak.view().push(vm);
    require(SQ_SUCCEEDED(sq_getweakrefval(vm, -1)) && sq_gettype(vm, -1) == OT_NULL, "released child invalidates weak reference");
    sq_pop(vm, 2);
    require_top(vm, top, "thread operations balanced");
}

constexpr int32_t callback_identity = 0x24682468;
SQInteger argument_callback(HSQUIRRELVM vm) {
    const auto id = address(vm);
    int32_t integer = 0, text = 0, borrowed[2] = {};
    float number = 0;
    if (sq_gettop(vm) != 5 || retdec_native_target_from_userdata(id) != callback_identity ||
        retdec_native_callback_from_stack(id) != callback_identity ||
        !retdec_native_integer_arg(id, 2, &integer) || integer != 19 ||
        !retdec_native_float_arg(id, 3, &number) || number != 2.5f ||
        !retdec_native_string_arg(id, 4, &text) || std::string(pointer<const char>(text)) != "ok" ||
        !retdec_native_value_pair(id, 2, borrowed) || borrowed[0] != OT_INTEGER || borrowed[1] != 19)
        return sq_throwerror(vm, "native argument contract");
    HostObject owned;
    if (!retdec_squirrel_pair_from_stack(id, 4, owned.words.data()) || function_4a96d0(owned.id()) != 2)
        return sq_throwerror(vm, "owning argument contract");
    sq_pushinteger(vm, integer + static_cast<int>(number));
    return 1;
}
void native_arguments(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    const auto id = address(vm);
    require(retdec_native_target_from_userdata(0) == 0 && retdec_native_callback_from_stack(0) == 0, "null native VM");
    require(retdec_native_target_from_userdata(id) == 0 && retdec_native_callback_from_stack(id) == 0, "empty native stack");
    int32_t integer = 91, text = 92, pair[2] = {93, 94};
    float number = 9.5f;
    sq_pushfloat(vm, 2.5f);
    require(!retdec_native_integer_arg(id, -1, &integer) && integer == 91, "do not coerce float to native integer");
    require(!retdec_native_string_arg(id, -1, &text) && text == 92, "wrong string argument unchanged");
    require(retdec_native_float_arg(id, -1, &number) && number == 2.5f, "float argument");
    require(!retdec_native_float_arg(id, -1, nullptr), "null float output rejected");
    // The old first-call diagnostic used an uninitialized userdata pointer here.
    require(retdec_native_target_from_userdata(id) == 0 && retdec_native_callback_from_stack(id) == 0, "wrong userdata type safe in diagnostics");
    require(!retdec_native_value_pair(id, 0, pair) && !retdec_native_value_pair(id, 2, pair), "argument index bounds");
    require(pair[0] == 93 && pair[1] == 94, "invalid borrowed pair unchanged");
    require(!retdec_native_value_pair(0, 1, pair) && !retdec_native_value_pair(id, 1, nullptr), "borrowed output guards");
    HostObject invalid;
    require(!retdec_squirrel_pair_from_stack(id, 2, invalid.words.data()) && invalid.words[1] == OT_NULL, "owning index bounds");
    sq_pop(vm, 1);
    sq_pushinteger(vm, 11);
    require(!retdec_native_float_arg(id, -1, &number) && number == 2.5f, "do not coerce integer to native float");
    require(retdec_native_integer_arg(id, -1, &integer) && integer == 11, "strict integer argument");
    require(!retdec_native_integer_arg(id, -1, nullptr), "null integer output rejected");
    sq_getlasterror(vm);
    const SQChar* message = nullptr;
    require(SQ_SUCCEEDED(sq_getstring(vm, -1, &message)) && std::string(message) == "Incorrect function argument", "original native error text");
    sq_pop(vm, 2);
    sq_pushstring(vm, "native-text", -1);
    require(retdec_native_string_arg(id, -1, &text) && std::string(pointer<const char>(text)) == "native-text", "strict string argument");
    require(!retdec_native_string_arg(id, -1, nullptr), "null string output rejected");
    sq_pop(vm, 1);
    void* payload = sq_newuserdata(vm, sizeof(int32_t));
    std::memcpy(payload, &callback_identity, sizeof(callback_identity));
    require(retdec_native_callback_from_stack(id) == callback_identity, "untagged callback payload");
    sq_settypetag(vm, -1, &integer);
    require(retdec_native_callback_from_stack(id) == 0, "callback tag must be null");
    require(retdec_native_target_from_userdata(id) == callback_identity, "target wrapper accepts original tagged payload");
    sq_pop(vm, 1);

    sq_pushroottable(vm);
    sq_pushstring(vm, "native_arguments", -1);
    payload = sq_newuserdata(vm, sizeof(int32_t));
    std::memcpy(payload, &callback_identity, sizeof(callback_identity));
    sq_newclosure(vm, argument_callback, 1);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "register native argument closure");
    sq_pop(vm, 1);
    const char program[] = "return native_arguments(19, 2.5, \"ok\");";
    require(SQ_SUCCEEDED(sq_compilebuffer(vm, program, sizeof(program) - 1, "native-arguments", SQFalse)), "compile native argument call");
    sq_pushroottable(vm);
    require(SQ_SUCCEEDED(kinoko_sq_call(id, 1, SQTrue, SQFalse)), "execute real closure with captured userdata");
    SQInteger result = 0;
    require(SQ_SUCCEEDED(sq_getinteger(vm, -1, &result)) && result == 21, "native argument result");
    // 2.2.2 sq_call pops its arguments, but retains the called closure.
    require_top(vm, top + 2, "source call leaves closure and result");
    require(sq_gettype(vm, -2) == OT_CLOSURE, "called closure retained");
    sq_pop(vm, 2);
    sq_reseterror(vm);
    require_top(vm, top, "native argument helpers preserve stack");
}

void native_instances(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    HostObject base, excluded;
    int base_tag = 1, excluded_tag = 2, type_tag = 3, native_data = 99;
    sq_newclass(vm, SQFalse); sq_settypetag(vm, -1, &base_tag); base.capture(vm);
    sq_newclass(vm, SQFalse); sq_settypetag(vm, -1, &excluded_tag); excluded.capture(vm);
    sq_pushroottable(vm);
    sq_pushstring(vm, "HostNative", -1);
    sq_newclass(vm, SQFalse);
    sq_settypetag(vm, -1, &type_tag);
    sq_pushstring(vm, "__ot", -1); sq_pushnull(vm);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "declare native object map");
    sq_pushstring(vm, "__ca", -1); sq_newarray(vm, 0);
    base.view().push(vm); sq_arrayappend(vm, -2);
    excluded.view().push(vm); sq_arrayappend(vm, -2);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "declare class array");
    sq_pushstring(vm, "constructor", -1); sq_newclosure(vm, constructor, 0);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "declare constructor");
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "register class");
    sq_pop(vm, 1);
    const int before_release = instance_releases, before_constructor = constructor_calls;
    require(kinoko_native_instance_create(address(vm), address("HostNative"), address(&native_data),
        address(reinterpret_cast<const void*>(&release_instance)), kinoko_squirrel_object_vtable()) == 1, "create native instance");
    require_top(vm, top + 1, "native creation leaves exactly the instance");
    require(constructor_calls == before_constructor, "native creation must NOT call script constructor");
    SQUserPointer actual = nullptr;
    require(SQ_SUCCEEDED(sq_getinstanceup(vm, -1, &actual, &type_tag)) && actual == &native_data, "native pointer/type association");
    sq_pushstring(vm, "__ot", -1);
    require(SQ_SUCCEEDED(sq_get(vm, -2)), "native map present");
    for (int32_t key : {kinoko_native_void_type(), address(&base_tag)}) {
        sq_pushinteger(vm, key);
        require(SQ_SUCCEEDED(sq_rawget(vm, -2)), "base/void mapping present");
        require(SQ_SUCCEEDED(sq_getuserpointer(vm, -1, &actual)) && actual == &native_data, "base/void pointer value");
        sq_pop(vm, 1);
    }
    sq_pushinteger(vm, address(&excluded_tag));
    require(SQ_FAILED(sq_rawget(vm, -2)), "final __ca element stays excluded");
    sq_pop(vm, 1); // map
    require(instance_releases == before_release, "stack still owns native instance");
    sq_pop(vm, 1);
    require(instance_releases == before_release + 1, "native release hook executes exactly once");
    require(kinoko_native_instance_create(address(vm), address("MissingNative"), address(&native_data), 0,
        kinoko_squirrel_object_vtable()) == 0, "missing native class fails");
    require_top(vm, top, "native failure restores incoming stack");
}
}

int main() {
    try {
        Machine machine;
        auto* vm = machine.get();
        for (int pass = 0; pass < 8; ++pass) {
            native_arguments(vm);
            ownership(vm);
            tables_arrays_and_iteration(vm);
            userdata_delegates_and_types(vm);
            threads(vm);
            native_instances(vm);
            require_top(vm, 0, "whole pass stack balance");
            sq_collectgarbage(vm);
        }
        std::puts("PASS: external ownership, typed host objects, integer keys, iteration, userdata, delegates, tags, child callbacks and native instances");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Host object contract failed: %s\n", error.what());
        return 1;
    }
}
