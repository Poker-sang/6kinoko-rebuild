#include "kinoko/squirrel_native_types.h"
#pragma once
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_host_object.hpp"
#include "sqpcheader.h"
#include "sqvm.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace bridge_test {
using kinoko::script::address;
using kinoko::script::pointer;
inline void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message); // Also executes in Release.
}
inline thread_local int32_t receiver = 0x12345678;
inline SQVM* exchange_receiver(SQVM* vm) {
    const auto old = receiver; receiver = (int32_t)(intptr_t)vm; return reinterpret_cast<SQVM*>(static_cast<uintptr_t>(old));
}
class Machine final {
public:
    Machine() : vm_(pointer<SQVM>(((int32_t)(uintptr_t)kinoko_sq_open(64)))) {
        require(vm_ != nullptr, "open source VM");
        kinoko_sq_set_context_exchange(exchange_receiver);
    }
    ~Machine() { kinoko_sq_set_context_exchange(nullptr); sq_close(vm_); }
    HSQUIRRELVM get() const { return vm_; }
    Machine(const Machine&) = delete;
    Machine& operator=(const Machine&) = delete;
private:
    HSQUIRRELVM vm_;
};
class Pair final {
public:
    explicit Pair(HSQUIRRELVM vm) : vm(vm) { HSQOBJECT o; sq_resetobject(&o); write(o); }
    ~Pair() { auto o = get(); sq_release(vm, &o); }
    Pair(const Pair&) = delete;
    Pair& operator=(const Pair&) = delete;
    void capture() {
        HSQOBJECT o; sq_getstackobj(vm, -1, &o); sq_addref(vm, &o);
        auto old = get(); sq_release(vm, &old); write(o); sq_pop(vm, 1);
    }
    HSQOBJECT get() const { HSQOBJECT o; std::memcpy(&o, words.data(), sizeof(o)); return o; }
    void write(const HSQOBJECT& o) { std::memcpy(words.data(), &o, sizeof(o)); }
    void push() const { sq_pushobject(vm, get()); }
    int32_t* data() { return words.data(); }
    int32_t id() { return address(words.data()); }
    HSQUIRRELVM vm;
    std::array<int32_t, 2> words;
};
class Top final {
public:
    explicit Top(HSQUIRRELVM vm) : vm_(vm), saved_(sq_gettop(vm)) {}
    ~Top() { sq_settop(vm_, saved_); }
    SQInteger saved() const { return saved_; }
private:
    HSQUIRRELVM vm_; SQInteger saved_;
};
inline void top(HSQUIRRELVM vm, SQInteger expected, const char* label) {
    require(sq_gettop(vm) == expected, label);
}
inline void last_error(HSQUIRRELVM vm) {
    sq_getlasterror(vm); const char* text = nullptr;
    if (SQ_SUCCEEDED(sq_getstring(vm, -1, &text)) && text) std::fprintf(stderr, "VM: %s\n", text);
    sq_pop(vm, 1);
}
inline void evaluate(HSQUIRRELVM vm, const char* source, Pair* result = nullptr) {
    Top restore(vm);
    require(SQ_SUCCEEDED(sq_compilebuffer(vm, source, static_cast<SQInteger>(std::strlen(source)),
        "bridge-contract", SQFalse)), "compile source contract");
    sq_pushroottable(vm);
    const auto saved_receiver = receiver;
    const auto status = kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), 1, SQTrue, SQFalse);
    if (SQ_FAILED(status)) last_error(vm);
    require(SQ_SUCCEEDED(status), "execute source contract");
    require(receiver == saved_receiver, "nested source call restores receiver");
    if (result) result->capture();
}
inline std::array<int32_t, 5> wrapper(HSQUIRRELVM vm, const Pair& value) {
    return {0x11111111, address(vm), value.words[0], value.words[1], 0};
}
inline SQInteger get_integer(HSQUIRRELVM vm, SQInteger index = -1) {
    SQInteger value; require(SQ_SUCCEEDED(sq_getinteger(vm, index, &value)), "integer result"); return value;
}
inline SQFloat get_float(HSQUIRRELVM vm, SQInteger index = -1) {
    SQFloat value; require(SQ_SUCCEEDED(sq_getfloat(vm, index, &value)), "float result"); return value;
}
inline std::string get_string(HSQUIRRELVM vm, SQInteger index = -1) {
    const char* value; require(SQ_SUCCEEDED(sq_getstring(vm, index, &value)), "string result"); return value;
}
template<class T> inline void store(void* storage, const T& value) { std::memcpy(storage, &value, sizeof(value)); }
template<class T> inline T load(const void* storage) { T value; std::memcpy(&value, storage, sizeof(value)); return value; }
}
