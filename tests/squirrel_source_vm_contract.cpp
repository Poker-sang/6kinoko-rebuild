#include "kinoko/squirrel_vm_lifecycle.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_legacy_api.h"
#include <squirrel.h>
#include <vector>
#include <limits>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
int32_t address(const void* value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
HSQUIRRELVM machine_at(int32_t value) noexcept {
    return reinterpret_cast<HSQUIRRELVM>(static_cast<uintptr_t>(static_cast<uint32_t>(value)));
}
thread_local int32_t current_receiver = 0x12345678;
SQVM* exchange_receiver(SQVM* vm) {
    const auto previous = current_receiver;
    current_receiver = (int32_t)(intptr_t)vm;
    return reinterpret_cast<SQVM*>(static_cast<uintptr_t>(previous));
}
class ReceiverRegistration final {
public:
    ReceiverRegistration() { kinoko_sq_set_context_exchange(exchange_receiver); }
    ~ReceiverRegistration() { kinoko_sq_set_context_exchange(nullptr); }
    ReceiverRegistration(const ReceiverRegistration&) = delete;
    ReceiverRegistration& operator=(const ReceiverRegistration&) = delete;
};
class Machine final {
public:
    Machine() : vm_(machine_at(((int32_t)(uintptr_t)kinoko_sq_open(64)))) {
        if (!vm_) throw std::runtime_error("sq_open failed");
    }
    ~Machine() { sq_close(vm_); }
    Machine(const Machine&) = delete;
    Machine& operator=(const Machine&) = delete;
    HSQUIRRELVM get() const noexcept { return vm_; }
private:
    HSQUIRRELVM vm_;
};
class StackScope final {
public:
    explicit StackScope(HSQUIRRELVM vm) : vm_(vm), top_(sq_gettop(vm)) {}
    ~StackScope() { sq_settop(vm_, top_); }
    StackScope(const StackScope&) = delete;
    StackScope& operator=(const StackScope&) = delete;
private:
    HSQUIRRELVM vm_;
    SQInteger top_;
};
void require(bool condition, const char* message) {
    // assert is disabled in Release; every contract must still execute.
    if (!condition) throw std::runtime_error(message);
}
void report_error(HSQUIRRELVM vm) {
    sq_getlasterror(vm);
    const SQChar* text = nullptr;
    if (SQ_SUCCEEDED(sq_getstring(vm, -1, &text)) && text)
        std::fprintf(stderr, "Squirrel: %s\n", text);
    sq_pop(vm, 1);
}
SQInteger evaluate(HSQUIRRELVM vm, const char* name, const char* source) {
    StackScope scope(vm);
    if (SQ_FAILED(sq_compilebuffer(vm, source, static_cast<SQInteger>(std::strlen(source)), name, SQFalse))) {
        report_error(vm);
        throw std::runtime_error(std::string("compile: ") + name);
    }
    sq_pushroottable(vm);
    const auto previous_receiver = current_receiver;
    const auto status = kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), 1, SQTrue, SQFalse);
    require(current_receiver == previous_receiver, "callback receiver leaked out of call");
    if (SQ_FAILED(status)) {
        report_error(vm);
        throw std::runtime_error(std::string("execute: ") + name);
    }
    SQInteger value = 0;
    require(SQ_SUCCEEDED(sq_getinteger(vm, -1, &value)), "expected integer result");
    return value;
}
SQInteger native_twice(HSQUIRRELVM vm) {
    if (current_receiver != address(vm))
        return sq_throwerror(vm, _SC("wrong native callback VM receiver"));
    SQInteger value = 0;
    if (SQ_FAILED(sq_getinteger(vm, 2, &value)))
        return sq_throwerror(vm, _SC("integer argument required"));
    sq_pushinteger(vm, value * 2);
    return 1;
}
SQInteger native_failure(HSQUIRRELVM vm) {
    if (current_receiver != address(vm))
        return sq_throwerror(vm, _SC("wrong failing callback VM receiver"));
    return sq_throwerror(vm, _SC("native-contract-error"));
}
void register_native(HSQUIRRELVM vm, const char* name, SQFUNCTION function) {
    StackScope scope(vm);
    sq_pushroottable(vm);
    sq_pushstring(vm, name, -1);
    sq_newclosure(vm, function, 0);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "register native function");
}
void child_lifecycle(HSQUIRRELVM parent) {
    const auto original_top = sq_gettop(parent);
    for (int i = 0; i < 16; ++i) {
        const auto previous_receiver = current_receiver;
        const auto child_address = ((int32_t)(uintptr_t)kinoko_sq_create_thread((SQVM*)(uintptr_t)(address(parent)), 64));
        require(child_address != 0, "source child creation");
        auto* child = machine_at(child_address);
        require(current_receiver == previous_receiver, "child creation changed receiver");
        require(sq_gettop(parent) == original_top + 1, "parent must own exactly one child reference");
        require(sq_getvmstate(child) == SQ_VMSTATE_IDLE, "new child must be idle");
        int32_t actual_vtable = 0;
        std::memcpy(&actual_vtable, child, sizeof(actual_vtable));
        require(actual_vtable != 0 && actual_vtable == kinoko_sq_source_vm_vtable(), "collector must recognize source child vtable");
        require(evaluate(child, "child-shared-root", "return native_twice(21);") == 42, "child must inherit parent root/native bindings");
        sq_pop(parent, 1);
        sq_collectgarbage(parent);
        require(sq_gettop(parent) == original_top, "child finalization leaked parent stack");
    }
}

struct Bytecode {
    std::vector<unsigned char> bytes;
    size_t cursor = 0;
};
SQInteger write_bytecode(SQUserPointer context, SQUserPointer data, SQInteger size) {
    if (size < 0) return -1;
    auto& output = *static_cast<Bytecode*>(context);
    const auto* begin = static_cast<const unsigned char*>(data);
    try { output.bytes.insert(output.bytes.end(), begin, begin + size); }
    catch (...) { return -1; }
    return size;
}
SQInteger read_bytecode(SQUserPointer context, SQUserPointer data, SQInteger size) {
    auto& input = *static_cast<Bytecode*>(context);
    if (size < 0 || static_cast<size_t>(size) > input.bytes.size() - input.cursor) return -1;
    std::memcpy(data, input.bytes.data() + input.cursor, size);
    input.cursor += size;
    return size;
}
void legacy_api_contracts(HSQUIRRELVM vm) {
    StackScope stack(vm);
    const auto machine = address(vm);
    const auto initial_top = sq_gettop(vm);

    int native_object = 42;
    ((int32_t)(uintptr_t)kinoko_sq_push_user_pointer(((SQVM*)(uintptr_t)(uint32_t)((machine))), ((void*)(uintptr_t)(uint32_t)((address(&native_object))))));
    require(kinoko_sq_get_type(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1)) == OT_USERPOINTER, "userpointer must not be encoded as an integer");
    int32_t restored = 0;
    require(SQ_SUCCEEDED(kinoko_sq_get_user_pointer(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1), ((void**)((&restored))))) && restored == address(&native_object), "userpointer round trip");
    require(SQ_FAILED(kinoko_sq_get_integer(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1), (&restored))), "userpointer is not an integer");
    ((int32_t)(uintptr_t)kinoko_sq_pop((SQVM*)(uintptr_t)((machine)), (1)));
    ((int32_t)(uintptr_t)kinoko_sq_reset_error_and_return_vm(((SQVM*)(uintptr_t)(uint32_t)((machine)))));

    ((int32_t)(uintptr_t)kinoko_sq_push_string(((SQVM*)(uintptr_t)(uint32_t)((machine))), ((const char*)(uintptr_t)(uint32_t)((address("externally-retained")))), (-1)));
    HSQOBJECT retained;
    require(SQ_SUCCEEDED(kinoko_sq_get_stack_object(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1), ((HSQOBJECT*)((reinterpret_cast<int32_t*>(&retained)))))), "get retained object");
    ((int32_t)(uintptr_t)kinoko_sq_add_object_reference(((SQVM*)(uintptr_t)(uint32_t)((machine))), ((HSQOBJECT*)(uintptr_t)(uint32_t)((address(&retained))))));
    ((int32_t)(uintptr_t)kinoko_sq_pop((SQVM*)(uintptr_t)((machine)), (1)));
    kinoko_sq_collect_garbage(((SQVM*)(uintptr_t)(uint32_t)((machine))), (0), (0));
    ((int32_t)(uintptr_t)kinoko_sq_push_raw_object(((SQVM*)(uintptr_t)(uint32_t)((machine))), (retained._type), (address(retained._unVal.pRefCounted))));
    const SQChar* string = nullptr;
    require(SQ_SUCCEEDED(sq_getstring(vm, -1, &string)) && std::strcmp(string, "externally-retained") == 0, "external RefTable retains popped object");
    kinoko_sq_release_object_reference(((SQVM*)(uintptr_t)(uint32_t)((machine))), ((HSQOBJECT*)(uintptr_t)(uint32_t)((address(&retained)))));
    ((int32_t)(uintptr_t)kinoko_sq_reset_object(((HSQOBJECT*)(uintptr_t)(uint32_t)((address(&retained))))));
    ((int32_t)(uintptr_t)kinoko_sq_pop((SQVM*)(uintptr_t)((machine)), (1)));

    auto* child = machine_at(((int32_t)(uintptr_t)kinoko_sq_create_thread((SQVM*)(uintptr_t)((machine)), (32))));
    require(child != nullptr, "legacy child creation");
    // Distinct source/destination stack contents expose using the wrong VM
    // when translating a negative source index in sq_move.
    sq_pushinteger(child, 73);
    sq_pushinteger(vm, 19);
    ((int32_t)(uintptr_t)kinoko_sq_move_object(((SQVM*)(uintptr_t)(uint32_t)((machine))), ((SQVM*)(uintptr_t)(uint32_t)((address(child)))), (-1)));
    SQInteger moved = 0;
    require(SQ_SUCCEEDED(sq_getinteger(vm, -1, &moved)) && moved == 73, "sq_move negative index uses source VM");
    require(sq_gettop(child) == 1, "sq_move does not pop the source");
    ((int32_t)(uintptr_t)kinoko_sq_pop((SQVM*)(uintptr_t)((machine)), (3)));

    Bytecode code;
    const char* source = "return 6 * 7;";
    require(SQ_SUCCEEDED(kinoko_sq_compile_text(((SQVM*)(uintptr_t)(uint32_t)((machine))), (const char*)(uintptr_t)((address(source))), (static_cast<int32_t>(std::strlen(source))), (const char*)(uintptr_t)((reinterpret_cast<int32_t*>(const_cast<char*>("legacy-bytecode")))), (0))), "legacy compilebuffer");
    require(SQ_SUCCEEDED(kinoko_sq_write_closure(((SQVM*)(uintptr_t)(uint32_t)((machine))), (SQWRITEFUNC)(((void*)(uintptr_t)(uint32_t)((address(reinterpret_cast<const void*>(write_bytecode)))))), ((void*)(uintptr_t)(uint32_t)((address(&code)))))), "source closure serialization");
    ((int32_t)(uintptr_t)kinoko_sq_pop((SQVM*)(uintptr_t)((machine)), (1)));
    require(SQ_SUCCEEDED(kinoko_sq_read_closure(((SQVM*)(uintptr_t)(uint32_t)((machine))), (SQREADFUNC)(((void*)(uintptr_t)(uint32_t)((address(reinterpret_cast<const void*>(read_bytecode)))))), (reinterpret_cast<int32_t*>(&code)))), "source closure deserialization");
    ((int32_t)(uintptr_t)kinoko_sq_push_root_table(((SQVM*)(uintptr_t)(uint32_t)((machine)))));
    require(SQ_SUCCEEDED(kinoko_sq_call((SQVM*)(uintptr_t)((machine)), (1), (1), (0))), "deserialized closure call");
    require(SQ_SUCCEEDED(sq_getinteger(vm, -1, &moved)) && moved == 42, "deserialized result");
    sq_settop(vm, initial_top);

    int type_tag = 0;
    require(SQ_SUCCEEDED(kinoko_sq_new_class(((SQVM*)(uintptr_t)(uint32_t)((machine))), (0))), "base class creation");
    require(SQ_SUCCEEDED(kinoko_sq_set_type_tag(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1), ((void*)(uintptr_t)(uint32_t)((address(&type_tag)))))), "base class tag");
    require(SQ_SUCCEEDED(kinoko_sq_new_class(((SQVM*)(uintptr_t)(uint32_t)((machine))), (1))), "derived class creation");
    require(SQ_SUCCEEDED(kinoko_sq_create_instance(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1))), "instance creation without constructor");
    require(SQ_SUCCEEDED(kinoko_sq_set_instance_pointer(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1), ((void*)(uintptr_t)(uint32_t)((address(&native_object)))))), "instance native object");
    require(SQ_SUCCEEDED(kinoko_sq_get_instance_pointer(((SQVM*)(uintptr_t)(uint32_t)((machine))), (-1), ((void**)((&restored))), ((void*)(uintptr_t)(uint32_t)((address(&type_tag)))))) && restored == address(&native_object), "type-tag lookup walks actual class base links");
    sq_settop(vm, initial_top);

    const std::string large(8192, 'x');
    require(SQ_SUCCEEDED(kinoko_sq_raise_formatted_error((SQVM*)(uintptr_t)(machine), "%s:%d", large.c_str(), 17)), "formatted error");
    ((int32_t)(uintptr_t)kinoko_sq_get_last_error(((SQVM*)(uintptr_t)(uint32_t)((machine)))));
    require(SQ_SUCCEEDED(sq_getstring(vm, -1, &string)) && std::string(string) == large + ":17", "formatted error must not truncate or overrun a fixed buffer");
    ((int32_t)(uintptr_t)kinoko_sq_pop((SQVM*)(uintptr_t)((machine)), (1)));
    ((int32_t)(uintptr_t)kinoko_sq_reset_error_and_return_vm(((SQVM*)(uintptr_t)(uint32_t)((machine)))));
    require(sq_gettop(vm) == initial_top, "legacy API stack balance");
}
void standard_library_contracts(HSQUIRRELVM vm) {
    const auto initial_top = sq_gettop(vm);
    sq_pushroottable(vm);
    require(SQ_SUCCEEDED(kinoko_sq_register_io_library(((SQVM*)(uintptr_t)(uint32_t)((address(vm)))))), "source io registration");
    require(SQ_SUCCEEDED(kinoko_sq_register_blob_library(((SQVM*)(uintptr_t)(uint32_t)((address(vm)))))), "source blob registration");
    require(SQ_SUCCEEDED(kinoko_sq_register_math_library(((SQVM*)(uintptr_t)(uint32_t)((address(vm)))))), "source math registration");
    require(SQ_SUCCEEDED(kinoko_sq_register_string_library(((SQVM*)(uintptr_t)(uint32_t)((address(vm)))))), "source string/regexp registration");
    sq_pop(vm, 1);
    require(evaluate(vm, "source-standard-library", R"SQ(
        local storage = blob(0);
        storage.writen(12345, 'i');
        storage.seek(0);
        local value = storage.readn('i');
        local expression = regexp("[a-z]+[0-9]+");
        if (!expression.match("kinoko222")) return -1;
        if (format("%s-%d", "source", 222) != "source-222") return -2;
        if (strip("  source \t") != "source") return -3;
        if (split("a,b,c", ",").len() != 3) return -4;
        return value + abs(-5);
    )SQ") == 12350, "source standard libraries preserve script results");
    require(sq_gettop(vm) == initial_top, "stdlib registration stack balance");
}

void contracts() {
    Machine machine;
    const auto vm = machine.get();
    const auto initial_top = sq_gettop(vm);
    register_native(vm, "native_twice", native_twice);
    register_native(vm, "native_failure", native_failure);
    child_lifecycle(vm);
    legacy_api_contracts(vm);
    standard_library_contracts(vm);

    require(evaluate(vm, "arithmetic", R"SQ(
        local total = 0;
        for (local i = 0; i < 10; ++i) total += i;
        return total;
    )SQ") == 45, "arithmetic and branch execution");

    // Squirrel 2.2 explicitly captures outer values with :(name). Captured
    // slots are immutable; a captured table retains shared mutable state.
    require(evaluate(vm, "captured-closure", R"SQ(
        local factory = function(seed) {
            local state = { value = seed };
            return function(delta) : (state) {
                state.value += delta;
                return state.value;
            };
        };
        local next = factory(10);
        next(2);
        return next(3);
    )SQ") == 15, "captured table ownership");

    require(evaluate(vm, "class-and-native", R"SQ(
        class Counter {
            value = 0;
            constructor(initial) { value = initial; }
            function advance(delta) { value += delta; return value; }
        }
        local counter = Counter(5);
        return native_twice(counter.advance(4));
    )SQ") == 18, "class constructor and native callback");

    require(evaluate(vm, "table-array-clone", R"SQ(
        local original = { value = 7 };
        local copy = clone original;
        copy.value = 11;
        local values = [original.value, copy.value];
        values.append(13);
        return values[0] + values[1] + values[2];
    )SQ") == 31, "table/array/clone ownership");

    require(evaluate(vm, "generator", R"SQ(
        local sequence = function() { yield 4; yield 9; return 16; };
        local generator = sequence();
        local first = resume generator;
        local second = resume generator;
        local third = resume generator;
        return first + second + third;
    )SQ") == 29, "generator yield/resume");

    require(evaluate(vm, "coroutine", R"SQ(
        local worker = newthread(function() {
            local value = suspend(7);
            return native_twice(value) + 3;
        });
        local first = worker.call();
        local second = worker.wakeup(11);
        return first + second;
    )SQ") == 32, "thread suspend/wakeup, native receiver and return value");

    require(evaluate(vm, "exceptions", R"SQ(
        local caught = 0;
        try { native_failure(); }
        catch (error) { if (error == "native-contract-error") caught = 1; }
        try { throw "script-contract-error"; }
        catch (error) { if (error == "script-contract-error") caught += 2; }
        return caught;
    )SQ") == 3, "native and script exception propagation");

    {
        StackScope scope(vm);
        const char* source = "native_failure();";
        require(SQ_SUCCEEDED(sq_compilebuffer(vm, source, static_cast<SQInteger>(std::strlen(source)), "failure", SQFalse)), "compile failure contract");
        sq_pushroottable(vm);
        const auto previous_receiver = current_receiver;
        require(SQ_FAILED(kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), 1, SQTrue, SQFalse)), "uncaught exception must fail");
        require(current_receiver == previous_receiver, "receiver leaked after exception");
    }
    require(evaluate(vm, "after-exception", "return 42;") == 42, "VM recovery after exception");

    for (int i = 0; i < 64; ++i) {
        require(evaluate(vm, "garbage-cycle", R"SQ(
            local root = {};
            local child = { owner = root };
            root.child <- child;
            return 1;
        )SQ") == 1, "cycle creation");
        sq_collectgarbage(vm);
        require(sq_gettop(vm) == initial_top, "stack leak across calls or GC");
    }
}
} // namespace

int main() {
    try {
        ReceiverRegistration registration;
        for (int i = 0; i < 8; ++i) contracts();
        std::puts("Squirrel source VM contracts passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Squirrel contract failed: %s\n", error.what());
        return 1;
    }
}
