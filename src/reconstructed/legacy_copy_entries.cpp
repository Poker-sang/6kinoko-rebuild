#include "kinoko/legacy_copy_entries.h"
#include <cstdint>

extern "C" int32_t function_43d110_this(int32_t receiver, int32_t source);

// The full recovered copy implementation is still C. These replace only its
// register shims; they do not claim that implementation has been migrated.
extern "C" int32_t __fastcall function_43d110(int32_t receiver, void*, int32_t source) {
    return function_43d110_this(receiver, source);
}
extern "C" int32_t __fastcall function_43cf20(int32_t receiver, void*, int32_t source) {
    // Original adjustor thunk: add ecx,4; tail-call copy; callee pops 4 bytes.
    const auto adjusted = static_cast<int32_t>(static_cast<uint32_t>(receiver) + 4u);
    return function_43d110_this(adjusted, source);
}
extern "C" int32_t __fastcall function_43e860(int32_t receiver, void*, int32_t source) {
    return function_43cf20(receiver, nullptr, source);
}
