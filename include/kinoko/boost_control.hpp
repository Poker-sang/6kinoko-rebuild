#pragma once
#include <cstddef>

// The counted object is created by the upstream implementation, never overlaid
// on an old byte buffer. Production Win32 has the recovered 16-byte layout.
namespace kinoko::native::upstream {
class OwnerControl;
OwnerControl* create_owner_control(void* allocation) noexcept;
bool owns_control(const void* control) noexcept;
bool lock(OwnerControl* control) noexcept;
void add_weak(OwnerControl* control) noexcept;
void release_weak(OwnerControl* control) noexcept;
void release_strong(OwnerControl* control) noexcept;
long use_count(const OwnerControl* control) noexcept;
void* allocation(const OwnerControl* control) noexcept;
} // namespace kinoko::native::upstream
