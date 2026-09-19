#pragma once
#include <utility>

namespace kinoko {
// Adopts exactly one existing COM reference. Copying would need an explicit
// AddRef; passing get() is a borrow and never changes the reference count.
template<class Interface>
class ComOwner final {
    Interface* value_ = nullptr;
public:
    ComOwner() noexcept = default;
    explicit ComOwner(Interface* owned) noexcept : value_(owned) {}
    ~ComOwner() { reset(); }
    ComOwner(const ComOwner&) = delete;
    ComOwner& operator=(const ComOwner&) = delete;
    ComOwner(ComOwner&& other) noexcept : value_(other.detach()) {}
    ComOwner& operator=(ComOwner&& other) noexcept {
        if (this != &other) {
            // Distinct owners can hold distinct references to the same COM
            // object. Moving must release this owner's reference even then.
            auto* previous = std::exchange(value_, other.detach());
            if (previous) previous->Release();
        }
        return *this;
    }
    Interface* get() const noexcept { return value_; }
    Interface* operator->() const noexcept { return value_; }
    explicit operator bool() const noexcept { return value_ != nullptr; }
    Interface* detach() noexcept { return std::exchange(value_, nullptr); }
    void reset(Interface* owned = nullptr) noexcept {
        if (value_ == owned) return;
        auto* previous = std::exchange(value_, owned);
        if (previous) previous->Release();
    }
    // Out-parameters receive a fresh owned reference, not an alias to an old one.
    Interface** put() noexcept { reset(); return &value_; }
};
}
