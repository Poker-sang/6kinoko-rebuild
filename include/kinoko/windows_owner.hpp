#pragma once
#include <windows.h>
#include <utility>

namespace kinoko::windows {
// Events/threads in the original runtime use NULL as the empty value. Thread
// owners MUST be joined before reset(); reset only closes the kernel handle.
class HandleOwner final {
    HANDLE value_ = nullptr;
public:
    HandleOwner() noexcept = default;
    explicit HandleOwner(HANDLE value) noexcept : value_(value) {}
    ~HandleOwner() { reset(); }
    HandleOwner(const HandleOwner&) = delete;
    HandleOwner& operator=(const HandleOwner&) = delete;
    HandleOwner(HandleOwner&& other) noexcept : value_(other.detach()) {}
    HandleOwner& operator=(HandleOwner&& other) noexcept {
        if (this != &other) reset(other.detach());
        return *this;
    }
    HANDLE get() const noexcept { return value_; }
    explicit operator bool() const noexcept { return value_ != nullptr; }
    HANDLE detach() noexcept { return std::exchange(value_, nullptr); }
    void reset(HANDLE value = nullptr) noexcept {
        if (value_ == value) return;
        const auto old = std::exchange(value_, value);
        if (old) CloseHandle(old);
    }
};
class CriticalLock final {
    CRITICAL_SECTION* section_;
public:
    explicit CriticalLock(CRITICAL_SECTION* section) noexcept : section_(section) {
        if (section_) EnterCriticalSection(section_);
    }
    ~CriticalLock() { if (section_) LeaveCriticalSection(section_); }
    CriticalLock(const CriticalLock&) = delete;
    CriticalLock& operator=(const CriticalLock&) = delete;
};
}
