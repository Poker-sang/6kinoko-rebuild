#include "kinoko/boost_control.hpp"
#if defined(_WIN32)
#include <boost/smart_ptr/detail/sp_counted_base_w32.hpp>
#else
// Portable semantic tests only. The production project requires MSVC Win32;
// the pthread backend has a different layout and is not used as its ABI.
#include <boost/smart_ptr/detail/sp_counted_base_pt.hpp>
#endif
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

namespace kinoko::native::upstream {
class OwnerControl final : public boost::detail::sp_counted_base {
public:
    explicit OwnerControl(void* allocation) noexcept : allocation_(allocation) {}
    void* allocation() const noexcept { return allocation_; }
    void dispose() override {
        // The original sp_counted_impl_p<Actor*> owns an Actor* slot, not the
        // pooled Actor addressed by that slot. Allocation pairing stays malloc.
        std::free(allocation_);
        allocation_ = nullptr;
    }
    void destroy() override {
        this->~OwnerControl();
        std::free(this);
    }
    void* get_deleter(const boost::detail::sp_typeinfo&) override { return nullptr; }
private:
    void* allocation_;
};
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(boost::detail::sp_counted_base) == 12, "recovered counter ABI");
static_assert(sizeof(OwnerControl) == 16, "recovered allocation slot follows the counts");
#endif
namespace {
std::uintptr_t table(const void* value) noexcept {
    std::uintptr_t result;
    std::memcpy(&result, value, sizeof(result));
    return result;
}
std::uintptr_t owner_table() noexcept {
    // Learn a compiler-emitted vtable from an actual object, never invent one.
    // Its destructor does not call dispose and it contains no allocation.
    static const std::uintptr_t value = [] {
        OwnerControl probe(nullptr);
        return table(&probe);
    }();
    return value;
}
}
CountedControl* create_owner_control(void* allocation) noexcept {
    void* storage = std::malloc(sizeof(OwnerControl));
    return storage ? new (storage) OwnerControl(allocation) : nullptr;
}
bool owns_control(const void* control) noexcept { return control && table(control) == owner_table(); }
bool lock(CountedControl* control) noexcept { return control->add_ref_lock(); }
void add_strong(CountedControl* control) noexcept { control->add_ref_copy(); }
void add_weak(CountedControl* control) noexcept { control->weak_add_ref(); }
void release_weak(CountedControl* control) noexcept { control->weak_release(); }
void release_strong(CountedControl* control) noexcept { control->release(); }
long use_count(const CountedControl* control) noexcept { return control->use_count(); }
void* allocation(const CountedControl* control) noexcept { return static_cast<const OwnerControl*>(control)->allocation(); }
} // namespace kinoko::native::upstream
