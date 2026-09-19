#pragma once
#include "kinoko/squirrel_host_object.hpp"

namespace kinoko::script::pair {
// Bare legacy type/data pairs are byte storage, not live HSQOBJECT instances.
// Borrowing never changes ownership; retain/release use the VM's EXTERNAL ref
// table. Release does not reset the pair (some original callers do that later).
inline HSQOBJECT read(const int32_t* bytes) noexcept {
    HSQOBJECT value;
    std::memcpy(&value, bytes, sizeof value);
    return value;
}
inline void write(int32_t* bytes, const HSQOBJECT& value) noexcept {
    std::memcpy(bytes, &value, sizeof value);
}
inline void reset(int32_t* bytes) noexcept {
    HSQOBJECT value; sq_resetobject(&value); write(bytes, value);
}
inline void retain(HSQUIRRELVM vm, const int32_t* bytes) {
    auto value = read(bytes); sq_addref(vm, &value);
}
inline void release(HSQUIRRELVM vm, const int32_t* bytes) {
    auto value = read(bytes); sq_release(vm, &value);
}
inline SQRESULT capture(HSQUIRRELVM vm, SQInteger index, int32_t* bytes) {
    HSQOBJECT value;
    const auto result = sq_getstackobj(vm, index, &value);
    if (SQ_SUCCEEDED(result)) write(bytes, value);
    return result;
}
// ACT's untagged instance getter must preserve the output on type mismatch.
inline SQRESULT instance_address(HSQUIRRELVM vm, SQInteger index, int32_t* output) {
    SQUserPointer user = nullptr;
    const auto result = sq_getinstanceup(vm, index, &user, nullptr);
    if (SQ_SUCCEEDED(result)) *output = address(user);
    return result;
}
struct InstanceInfo { int32_t class_address = 0; int32_t user_address = 0; };
inline InstanceInfo inspect_instance(HSQUIRRELVM vm, const HSQOBJECT& value) {
    InstanceInfo result;
    if (sq_type(value) != OT_INSTANCE) return result;
    StackTop stack(vm);
    sq_pushobject(vm, value);
    SQUserPointer user = nullptr;
    if (SQ_SUCCEEDED(sq_getinstanceup(vm, -1, &user, nullptr)))
        result.user_address = address(user);
    if (SQ_SUCCEEDED(sq_getclass(vm, -1))) {
        HSQOBJECT type;
        sq_getstackobj(vm, -1, &type);
        result.class_address = data_bits(type);
    }
    return result;
}
}
