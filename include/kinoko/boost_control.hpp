#pragma once
#include <cstddef>

// The counted object is created by the upstream implementation, never overlaid
// on an old byte buffer. All count operations require a constructed Boost base. Production Win32 has the recovered 16-byte layout.
namespace boost { namespace detail { class sp_counted_base; } }
namespace kinoko::native::upstream {
using CountedControl = boost::detail::sp_counted_base;
CountedControl* create_owner_control(void* allocation) noexcept;
bool owns_control(const void* control) noexcept;
bool lock(CountedControl* control) noexcept;
void add_strong(CountedControl* control) noexcept;
void add_weak(CountedControl* control) noexcept;
void release_weak(CountedControl* control) noexcept;
void release_strong(CountedControl* control) noexcept;
long use_count(const CountedControl* control) noexcept;
void* allocation(const CountedControl* control) noexcept;
} // namespace kinoko::native::upstream
