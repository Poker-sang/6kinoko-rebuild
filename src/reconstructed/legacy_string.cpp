// Recovered MSVC/Dinkumware string operations. The layout/growth evidence is
// in original 4038C0, 4039E0, 403BF0, 403CE0/403DBC and 4066F0; see the audit.
// Do not replace these byte records with the current toolchain's std::string.
#include "kinoko/legacy_string.h"
#include "kinoko/legacy_string.hpp"
#include <algorithm>
#include <cstdlib>
#include <memory>

namespace kinoko::legacy {
namespace {
struct FreeBuffer final {
    void operator()(char* buffer) const noexcept { std::free(buffer); }
};
using Buffer = std::unique_ptr<char, FreeBuffer>;
Buffer allocate(std::uint32_t capacity) {
    return Buffer(static_cast<char*>(std::malloc(static_cast<std::size_t>(capacity) + 1)));
}
}

void StringView::assign(const char* source, std::uint32_t size) const {
    if (!*this || size > maximum_size) return;
    const auto old_length = length();
    const auto old_capacity = capacity();
    const auto* old_data = data();
    Buffer temporary;
    // An assignment may refer to the old allocation, including its NUL byte.
    // Retain the snapshot and allocation-failure behavior before any growth.
    if (source && size) {
        const auto from = reinterpret_cast<std::uintptr_t>(source);
        const auto begin = reinterpret_cast<std::uintptr_t>(old_data);
        const auto end = begin + static_cast<std::uintptr_t>(old_length) + 1;
        if (from >= begin && from < end && from <= UINTPTR_MAX - size && from + size <= end) {
            temporary.reset(static_cast<char*>(std::malloc(size)));
            if (!temporary) return;
            std::memcpy(temporary.get(), source, size);
            source = temporary.get();
        }
    }
    if (old_capacity < size && !grow(size, old_length)) return;
    if (size && source) std::memmove(data(), source, size);
    terminate(size);
}

void StringView::assign(StringView source, std::uint32_t position, std::uint32_t size) const {
    if (!*this || !source || position > source.length()) return;
    const auto count = (std::min)(source.length() - position, size);
    if (storage() == source.storage()) {
        // 406CC0 erases the suffix then prefix in place, retaining capacity.
        if (count) std::memmove(data(), data() + position, count);
        terminate(count);
    } else {
        assign(source.data() + position, count);
    }
}

void StringView::append(const char* source, std::uint32_t size) const {
    if (!*this) return;
    const auto old_length = length();
    const auto from = reinterpret_cast<std::uintptr_t>(source);
    const auto begin = reinterpret_cast<std::uintptr_t>(data());
    // Numeric comparisons preserve Win32 address ordering without comparing
    // pointers into unrelated allocations as C++ array iterators.
    if (source && from >= begin && from < begin + old_length) {
        append(*this, static_cast<std::uint32_t>(from - begin), size);
        return;
    }
    if (size > invalid_size - old_length) return;
    const auto new_length = old_length + size;
    if (new_length == invalid_size) return;
    if (capacity() < new_length && !grow(new_length, old_length)) return;
    if (size && source) std::memmove(data() + old_length, source, size);
    terminate(new_length);
}

void StringView::append(StringView source, std::uint32_t position, std::uint32_t size) const {
    if (!*this || !source || position > source.length()) return;
    const auto count = (std::min)(source.length() - position, size);
    if (!count) return;
    const auto old_length = length();
    if (count > invalid_size - old_length) return;
    const auto new_length = old_length + count;
    if (capacity() < new_length && !grow(new_length, old_length)) return;
    // Resolve both buffers AFTER growth. The source may be this very string.
    std::memmove(data() + old_length, source.data() + position, count);
    terminate(new_length);
}

bool StringView::reserve(std::uint32_t requested, bool shrink) const {
    if (!*this || requested == invalid_size) return false;
    const auto old_capacity = capacity();
    const auto old_length = length();
    if (old_capacity < requested) return grow(requested, old_length) != 0;
    if (requested >= inline_bytes || !shrink) {
        if (!requested) terminate(0);
        return requested != 0;
    }
    const auto kept = (std::min)(old_length, requested);
    if (is_heap()) {
        Buffer old(data());
        if (kept) std::memmove(record_.bytes(&StringRecord::characters), old.get(), kept);
    }
    capacity(inline_capacity);
    terminate(kept);
    return requested != 0;
}

std::uintptr_t StringView::grow(std::uint32_t requested, std::uint32_t old_length) const {
    if (!*this) return 0;
    const auto old_capacity = capacity();
    auto adjusted = requested | inline_capacity;
    if (adjusted != invalid_size) {
        const auto half = old_capacity / 2;
        if (half > adjusted / 3)
            adjusted = old_capacity > maximum_size - half ? maximum_size : old_capacity + half;
    } else {
        adjusted = requested;
    }
    if (adjusted == invalid_size) return 0;
    auto next = allocate(adjusted);
    if (!next) return 0;
    if (old_length) std::memcpy(next.get(), data(), old_length);
    if (is_heap()) std::free(data());
    // The historical return is an address, not the retained buffer in every
    // branch. Capture its integer representation before a possible free.
    const auto result = reinterpret_cast<std::uintptr_t>(next.get());
    const auto* allocation = next.get();
    std::memcpy(record_.bytes(&StringRecord::characters), &allocation, sizeof(allocation));
    capacity(adjusted);
    length(old_length);
    if (adjusted < inline_bytes) {
        if (old_length) std::memcpy(record_.bytes(&StringRecord::characters), next.get(), old_length);
        data()[old_length] = 0;
    } else {
        next.get()[old_length] = 0;
        next.release(); // Ownership transfers to the containing legacy record.
    }
    return result;
}
} // namespace kinoko::legacy

namespace {
using kinoko::legacy::StringView;
void* pointer(std::int32_t value) noexcept {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(static_cast<std::uint32_t>(value)));
}
std::int32_t address(const void* value) noexcept {
    return static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(value));
}
}
extern "C" const char* retdec_std_string_data(int32_t object) {
    return StringView(pointer(object)).data();
}
extern "C" int32_t retdec_string_assign_n(int32_t* object, const char* source, uint32_t size) {
    StringView(object).assign(source, size);
    return address(object);
}
extern "C" int32_t retdec_string_assign_cstr(int32_t* object, const char* source) {
    return retdec_string_assign_n(object, source, retdec_safe_c_string_length(source));
}
extern "C" int32_t kinoko_string_assign_substring(int32_t object, int32_t source,
    uint32_t position, uint32_t size) {
    if (!object || !source) return 0;
    StringView(pointer(object)).assign(StringView(pointer(source)), position, size);
    return object;
}
// Address range: 0x4038c0 - 0x4039d3
extern "C" int32_t function_4038c0(int32_t object, const char* source, uint32_t size) {
    StringView(pointer(object)).append(source, size);
    return object;
}
// Address range: 0x4039e0 - 0x403a8b
extern "C" int32_t function_4039e0(int32_t object, uint32_t capacity, int32_t shrink) {
    return StringView(pointer(object)).reserve(capacity, shrink != 0);
}
// Address range: 0x403bf0 - 0x403cd3
extern "C" int32_t function_403bf0(int32_t object, int32_t source, uint32_t position, uint32_t size) {
    if (!object || !source) return 0;
    StringView(pointer(object)).append(StringView(pointer(source)), position, size);
    return object;
}
// Address range: 0x403ce0 - 0x403e18 (includes original cleanup 403DBC)
extern "C" int32_t function_403ce0(int32_t object, uint32_t capacity, uint32_t old_length) {
    return static_cast<int32_t>(StringView(pointer(object)).grow(capacity, old_length));
}
