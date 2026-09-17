#include <squirrel.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
class Machine final {
public:
    Machine() : vm_(sq_open(64)) {
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
    // Do not use assert: these contracts also run in Release/NDEBUG builds.
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
    if (SQ_FAILED(sq_call(vm, 1, SQTrue, SQFalse))) {
        report_error(vm);
        throw std::runtime_error(std::string("execute: ") + name);
    }
    SQInteger value = 0;
    require(SQ_SUCCEEDED(sq_getinteger(vm, -1, &value)), "expected integer result");
    return value;
}

SQInteger native_twice(HSQUIRRELVM vm) {
    SQInteger value = 0;
    if (SQ_FAILED(sq_getinteger(vm, 2, &value)))
        return sq_throwerror(vm, _SC("integer argument required"));
    sq_pushinteger(vm, value * 2);
    return 1;
}

SQInteger native_failure(HSQUIRRELVM vm) {
    return sq_throwerror(vm, _SC("native-contract-error"));
}

void register_native(HSQUIRRELVM vm, const char* name, SQFUNCTION function) {
    StackScope scope(vm);
    sq_pushroottable(vm);
    sq_pushstring(vm, name, -1);
    sq_newclosure(vm, function, 0);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "register native function");
}

void contracts() {
    Machine machine;
    const auto vm = machine.get();
    const auto initial_top = sq_gettop(vm);
    register_native(vm, "native_twice", native_twice);
    register_native(vm, "native_failure", native_failure);

    require(evaluate(vm, "arithmetic", R"SQ(
        local total = 0;
        for (local i = 0; i < 10; ++i) total += i;
        return total;
    )SQ") == 45, "arithmetic and branch execution");

    require(evaluate(vm, "captured-closure", R"SQ(
        local factory = function(seed) {
            return function(delta) { seed += delta; return seed; };
        };
        local next = factory(10);
        next(2);
        return next(3);
    )SQ") == 15, "captured variable ownership");

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
            return value + 3;
        });
        local first = worker.call();
        local second = worker.wakeup(11);
        return first + second;
    )SQ") == 21, "thread suspend/wakeup and return value");

    require(evaluate(vm, "exceptions", R"SQ(
        local caught = 0;
        try { native_failure(); }
        catch (error) { if (error == "native-contract-error") caught = 1; }
        try { throw "script-contract-error"; }
        catch (error) { if (error == "script-contract-error") caught += 2; }
        return caught;
    )SQ") == 3, "native and script exception propagation");

    // A failed call must not make subsequent compilation/execution unusable.
    {
        StackScope scope(vm);
        const char* source = "throw \"uncaught-contract-error\";";
        require(SQ_SUCCEEDED(sq_compilebuffer(vm, source, static_cast<SQInteger>(std::strlen(source)), "failure", SQFalse)), "compile failure contract");
        sq_pushroottable(vm);
        require(SQ_FAILED(sq_call(vm, 1, SQTrue, SQFalse)), "uncaught exception must fail");
    }
    require(evaluate(vm, "after-exception", "return 42;") == 42, "VM recovery after exception");

    // Exercise cyclic objects and finalization repeatedly, rather than only
    // checking that a single script produced a plausible value.
    for (int i = 0; i < 64; ++i) {
        require(evaluate(vm, "garbage-cycle", R"SQ(
            local root = {};
            local child = { parent = root };
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
        for (int i = 0; i < 8; ++i) contracts();
        std::puts("Squirrel source VM contracts passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Squirrel contract failed: %s\n", error.what());
        return 1;
    }
}
