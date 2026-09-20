#include "kinoko/upstream_bindings.hpp"
#include <sqplus.h>
#include <sqrat/sqratTable.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

namespace up = kinoko::script::upstream;
namespace {
void require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
HSQOBJECT empty() { HSQOBJECT value; sq_resetobject(&value); return value; }
bool same(HSQOBJECT a, HSQOBJECT b) {
    return a._type == b._type && a._unVal.pRefCounted == b._unVal.pRefCounted;
}
struct Machine {
    HSQUIRRELVM vm = sq_open(64);
    Machine() { require(vm != nullptr, "sq_open"); }
    ~Machine() { sq_close(vm); }
};
thread_local int released = 0;
thread_local HSQUIRRELVM expected_context = nullptr;
thread_local HSQUIRRELVM nested_context = nullptr;
thread_local bool contexts_ok = true;
SQInteger release_hook(SQUserPointer, SQInteger) {
    ++released;
    contexts_ok = contexts_ok && SquirrelVM::GetVMPtr() == expected_context;
    if (nested_context) {
        auto* parent = SquirrelVM::GetVMPtr();
        auto root = up::sqrat_root(nested_context);
        const auto owned = up::sqplus_assign(nested_context, empty(), root);
        up::sqplus_release(nested_context, owned);
        up::sqrat_release(nested_context, root);
        contexts_ok = contexts_ok && SquirrelVM::GetVMPtr() == parent;
    }
    return 0;
}
HSQOBJECT owned_userdata(HSQUIRRELVM vm) {
    sq_newuserdata(vm, 8);
    sq_setreleasehook(vm, -1, release_hook);
    HSQOBJECT value; sq_getstackobj(vm, -1, &value);
    value = up::sqplus_assign(vm, empty(), value);
    sq_pop(vm, 1);
    return value;
}
void objects(HSQUIRRELVM vm, HSQUIRRELVM other) {
    const auto top = sq_gettop(vm);
    const auto start = released;
    expected_context = vm;
    nested_context = other;
    require(SquirrelVM::GetVMPtr() == nullptr, "no borrowed VM outside scope");
    auto value = owned_userdata(vm);
    value = up::sqplus_assign(vm, value, value);
    require(released == start, "self assignment retains before release");
    up::sqplus_retain(vm, value);
    up::sqplus_release(vm, value);
    require(released == start, "copy owns an external reference");
    up::sqplus_release(vm, value);
    require(released == start + 1, "last SqPlus reference released exactly once");
    require(contexts_ok && SquirrelVM::GetVMPtr() == nullptr, "reentrant VM context restored");

    value = owned_userdata(vm);
    up::sqrat_retain(vm, value);
    up::sqplus_release(vm, value);
    require(released == start + 1, "Sqrat retains actual VM external reference");
    // Sqrat receives its VM directly and does not need the SqPlus TLS context.
    expected_context = nullptr;
    up::sqrat_release(vm, value);
    require(released == start + 2 && contexts_ok, "Sqrat releases once without hidden VM scope");
    auto root = up::sqrat_root(vm);
    auto table = up::sqrat_table(vm);
    require(root._type == OT_TABLE && table._type == OT_TABLE, "upstream root and new table");
    sq_pushroottable(vm);
    HSQOBJECT actual_root; sq_getstackobj(vm, -1, &actual_root);
    sq_pop(vm, 1);
    require(same(root, actual_root) && !same(root, table), "root is existing root, table is fresh");
    up::sqrat_release(vm, table);
    up::sqrat_release(vm, root);
    up::sqplus_release(vm, empty());
    up::sqplus_retain(vm, empty());
    require(sq_gettop(vm) == top, "all object operations preserve stack");
    nested_context = nullptr;
}
void classes(HSQUIRRELVM vm) {
    auto base = empty(), child = empty();
    int base_tag = 0, child_tag = 0;
    const auto top = sq_gettop(vm);
    require(up::sqplus_create_class(vm, base, &base_tag, "Base", nullptr), "create base class");
    require(up::sqplus_create_class(vm, child, &child_tag, "Child", "Base"), "inherit upstream class");
    sq_pushobject(vm, child);
    SQUserPointer tag = nullptr;
    require(SQ_SUCCEEDED(sq_gettypetag(vm, -1, &tag)) && tag == &child_tag, "child typetag identity");
    require(SQ_SUCCEEDED(sq_getbase(vm, -1)), "child base lookup");
    HSQOBJECT actual_base; sq_getstackobj(vm, -1, &actual_base);
    require(same(base, actual_base), "inheritance identity preserved");
    sq_pop(vm, 2);
    const auto before = child;
    require(!up::sqplus_create_class(vm, child, &child_tag, "Bad", "MissingBase"), "missing base fails");
    require(same(before, child) && sq_gettop(vm) == top, "failed creation keeps output and stack");
    // Existing non-class parent follows the source VM's failure path too.
    sq_pushroottable(vm); sq_pushstring(vm, "NotClass", -1); sq_pushinteger(vm, 1);
    sq_newslot(vm, -3, SQFalse); sq_pop(vm, 1);
    require(!up::sqplus_create_class(vm, child, &child_tag, "Bad", "NotClass"), "nonclass base fails");
    require(same(before, child) && sq_gettop(vm) == top, "invalid base keeps previous owner");
    up::sqplus_release(vm, child); up::sqplus_release(vm, base);
    require(SquirrelVM::GetVMPtr() == nullptr, "class operations restore borrowed VM");
}
// The _get result has no table owner: only the VM return stack holds it.
// Sqrat must acquire its external reference before popping that stack slot.
thread_local int slot_releases = 0;
thread_local int slot_calls = 0;
SQInteger slot_cleanup(SQUserPointer, SQInteger) { ++slot_releases; return 0; }
SQInteger temporary_slot(HSQUIRRELVM vm) {
    ++slot_calls;
    const SQChar* name = nullptr;
    if (SQ_FAILED(sq_getstring(vm, 2, &name))) return SQ_ERROR;
    if (std::strcmp(name, "temporary") == 0) {
        sq_newuserdata(vm, 16);
        sq_setreleasehook(vm, -1, slot_cleanup);
        return 1;
    }
    if (std::strcmp(name, "null") == 0) { sq_pushnull(vm); return 1; }
    return sq_throwerror(vm, "missing property");
}
void slot_lifetime(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    const auto first_release = slot_releases;
    const auto first_call = slot_calls;
    {
        sq_newtable(vm);
        HSQOBJECT receiver; sq_getstackobj(vm, -1, &receiver);
        Sqrat::Object object(receiver, vm);
        sq_newtable(vm);
        sq_pushstring(vm, "_get", -1); sq_newclosure(vm, temporary_slot, 0);
        require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "register temporary _get");
        require(SQ_SUCCEEDED(sq_setdelegate(vm, -2)), "delegate _get");
        sq_pop(vm, 1);
        {
            auto value = object.GetSlot("temporary");
            require(value.GetObject()._type == OT_USERDATA, "upstream GetSlot result");
            require(slot_calls == first_call + 1, "GetSlot invokes _get exactly once");
            require(slot_releases == first_release, "GetSlot owns result before stack pop");
            require(sq_gettop(vm) == top, "GetSlot restores stack while result is owned");
        }
        require(slot_releases == first_release + 1, "GetSlot result releases exactly once");
        require(object.GetSlot("missing").IsNull(), "missing GetSlot returns null");
        require(sq_gettop(vm) == top, "missing GetSlot balances stack");
        sq_reseterror(vm);
        HSQOBJECT result;
        int calls = slot_calls;
        require(up::sqrat_get(vm, receiver, "temporary", result), "host uses upstream GetSlot");
        require(slot_calls == calls + 1 && slot_releases == first_release + 1,
                "transferred GetSlot result is alive, single lookup");
        up::sqrat_release(vm, result);
        require(slot_releases == first_release + 2, "transferred GetSlot releases once");
        require(up::sqrat_get(vm, receiver, "null", result) && result._type == OT_NULL,
                "existing null is not a missing slot");
        calls = slot_calls;
        require(!up::sqrat_get(vm, receiver, "missing", result) && result._type == OT_NULL,
                "missing lookup still resets output");
        require(slot_calls == calls + 1, "status does not probe _get twice");
        calls = slot_calls;
        result = up::sqplus_get_value(vm, receiver, "temporary");
        require(result._type == OT_USERDATA && slot_calls == calls + 1 &&
                slot_releases == first_release + 2, "SqPlus GetValue transfers owned result");
        up::sqplus_release(vm, result);
        require(up::sqplus_exists(vm, receiver, "null"), "Exists accepts null result");
        require(!up::sqplus_exists(vm, receiver, "missing"), "Exists misses key");
        sq_reseterror(vm);
    }
    require(slot_releases == first_release + 3 && sq_gettop(vm) == top,
            "temporary result is neither leaked nor released twice");
}
HSQOBJECT owned_top(HSQUIRRELVM vm) {
    auto result = up::sqplus_capture(vm, empty(), -1);
    sq_pop(vm, 1);
    return result;
}
HSQOBJECT integer(SQInteger value) {
    auto result = empty(); result._type = OT_INTEGER; result._unVal.nInteger = value; return result;
}
void object_operations(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    auto table = up::sqrat_table(vm);
    sq_newarray(vm, 0); auto array = owned_top(vm);
    sq_pushstring(vm, "value", -1); auto text = owned_top(vm);
    int native = 7, tag = 9, other_tag = 10;
    auto pointer = empty(); pointer._type = OT_USERPOINTER; pointer._unVal.pUserPointer = &native;
    require(up::sqplus_raw_set(vm, table, integer(-3), integer(-13)), "upstream object-key SetValue");
    require(up::sqplus_raw_set(vm, table, integer(4), text), "upstream string value");
    require(up::sqplus_raw_set(vm, table, integer(7), pointer), "upstream pointer value");
    require(up::sqplus_raw_set(vm, table, "named", text), "upstream string-key SetValue");
    require(up::sqplus_get_integer(vm, table, -3) == -13, "negative integer key");
    require(std::strcmp(up::sqplus_get_string(vm, table, 4), "value") == 0, "borrowed stored string");
    require(up::sqplus_get_userpointer(vm, table, 7) == &native, "native pointer identity");
    require(up::sqplus_get_integer(vm, table, 4) == 0, "wrong numeric conversion");
    require(up::sqplus_get_string(vm, table, -3) == nullptr, "wrong string conversion");
    require(up::sqplus_get_userpointer(vm, table, -3) == nullptr, "wrong pointer conversion");
    require(up::sqplus_get_integer(vm, table, 999) == 0, "missing integer key");
    require(up::sqplus_length(vm, table) == 4 && up::sqplus_length(vm, text) == 5,
            "upstream length type restrictions");
    require(up::sqplus_length(vm, integer(5)) == 0, "non-container length is zero");
    up::sqplus_append(vm, array, integer(10)); up::sqplus_append(vm, array, integer(20));
    require(up::sqplus_reverse(vm, array) && up::sqplus_get_integer(vm, array, 0) == 20,
            "source array append and reverse");
    up::sqplus_append(vm, table, integer(3));
    require(up::sqplus_length(vm, table) == 4, "append nonarray no-op");
    require(!up::sqplus_reverse(vm, table), "reverse nonarray fails");
    sq_reseterror(vm);
    require(!up::sqplus_begin_iteration(vm, integer(0)) && sq_gettop(vm) == top,
            "non-container cannot begin iteration");
    require(up::sqplus_begin_iteration(vm, table) && sq_gettop(vm) == top + 2,
            "iteration leaves container and iterator");
    auto key = empty(), value = empty(); int count = 0;
    while (SQ_SUCCEEDED(sq_next(vm, -2))) {
        key = up::sqplus_capture(vm, key, -2);
        value = up::sqplus_capture(vm, value, -1);
        sq_pop(vm, 2); ++count;
    }
    sq_pop(vm, 2);
    require(count == 4, "iteration captures all entries through source AttachToStackObject");
    up::sqplus_release(vm, key); up::sqplus_release(vm, value);
    auto delegate = up::sqrat_table(vm);
    sq_pushobject(vm, table); sq_pushobject(vm, delegate);
    require(SQ_SUCCEEDED(sq_setdelegate(vm, -2)), "set fixture delegate"); sq_pop(vm, 1);
    auto fetched = up::sqplus_get_delegate(vm, table);
    require(same(fetched, delegate), "GetDelegate identity and owned reference");
    up::sqplus_release(vm, fetched);
    auto klass = empty();
    require(up::sqplus_create_class(vm, klass, &tag, "NativeInstance", nullptr), "instance class");
    sq_pushobject(vm, klass); require(SQ_SUCCEEDED(sq_createinstance(vm, -1)), "create instance");
    auto instance = owned_top(vm); sq_pop(vm, 1);
    require(up::sqplus_set_instance_up(vm, instance, &native) &&
            up::sqplus_get_instance_up(vm, instance, &tag) == &native, "source instance association");
    require(up::sqplus_get_instance_up(vm, instance, &other_tag) == nullptr,
            "wrong instance tag rejected");
    sq_getlasterror(vm); require(sq_gettype(vm, -1) == OT_NULL, "GetInstanceUP clears mismatch error");
    sq_pop(vm, 1);
    require(!up::sqplus_set_instance_up(vm, table, &native), "noninstance setter fails");
    require(up::sqplus_length(vm, klass) == 0, "class excluded from Len despite sq_getsize support");
    for (auto object : {instance, klass, delegate, text, array, table}) up::sqplus_release(vm, object);
    require(sq_gettop(vm) == top && SquirrelVM::GetVMPtr() == nullptr,
            "source operations restore stack and borrowed VM scope");
}
void cycle() {
    Machine root, independent;
    objects(root.vm, independent.vm);
    classes(root.vm);
    slot_lifetime(root.vm);
    object_operations(root.vm);
    // Child VM shares the original VM's ref table but has a separate stack.
    auto child = sq_newthread(root.vm, 32);
    require(child != nullptr, "child VM");
    objects(child, root.vm);
    slot_lifetime(child);
    object_operations(child);
    sq_pop(root.vm, 1);
}
}
int main() {
    try {
        for (int i = 0; i < 8; ++i) cycle();
        std::atomic<bool> ok{true};
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) threads.emplace_back([&] {
            try { for (int i = 0; i < 16; ++i) cycle(); }
            catch (...) { ok = false; }
        });
        for (auto& thread : threads) thread.join();
        require(ok, "independent explicit VMs must not share contextual state across threads");
        std::puts("upstream binding contracts passed: 72 root/independent/child VM cycles");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what()); return 1;
    }
}
