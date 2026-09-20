#include "kinoko/upstream_bindings.hpp"
#include "kinoko/sqplus_source_entries.hpp"
#include <sqplus.h>
#include <sqrat/sqratTable.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
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
SQInteger factory_callback(HSQUIRRELVM vm) { sq_pushinteger(vm, 23); return 1; }
void factories(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    auto table = up::sqplus_new_table(vm);
    auto text = up::sqplus_new_string(vm, "source factory");
    auto null_text = up::sqplus_new_string(vm, nullptr);
    auto closure = up::sqplus_new_closure(vm, factory_callback);
    require(table._type == OT_TABLE && text._type == OT_STRING && null_text._type == OT_NULL &&
            closure._type == OT_NATIVECLOSURE && sq_gettop(vm) == top, "source factory types and stack");
    sq_pushobject(vm, text);
    const SQChar* data = nullptr;
    require(SQ_SUCCEEDED(sq_getstring(vm, -1, &data)) && !std::strcmp(data, "source factory"), "CreateString data");
    sq_pop(vm, 1);
    sq_pushobject(vm, closure); sq_pushobject(vm, table);
    require(SQ_SUCCEEDED(sq_call(vm, 1, SQTrue, SQFalse)), "CreateFunction actual native call");
    SQInteger answer = 0;
    require(SQ_SUCCEEDED(sq_getinteger(vm, -1, &answer)) && answer == 23, "factory callback result");
    sq_pop(vm, 2);
    for (auto value : {closure, text, null_text, table}) up::sqplus_release(vm, value);
    require(sq_gettop(vm) == top && SquirrelVM::GetVMPtr() == nullptr, "factory ownership and scope");
}
void variable_names() {
    for (const char* name : {static_cast<const char*>(nullptr), "", "name", "_already_tagged"}) {
        const auto key = up::sqplus_variable_key(name);
        require(std::string(key.data()) == std::string("_v") + (name ? name : ""), "source metadata key");
        for (auto i = std::strlen(key.data()) + 1; i < key.size(); ++i)
            require(key[i] == 0, "unused key storage stays zero initialized");
    }
    for (size_t length : {254, 255, 256, 1024}) {
        const std::string name(length, 'n');
        const auto key = up::sqplus_variable_key(name.c_str());
        require(std::string(key.data()) == "_v" + name.substr(0, 255), "metadata name truncates at 255");
    }
    // Match the host's bounded prefix contract: no read of byte 255.
    const std::vector<char> prefix(255, 'b');
    const auto key = up::sqplus_variable_key(prefix.data());
    require(std::string(key.data()) == "_v" + std::string(255, 'b'), "bounded non-NUL prefix");
}
// Exercise source scalar switches through the same unaligned host adapter.
// Expected narrowing comes from original 4AACF5/4AAD07, not a copy of the adapter.
void scalar_variables(HSQUIRRELVM vm) {
    using kinoko::script::binding::Variable;
    namespace binding = kinoko::script::binding;
    const auto original_top = sq_gettop(vm);
    require(original_top == 0, "scalar fixture starts with empty stack");
    std::array<unsigned char, 8> bytes{};
    void* data = bytes.data() + 1;
    auto integer_at = [&](SQInteger index) {
        SQInteger value = 0;
        require(SQ_SUCCEEDED(sq_getinteger(vm, index, &value)), "scalar integer result");
        return value;
    };
    for (int category : {0, 1}) for (int size : {1, 2, 4}) {
        Variable info{0, category, 0, 0, static_cast<uint16_t>(size), 0};
        for (int32_t input : {-32769, -129, -1, 0, 127, 128, 32767, 65535, 65536}) {
            bytes.fill(0xA5); sq_settop(vm, 0);
            sq_pushnull(vm); sq_pushnull(vm); sq_pushinteger(vm, input);
            const int32_t expected = category == 0 && size == 1 ? static_cast<int8_t>(input) :
                category == 0 && size == 2 ? static_cast<int16_t>(input) : input;
            require(up::sqplus_write_scalar(vm, info, data) == 1 && sq_gettop(vm) == 4,
                    "source setter pushes one result");
            require(integer_at(-1) == expected, "source setter returns stored integer");
            const auto count = category == 1 ? 4 : size;
            require(bytes[0] == 0xA5 && bytes[count + 1] == 0xA5, "scalar write canaries");
            require(up::sqplus_read_scalar(vm, info, data, 0) == 1 &&
                    integer_at(-1) == expected, "source integer getter round trip");
            info.flags = binding::Constant;
            require(up::sqplus_read_scalar(vm, info, nullptr, input) == 1 &&
                    integer_at(-1) == input, "constant integers do not use narrow storage");
            info.flags = 0;
        }
    }
    Variable info{0, 0, 0, 0, 4, 0};
    sq_settop(vm, 0); sq_pushnull(vm); sq_pushnull(vm); sq_pushstring(vm, "bad", -1);
    require(up::sqplus_write_scalar(vm, info, data) == 1 && integer_at(-1) == 0,
            "failed integer conversion follows source StackHandler zero policy");
    sq_settop(vm, 2); sq_pushfloat(vm, 12.75f);
    require(up::sqplus_write_scalar(vm, info, data) == 1 && integer_at(-1) == 12,
            "source integer conversion accepts float");
    info.category = 2;
    sq_settop(vm, 2); sq_pushinteger(vm, 42);
    require(up::sqplus_write_scalar(vm, info, data) == 1, "source float setter accepts integer");
    float stored_float = 0; std::memcpy(&stored_float, data, sizeof(stored_float));
    require(stored_float == 42, "source float storage");
    const auto saved = bytes;
    sq_settop(vm, 2); sq_pushstring(vm, "bad", -1);
    require(SQ_FAILED(up::sqplus_write_scalar(vm, info, data)) && bytes == saved && sq_gettop(vm) == 3,
            "host float rejection preserves destination and stack");
    sq_getlasterror(vm); require(sq_gettype(vm, -1) == OT_STRING, "float rejection retains API error");
    sq_settop(vm, 0); info.flags = binding::Constant;
    require(up::sqplus_read_scalar(vm, info, nullptr, -42) == 1, "host float constant conversion");
    SQFloat result_float = 0; sq_getfloat(vm, -1, &result_float);
    require(result_float == -42.0f, "host float constant is numeric, not pointer-bit overlay");
    info.category = 3; info.flags = 0; info.size = 1;
    for (bool input : {false, true}) {
        bytes.fill(0xA5); sq_settop(vm, 0);
        sq_pushnull(vm); sq_pushnull(vm); sq_pushbool(vm, input);
        require(up::sqplus_write_scalar(vm, info, data) == 1 && bytes[1] == input && bytes[2] == 0xA5,
                "source bool store is one normalized byte");
    }
    for (int byte : {0, 1, 128, 255}) {
        bytes[1] = static_cast<unsigned char>(byte);
        require(up::sqplus_read_scalar(vm, info, data, 0) == 1, "byte-backed bool getter");
        SQBool result = SQFalse; sq_getbool(vm, -1, &result);
        require((result != 0) == (byte != 0), "arbitrary host bool byte is normalized before source read");
        sq_pop(vm, 1);
    }
    info.flags = binding::Constant;
    require(up::sqplus_read_scalar(vm, info, nullptr, 256) == 1, "host bool constant complete-word test");
    SQBool result = SQFalse; sq_getbool(vm, -1, &result);
    require(result == SQTrue, "constant bool does not inspect only low byte");
    sq_settop(vm, 0);
    sq_pushnull(vm); sq_pushnull(vm); sq_pushinteger(vm, 1);
    auto unchanged = bytes;
    for (uint16_t flags : {binding::ReadOnly, binding::Constant}) {
        info.flags = flags;
        require(SQ_FAILED(up::sqplus_write_scalar(vm, info, data)) && bytes == unchanged && sq_gettop(vm) == 3,
                "host readonly/constant rejection is not replaced by snapshot exception policy");
    }
    // Verify source writeback ordering independently: callback sees the narrowed
    // temporary while the result has not yet been pushed to the actual VM.
    struct CommitProbe {
        HSQUIRRELVM vm; char* staged; bool observed = false;
        static void run(void* state) noexcept {
            auto& self = *static_cast<CommitProbe*>(state);
            self.observed = *self.staged == -128 && sq_gettop(self.vm) == 3;
        }
    };
    sq_settop(vm, 2); sq_pushinteger(vm, 128);
    StackHandler stack(vm); SqPlus::VarRef metadata;
    metadata.m_type = SqPlus::VAR_TYPE_INT; metadata.m_size = 1; metadata.varType = nullptr;
    char staged = 0; CommitProbe probe{vm, &staged};
    require(SqPlus::WriteScalarForHost(stack, metadata, &staged, &CommitProbe::run, &probe) == 1 &&
            probe.observed && integer_at(-1) == -128, "commit occurs between original assignment and Return");
    sq_settop(vm, original_top);
}
void cycle() {
    Machine root, independent;
    objects(root.vm, independent.vm);
    classes(root.vm);
    slot_lifetime(root.vm);
    object_operations(root.vm);
    factories(root.vm);
    scalar_variables(root.vm);
    // Child VM shares the original VM's ref table but has a separate stack.
    auto child = sq_newthread(root.vm, 32);
    require(child != nullptr, "child VM");
    objects(child, root.vm);
    slot_lifetime(child);
    object_operations(child);
    factories(child);
    scalar_variables(child);
    sq_pop(root.vm, 1);
}
}
int main() {
    try {
        variable_names();
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
