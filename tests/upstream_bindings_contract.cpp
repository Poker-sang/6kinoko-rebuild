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
    }
    require(slot_releases == first_release + 1 && sq_gettop(vm) == top,
            "temporary result is neither leaked nor released twice");
}
void cycle() {
    Machine root, independent;
    objects(root.vm, independent.vm);
    classes(root.vm);
    slot_lifetime(root.vm);
    // Child VM shares the original VM's ref table but has a separate stack.
    auto child = sq_newthread(root.vm, 32);
    require(child != nullptr, "child VM");
    objects(child, root.vm);
    slot_lifetime(child);
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
