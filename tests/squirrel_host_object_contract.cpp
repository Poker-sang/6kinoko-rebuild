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
struct SQVM *kinoko_primary_vm = nullptr;
const void* kinoko_squirrel_object_vtable(void) { return reinterpret_cast<const void*>(0x12345678); }
int32_t kinoko_native_void_type(void) { return 0x13572468; }
void kinoko_trace(const char*) {}
void kinoko_trace_i32(const char*, int32_t) {}
void kinoko_trace_squirrel_name(const char*, int32_t) {}
void kinoko_host_free_allocation(int32_t* p) { std::free(p); }
}

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message); // Runs in Release too.
}
SQVM* exchange_vm(SQVM* vm) {
    const auto previous = address(kinoko_primary_vm);
    kinoko_primary_vm = pointer<SQVM>((int32_t)(intptr_t)vm);
    return reinterpret_cast<SQVM*>(static_cast<uintptr_t>(previous));
}
class Machine final {
public:
    Machine() : vm_(pointer<SQVM>(((int32_t)(uintptr_t)kinoko_sq_open(64)))) {
        require(vm_ != nullptr, "open VM");
        kinoko_primary_vm = reinterpret_cast<SQVM*>(vm_);
        kinoko_sq_set_context_exchange(exchange_vm);
    }
    ~Machine() {
        kinoko_sq_set_context_exchange(nullptr);
        sq_close(vm_);
        kinoko_primary_vm = nullptr;
    }
    HSQUIRRELVM get() const { return vm_; }
    Machine(const Machine&) = delete;
    Machine& operator=(const Machine&) = delete;
private:
    HSQUIRRELVM vm_;
};
class HostObject final {
public:
    HostObject() { (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)(id()))); }
    ~HostObject() { (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(id()))); }
    HostObject(const HostObject&) = delete;
    HostObject& operator=(const HostObject&) = delete;
    int32_t id() { return address(words.data()); }
    ObjectView view() { return ObjectView(words.data()); }
    void capture(HSQUIRRELVM vm) { view().capture(vm, -1); sq_pop(vm, 1); }
    void table() { (int32_t*)(intptr_t)(kinoko_sqplus_object_new_table((void *)(intptr_t)(words.data()))); }
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
    require(kinoko_sqplus_object_is_null((void *)(intptr_t)(first.id())) == 1, "default null object");
    require(first.words[0] == address(kinoko_squirrel_object_vtable()), "original vtable identity");
    push_owned_userdata(vm);
    require(kinoko_sqplus_object_capture((void *)(intptr_t)(first.id()), -1) == OT_USERDATA, "capture returns type");
    sq_pop(vm, 1);
    require((int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(copy.words.data()), (const void *)(intptr_t)(first.id()))) == copy.words.data(), "copy construction result");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(intptr_t)(first.id()), (const void *)(intptr_t)(first.id()))) == first.id(), "self-assignment result");
    require(userdata_releases == released, "self-assignment must retain before release");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_reset((void *)(intptr_t)(first.id()))) == first.id() + 4, "reset returns payload address");
    require(userdata_releases == released, "copy keeps userdata alive");
    push_owned_userdata(vm);
    replacement.capture(vm);
    require((int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(intptr_t)(replacement.id()), (const void *)(intptr_t)(copy.id()))) == replacement.id(), "assign object result");
    require(userdata_releases == released + 1, "assignment releases previous value exactly once");
    kinoko_sqplus_object_reset((void *)(intptr_t)(copy.id()));
    require(userdata_releases == released + 1, "second owner keeps userdata alive");
    kinoko_sqplus_object_reset((void *)(intptr_t)(replacement.id()));
    require(userdata_releases == released + 2, "last external reference releases userdata");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(replacement.id()))) == replacement.id() + 4, "destructor result");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)(0))) == 0 && (int32_t)(intptr_t)(kinoko_sqplus_object_reset((void *)(intptr_t)(0))) == 0, "null constructor/reset");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(0))) == 0 && kinoko_sqplus_object_capture((void *)(intptr_t)(0), 1) == 0, "null destroy/capture");
    require_top(vm, top, "ownership preserves VM stack");

    // Real external references through deliberately unaligned legacy storage.
    for (int offset = 0; offset < 4; ++offset) {
        std::array<unsigned char, 20> buffer;
        buffer.fill(0xa5);
        const int32_t slot = address(buffer.data() + 1 + offset);
        require((int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)(slot))) == slot, "legacy default-constructor alias");
        sq_pushstring(vm, "unaligned-owner", -1);
        HSQOBJECT borrowed;
        sq_getstackobj(vm, -1, &borrowed);
        require((int32_t)(intptr_t)(kinoko_sqplus_object_construct_value((void *)(intptr_t)(slot), borrowed._type, data_bits(borrowed))) == slot, "pair constructor");
        sq_pop(vm, 1);
        require(kinoko_sqplus_object_size((void *)(intptr_t)(slot)) == 15, "unaligned object uses actual string length");
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(slot)));
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
    require(kinoko_sqplus_object_raw_set_object((void *)(intptr_t)(table.id()), (const void *)(intptr_t)(key.id()), (const void *)(intptr_t)(value.id())) == 1, "object-key rawset");
    require(kinoko_sqplus_object_get_index_integer((void *)(intptr_t)(table.id()), 2) == 37, "integer getter");
    require(kinoko_sqplus_object_get_index_integer((void *)(intptr_t)(table.id()), -999) == 0, "missing integer key returns zero");
    // Regression: these keys must never be dereferenced by trace argument evaluation.
    require(kinoko_sqplus_object_set_index_string((void *)(intptr_t)(table.id()), 1, (const char *)("text")) == 1, "small integer string key");
    require(std::string(pointer<const char>((int32_t)(intptr_t)(kinoko_sqplus_object_get_index_string((void *)(intptr_t)(table.id()), 1)))) == "text", "string getter");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_get_index_string((void *)(intptr_t)(table.id()), 2)) == 0, "wrong string type returns null");
    require(kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(table.id()), "answer", (const void *)(intptr_t)(value.id())) == 1, "string-key rawset");
    require((int32_t*)(intptr_t)(kinoko_sqplus_object_get_value((void *)(intptr_t)(table.id()), (void *)(intptr_t)(result.id()), "answer")) == result.words.data(), "named lookup result pointer");
    require(result.words[1] == OT_INTEGER && result.words[2] == 37, "named lookup copies entire pair");
    kinoko_sqplus_object_reset((void *)(intptr_t)(result.id()));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(table.id()), (void *)(intptr_t)(result.id()), "missing");
    require(result.words[1] == OT_NULL, "missing named lookup initializes null");
    require(kinoko_sqplus_object_exists((void *)(intptr_t)(table.id()), "answer") == 1 && kinoko_sqplus_object_exists((void *)(intptr_t)(table.id()), "absent") == 0, "slot existence");
    require(kinoko_sqplus_object_size((void *)(intptr_t)(table.id())) == 3, "table size");
    require(kinoko_sqplus_object_type((void *)(intptr_t)(table.id())) == OT_TABLE, "type getter");

    int native = 77;
    key.integer(vm, 3);
    sq_pushuserpointer(vm, &native);
    value.capture(vm);
    kinoko_sqplus_object_raw_set_object((void *)(intptr_t)(table.id()), (const void *)(intptr_t)(key.id()), (const void *)(intptr_t)(value.id()));
    require((int32_t)(intptr_t)(kinoko_sqplus_object_get_index_userpointer((void *)(intptr_t)(table.id()), 3)) == address(&native), "user-pointer getter");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_get_index_userpointer((void *)(intptr_t)(table.id()), 2)) == 0, "wrong user-pointer type");

    require((int32_t*)(intptr_t)(kinoko_sqplus_object_new_array((void *)(intptr_t)(array.words.data()), 0)) == array.words.data(), "array constructor");
    for (int i = 0; i < 4; ++i) {
        value.integer(vm, i + 10);
        require((int32_t)(intptr_t)(kinoko_sqplus_object_append((void *)(intptr_t)(array.id()), (const void *)(intptr_t)(value.id()))) == address(vm), "append returns VM address");
    }
    require(kinoko_sqplus_object_size((void *)(intptr_t)(array.id())) == 4, "array size");
    require(kinoko_sqplus_object_reverse((void *)(intptr_t)(array.id())) == 1, "array reverse");
    require(kinoko_sqplus_object_get_index_integer((void *)(intptr_t)(array.id()), 0) == 13 && kinoko_sqplus_object_get_index_integer((void *)(intptr_t)(array.id()), 3) == 10, "reverse contents");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_append((void *)(intptr_t)(table.id()), (const void *)(intptr_t)(value.id()))) == 0, "append rejected for nonarray");
    require_top(vm, top, "balanced object operations");

    require(kinoko_sqplus_object_begin_iteration((void *)(intptr_t)(array.id())) == 1, "begin iteration");
    require_top(vm, top + 2, "iteration keeps container and iterator");
    int count = 0, sum = 0;
    while (kinoko_sqplus_object_next(key.words.data(), value.words.data())) {
        require_top(vm, top + 2, "next retains iterator only");
        require(key.words[2] == count, "array iteration order");
        sum += value.words[2];
        require(++count <= 4, "iteration terminates");
    }
    require(count == 4 && sum == 46, "iteration values");
    require_top(vm, top + 2, "failed next preserves iterator stack");
    require(kinoko_sqplus_object_end_iteration() == address(vm), "end iteration returns VM");
    value.integer(vm, 9);
    require(kinoko_sqplus_object_begin_iteration((void *)(intptr_t)(value.id())) == 0, "integer not iterable");
    require_top(vm, top, "end iteration balances stack");
}

void userdata_delegates_and_types(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    HostObject table, delegate, captured_delegate, null_object, klass, instance, scalar;
    table.table(); delegate.table();
    int tag = 9, out = 0, out_tag = 0;
    require(kinoko_sqplus_object_new_userdata((void *)(intptr_t)(delegate.id()), (const char *)("payload"), 20, (void *)(&tag)) == 1, "create named userdata");
    require(kinoko_sqplus_object_get_userdata((void *)(intptr_t)(delegate.id()), (const char *)("payload"), (void *)(&out), (void *)(&out_tag)) == 1, "get named userdata");
    require(out != 0 && out_tag == address(&tag), "userdata and tag outputs");
    require(kinoko_sqplus_object_set_delegate((void *)(intptr_t)(table.id()), (const void *)(intptr_t)(delegate.id())) == 1, "set delegate");
    require(kinoko_sqplus_object_get_userdata((void *)(intptr_t)(table.id()), (const char *)("payload"), (void *)(&out), (void *)(intptr_t)(0)) == 1, "normal lookup follows delegate");
    require(kinoko_sqplus_object_raw_get_userdata((void *)(intptr_t)(table.id()), "payload", &out, (void *)(intptr_t)(0)) == 0, "raw lookup does not follow delegate");
    require(kinoko_sqplus_object_raw_get_userdata((void *)(intptr_t)(delegate.id()), "payload", &out, (void *)(intptr_t)(0)) == 1, "raw userdata lookup");
    require((int32_t*)(intptr_t)(kinoko_sqplus_object_get_delegate((void *)(intptr_t)(table.id()), (void *)(intptr_t)(captured_delegate.id()))) == captured_delegate.words.data(), "delegate result address");
    require(captured_delegate.words[1] == OT_TABLE && captured_delegate.words[2] == delegate.words[2], "delegate external reference");
    scalar.integer(vm, 5);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(table.id()), "scalar", (const void *)(intptr_t)(scalar.id()));
    out = 123; out_tag = 456;
    require(kinoko_sqplus_object_get_userdata((void *)(intptr_t)(table.id()), (const char *)("scalar"), (void *)(&out), (void *)(&out_tag)) == 1, "lookup-success result despite conversion failure");
    require(out == 123 && out_tag == 456, "failed userdata conversion leaves outputs unchanged");
    require(kinoko_sqplus_object_get_userdata((void *)(intptr_t)(table.id()), (const char *)("absent"), (void *)(&out), (void *)(intptr_t)(0)) == 0 && out == 123, "missing userdata output unchanged");
    require(kinoko_sqplus_object_set_delegate((void *)(intptr_t)(table.id()), (const void *)(intptr_t)(scalar.id())) == 0, "invalid delegate type rejected");
    require(kinoko_sqplus_object_set_delegate((void *)(intptr_t)(table.id()), (const void *)(intptr_t)(null_object.id())) == 1, "clear delegate");

    sq_newclass(vm, SQFalse);
    sq_settypetag(vm, -1, &tag);
    klass.capture(vm);
    require((int32_t)(intptr_t)(kinoko_sqplus_object_new_instance((void *)(intptr_t)(instance.id()), (const void *)(intptr_t)(klass.id()))) == instance.id(), "create instance without constructor");
    require(instance.words[1] == OT_INSTANCE && kinoko_sqplus_object_size((void *)(intptr_t)(klass.id())) == 0, "instance type and restricted size");
    require(kinoko_sqplus_object_set_instance((void *)(intptr_t)(instance.id()), (void *)(&out)) == 1, "set instance native pointer");
    require((int32_t)(intptr_t)(kinoko_sqplus_object_instance((void *)(intptr_t)(instance.id()), (void *)(&tag))) == address(&out), "get instance by type tag");
    int different_tag = 0;
    require((int32_t)(intptr_t)(kinoko_sqplus_object_instance((void *)(intptr_t)(instance.id()), (void *)(&different_tag))) == 0, "wrong tag returns null");
    sq_getlasterror(vm);
    require(sq_gettype(vm, -1) == OT_NULL, "wrong instance tag clears error as before");
    sq_pop(vm, 1);
    require(kinoko_sqplus_object_typetag((void *)(intptr_t)(instance.id()), &out_tag) == 1 && out_tag == address(&tag), "instance type tag");
    require(kinoko_sqplus_object_typetag((void *)(intptr_t)(klass.id()), &out_tag) == 1 && out_tag == address(&tag), "class type tag");
    out_tag = 88;
    require(kinoko_sqplus_object_typetag((void *)(intptr_t)(scalar.id()), &out_tag) == 0 && out_tag == 88, "failed object tag leaves output");
    kinoko_sqplus_object_reset((void *)(intptr_t)(instance.id()));
    kinoko_sqplus_object_new_instance((void *)(intptr_t)(instance.id()), (const void *)(intptr_t)(scalar.id()));
    require(instance.words[1] == OT_NULL, "invalid class produces null wrapper");
    sq_getlasterror(vm);
    require(sq_gettype(vm, -1) == OT_STRING, "source factory preserves invalid-class error");
    sq_pop(vm, 1);
    require_top(vm, top, "userdata/delegate/type operations balanced");
}

SQInteger host_callback(HSQUIRRELVM vm) {
    if (kinoko_primary_vm != reinterpret_cast<SQVM*>(vm)) return sq_throwerror(vm, "wrong host VM");
    HostObject table;
    table.table();
    if (kinoko_sqplus_object_set_index_string((void *)(intptr_t)(table.id()), 1, (const char *)("child")) != 1)
        return sq_throwerror(vm, "child rawset failed");
    sq_pushinteger(vm, kinoko_sqplus_object_size((void *)(intptr_t)(table.id())));
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
    require((int32_t)(intptr_t)(kinoko_sqplus_object_assign_thread((void *)(intptr_t)(thread_owner.id()), (struct SQVM *)(child))) == thread_owner.id(), "thread assignment result");
    require(thread_owner.words[1] == OT_THREAD && thread_owner.words[2] == address(child), "thread type pair");
    sq_pop(vm, 1); // External owner must now keep the child alive.
    require(SQ_SUCCEEDED(sq_compilebuffer(child, "return host_callback();", 23, "host-thread", SQFalse)), "child compile");
    sq_pushroottable(child);
    require(SQ_SUCCEEDED(kinoko_sq_call((SQVM*)(uintptr_t)(address(child)), 1, SQTrue, SQFalse)), "child invokes host wrapper on child VM");
    SQInteger result = 0;
    require(SQ_SUCCEEDED(sq_getinteger(child, -1, &result)) && result == 1, "child result");
    require(kinoko_primary_vm == reinterpret_cast<SQVM*>(vm), "host receiver restored");
    sq_settop(child, 0);
    // Exercise the old stack-reservation path while retaining the input thread.
    while (static_cast<SQUnsignedInteger>(vm->_top) < vm->_stack.size()) sq_pushinteger(vm, 17);
    const auto full_top = sq_gettop(vm);
    const auto capacity = vm->_stack.size();
    kinoko_sqplus_object_assign_thread((void *)(intptr_t)(thread_owner.id()), (struct SQVM *)(child));
    require_top(vm, full_top, "thread assignment after stack growth");
    require(vm->_stack.size() > capacity, "thread push reserves source VM stack");
    sq_settop(vm, top);
    require((int32_t)(intptr_t)(kinoko_sqplus_object_assign_thread((void *)(intptr_t)(thread_owner.id()), (struct SQVM *)(intptr_t)(0))) == thread_owner.id(), "clear thread wrapper");
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
    if (sq_gettop(vm) != 5 || (int32_t)(intptr_t)(kinoko_native_target_from_userdata((struct SQVM *)(intptr_t)(id))) != callback_identity ||
        (int32_t)(intptr_t)(kinoko_native_callback_from_stack((struct SQVM *)(intptr_t)(id))) != callback_identity ||
        !kinoko_native_integer_arg((struct SQVM *)(intptr_t)(id), 2, &integer) || integer != 19 ||
        !kinoko_native_float_arg((struct SQVM *)(intptr_t)(id), 3, &number) || number != 2.5f ||
        !kinoko_native_string_arg((struct SQVM *)(intptr_t)(id), 4, &text) || std::string(pointer<const char>(text)) != "ok" ||
        !kinoko_native_value_pair((struct SQVM *)(intptr_t)(id), 2, borrowed) || borrowed[0] != OT_INTEGER || borrowed[1] != 19)
        return sq_throwerror(vm, "native argument contract");
    HostObject owned;
    if (!kinoko_squirrel_pair_from_stack((struct SQVM *)(intptr_t)(id), 4, owned.words.data()) || kinoko_sqplus_object_size((void *)(intptr_t)(owned.id())) != 2)
        return sq_throwerror(vm, "owning argument contract");
    sq_pushinteger(vm, integer + static_cast<int>(number));
    return 1;
}
void native_arguments(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    const auto id = address(vm);
    require((int32_t)(intptr_t)(kinoko_native_target_from_userdata((struct SQVM *)(intptr_t)(0))) == 0 && (int32_t)(intptr_t)(kinoko_native_callback_from_stack((struct SQVM *)(intptr_t)(0))) == 0, "null native VM");
    require((int32_t)(intptr_t)(kinoko_native_target_from_userdata((struct SQVM *)(intptr_t)(id))) == 0 && (int32_t)(intptr_t)(kinoko_native_callback_from_stack((struct SQVM *)(intptr_t)(id))) == 0, "empty native stack");
    int32_t integer = 91, text = 92, pair[2] = {93, 94};
    float number = 9.5f;
    sq_pushfloat(vm, 2.5f);
    require(!kinoko_native_integer_arg((struct SQVM *)(intptr_t)(id), -1, &integer) && integer == 91, "do not coerce float to native integer");
    require(!kinoko_native_string_arg((struct SQVM *)(intptr_t)(id), -1, &text) && text == 92, "wrong string argument unchanged");
    require(kinoko_native_float_arg((struct SQVM *)(intptr_t)(id), -1, &number) && number == 2.5f, "float argument");
    require(!kinoko_native_float_arg((struct SQVM *)(intptr_t)(id), -1, nullptr), "null float output rejected");
    // The old first-call diagnostic used an uninitialized userdata pointer here.
    require((int32_t)(intptr_t)(kinoko_native_target_from_userdata((struct SQVM *)(intptr_t)(id))) == 0 && (int32_t)(intptr_t)(kinoko_native_callback_from_stack((struct SQVM *)(intptr_t)(id))) == 0, "wrong userdata type safe in diagnostics");
    require(!kinoko_native_value_pair((struct SQVM *)(intptr_t)(id), 0, pair) && !kinoko_native_value_pair((struct SQVM *)(intptr_t)(id), 2, pair), "argument index bounds");
    require(pair[0] == 93 && pair[1] == 94, "invalid borrowed pair unchanged");
    require(!kinoko_native_value_pair((struct SQVM *)(intptr_t)(0), 1, pair) && !kinoko_native_value_pair((struct SQVM *)(intptr_t)(id), 1, nullptr), "borrowed output guards");
    HostObject invalid;
    require(!kinoko_squirrel_pair_from_stack((struct SQVM *)(intptr_t)(id), 2, invalid.words.data()) && invalid.words[1] == OT_NULL, "owning index bounds");
    sq_pop(vm, 1);
    sq_pushinteger(vm, 11);
    require(!kinoko_native_float_arg((struct SQVM *)(intptr_t)(id), -1, &number) && number == 2.5f, "do not coerce integer to native float");
    require(kinoko_native_integer_arg((struct SQVM *)(intptr_t)(id), -1, &integer) && integer == 11, "strict integer argument");
    require(!kinoko_native_integer_arg((struct SQVM *)(intptr_t)(id), -1, nullptr), "null integer output rejected");
    sq_getlasterror(vm);
    const SQChar* message = nullptr;
    require(SQ_SUCCEEDED(sq_getstring(vm, -1, &message)) && std::string(message) == "Incorrect function argument", "original native error text");
    sq_pop(vm, 2);
    sq_pushstring(vm, "native-text", -1);
    require(kinoko_native_string_arg((struct SQVM *)(intptr_t)(id), -1, &text) && std::string(pointer<const char>(text)) == "native-text", "strict string argument");
    require(!kinoko_native_string_arg((struct SQVM *)(intptr_t)(id), -1, nullptr), "null string output rejected");
    sq_pop(vm, 1);
    void* payload = sq_newuserdata(vm, sizeof(int32_t));
    std::memcpy(payload, &callback_identity, sizeof(callback_identity));
    require((int32_t)(intptr_t)(kinoko_native_callback_from_stack((struct SQVM *)(intptr_t)(id))) == callback_identity, "untagged callback payload");
    sq_settypetag(vm, -1, &integer);
    require((int32_t)(intptr_t)(kinoko_native_callback_from_stack((struct SQVM *)(intptr_t)(id))) == 0, "callback tag must be null");
    require((int32_t)(intptr_t)(kinoko_native_target_from_userdata((struct SQVM *)(intptr_t)(id))) == callback_identity, "target wrapper accepts original tagged payload");
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
    require(SQ_SUCCEEDED(kinoko_sq_call((SQVM*)(uintptr_t)(id), 1, SQTrue, SQFalse)), "execute real closure with captured userdata");
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
    require(kinoko_native_instance_create((SQVM*)(uintptr_t)(address(vm)), (const char*)(uintptr_t)(address("HostNative")), (void*)(uintptr_t)(address(&native_data)), (SQRELEASEHOOK)(uintptr_t)(address(reinterpret_cast<const void*>(&release_instance)))) == 1, "create native instance");
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
    require(kinoko_native_instance_create((SQVM*)(uintptr_t)(address(vm)), (const char*)(uintptr_t)(address("MissingNative")), (void*)(uintptr_t)(address(&native_data)), (SQRELEASEHOOK)(uintptr_t)(0)) == 0, "missing native class fails");
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
