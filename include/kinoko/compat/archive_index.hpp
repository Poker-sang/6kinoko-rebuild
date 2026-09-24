#pragma once
#include "kinoko/compat/resource_rules.hpp"
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>

namespace kinoko::compat {
inline constexpr std::uint32_t dat_header_bytes = 6;
inline constexpr std::uint32_t dat_index_entry_header_bytes = 9;
struct DatIndexEntry {
    std::uint32_t offset, size;
    std::string_view path; // borrows the caller-owned decoded index bytes
};
// Parsing only: caller controls mount order and incremental publication. On
// malformed input the cursor is not reusable. No path normalization, bounds
// check against archive file length, payload decoding or mount rollback here.
class DatIndexCursor final {
    const std::uint8_t* bytes_;
    std::uint32_t size_, position_ = 0;
    bool failed_ = false;
public:
    DatIndexCursor(const std::uint8_t* bytes, std::uint32_t size) noexcept
        : bytes_(bytes), size_(size) {}
    bool next(DatIndexEntry& entry) noexcept {
        if (failed_) return false;
        if (!bytes_ || size_ - position_ < dat_index_entry_header_bytes) {
            failed_ = true;
            return false;
        }
        const auto* header = bytes_ + position_;
        const auto path_size = std::uint32_t{header[8]};
        position_ += dat_index_entry_header_bytes;
        if (size_ - position_ < path_size) {
            failed_ = true;
            return false;
        }
        entry = {read_le32(header), read_le32(header + 4),
                 std::string_view(reinterpret_cast<const char*>(bytes_ + position_), path_size)};
        position_ += path_size;
        return true;
    }
    std::uint32_t position() const noexcept { return position_; }
};

// The caller supplies the engine. The production wrapper deliberately passes
// its SHARED MT19937: replacing it with a private engine changes later random
// draws even if the resulting decoded index bytes are identical.
inline void decode_dat_index(std::uint8_t* bytes, std::uint32_t size,
                             std::mt19937& random) {
    random.seed(size + dat_header_bytes); // original uint32_t wrap
    for (std::uint32_t i = 0; i < size; ++i) bytes[i] ^= static_cast<std::uint8_t>(random());
    std::uint8_t key = 0xc5u, step = 0x89u;
    for (std::uint32_t i = 0; i < size; ++i) {
        bytes[i] ^= key;
        key = static_cast<std::uint8_t>(key + step);
        step = static_cast<std::uint8_t>(step + 0x49u);
    }
}
} // namespace kinoko::compat
