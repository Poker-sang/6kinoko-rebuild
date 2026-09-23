#pragma once
#include "kinoko/native_record_view.hpp"
#include <cstdint>
#include <limits>
#include <string>

namespace kinoko::legacy {
// Boundary schema only: native strings publish data at +0, own a std::string
// at +4 and expose length/capacity at +16/+20. Capacity >=16 selects the
// borrowed data pointer even when std::string uses its own short storage.
// Untouched empty records and read-only legacy fixtures may still be inline.
struct StringRecord final {
    unsigned char characters[16];
    std::uint32_t length;
    std::uint32_t capacity;
};
static_assert(sizeof(StringRecord) == 24);
static_assert(offsetof(StringRecord, length) == 16);
static_assert(offsetof(StringRecord, capacity) == 20);

// Non-owning: copying/destroying a view does not copy/free its string. Lifetime
// continues to belong to the ACT/Actor/native record that embeds this storage.
class StringView final {
public:
    static constexpr std::uint32_t inline_bytes = sizeof(StringRecord::characters);
    static constexpr std::uint32_t inline_capacity = inline_bytes - 1;
    static constexpr std::uint32_t invalid_size = (std::numeric_limits<std::uint32_t>::max)();
    static constexpr std::uint32_t maximum_size = invalid_size - 1;

    explicit StringView(void* storage) noexcept : record_(storage) {}
    void* storage() const noexcept { return record_.data(); }
    explicit operator bool() const noexcept { return storage() != nullptr; }
    std::uint32_t length() const noexcept { return record_.get(&StringRecord::length); }
    std::uint32_t capacity() const noexcept { return record_.get(&StringRecord::capacity); }
    bool is_heap() const noexcept { return capacity() >= inline_bytes; }
    char* data() const noexcept {
        if (!*this) return nullptr;
        auto* bytes = record_.bytes(&StringRecord::characters);
        if (!is_heap()) return reinterpret_cast<char*>(bytes);
        // memcpy is required even for the pointer: generated native fields
        // need not have pointer alignment or a constructed C++ pointer object.
        static_assert(sizeof(char*) == sizeof(std::uint32_t), "legacy string ABI is Win32");
        char* result;
        std::memcpy(&result, bytes, sizeof(result));
        return result;
    }
    void destroy() const noexcept;
    void assign(const char* source, std::uint32_t size) const;
    void assign(StringView source, std::uint32_t position, std::uint32_t size) const;
    void append(const char* source, std::uint32_t size) const;
    void append(StringView source, std::uint32_t position, std::uint32_t size) const;
    bool reserve(std::uint32_t capacity, bool shrink) const;
    // Borrowed buffer, including short storage; invalidated by mutation.
    // Its lifetime belongs to this record's native string owner.
    char* grow(std::uint32_t capacity, std::uint32_t old_length) const;
private:
    std::string* owner() const noexcept;
    std::string& ensure_owner() const;
    void publish(std::string* value) const noexcept;
    kinoko::native::RecordView<StringRecord> record_;
};
} // namespace kinoko::legacy
