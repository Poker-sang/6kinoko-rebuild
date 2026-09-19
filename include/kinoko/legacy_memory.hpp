#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <type_traits>

namespace kinoko::legacy {
static_assert(sizeof(void*) == 4, "Legacy object addresses are Win32 values");

template<class T = void> T* pointer(std::int32_t value) noexcept {
    return reinterpret_cast<T*>(static_cast<std::uintptr_t>(static_cast<std::uint32_t>(value)));
}
template<class T> std::int32_t address(T* value) noexcept {
    return static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(value));
}
// These fields belong to native byte-layout records, not Squirrel internals.
// Do not use this accessor to reproduce the VM implementation: use sq_*.
template<class T> T& field(std::int32_t base, std::uint32_t offset = 0) noexcept {
    return *reinterpret_cast<T*>(static_cast<std::uintptr_t>(
        static_cast<std::uint32_t>(base) + offset));
}
template<class T> T load(const void* source) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    T value; std::memcpy(&value, source, sizeof value); return value;
}
template<class T> void store(void* destination, const T& value) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memcpy(destination, &value, sizeof value);
}
struct Free {
    void operator()(void* value) const noexcept { std::free(value); }
};
template<class T> using Allocation = std::unique_ptr<T, Free>;
}
