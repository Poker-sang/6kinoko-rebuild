#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// Format/lookup rules extracted from the Win32 runtime, not a new filesystem.
// No HANDLE, VM object, host-layout overlay, mount policy or fallback lives here.
namespace kinoko::compat {

// DAT and CV4 fields are independent of the host's pointer width. Callers must
// establish the readable byte count before using these unaligned LE readers.
constexpr std::uint16_t read_le16(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint16_t>(std::uint16_t{bytes[0]} |
                                     (std::uint16_t{bytes[1]} << 8));
}
constexpr std::uint32_t read_le32(const std::uint8_t* bytes) noexcept {
    return std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8) |
           (std::uint32_t{bytes[2]} << 16) | (std::uint32_t{bytes[3]} << 24);
}

// 410A07 removes exactly one leading "./" BEFORE converting backslashes.
// Keep case, repeated prefixes, dot components and non-ASCII bytes unchanged.
// Insertion, CharLowerBuffA CRC (including NUL), _stricmp and the single-entry
// collision shortcut still belong to archive_store.cpp, not this helper.
inline std::string runtime_archive_lookup_path(std::string_view path) {
    if (path.size() >= 2 && path[0] == '.' && path[1] == '/')
        path.remove_prefix(2);
    std::string result(path);
    for (auto& character : result)
        if (character == '\\') character = '/';
    return result;
}

constexpr std::uint8_t archive_payload_key(std::uint32_t offset) noexcept {
    return static_cast<std::uint8_t>((offset >> 1) | 0x23u);
}

// Count is supplied by the caller, deliberately NOT inferred from bytes read.
// The old virtual reader decodes its clamped request; read_exact decodes only
// a successful exact read. Do not merge those two stream contracts here.
inline void decode_archive_payload(void* data, std::uint32_t count,
                                   std::uint8_t key) noexcept {
    auto* bytes = static_cast<std::uint8_t*>(data);
    for (std::uint32_t index = 0; index < count; ++index) bytes[index] ^= key;
}

inline constexpr std::size_t legacy_script_path_capacity = 260;
inline constexpr std::uint16_t squirrel_bytecode_tag = 0xfafau;

// 402D40 replaces the final FOUR BYTES in packed mode. It does not test for
// a .nut suffix. These length guards already existed in the reconstruction;
// they are not a newly inferred original-game rule. On rejection, keep path.
inline bool select_script_lookup_path(std::string& path, bool packed) {
    if (!packed) return true;
    if (path.size() >= legacy_script_path_capacity || path.size() < 4) return false;
    path.replace(path.size() - 4, 4, ".cv4");
    return true;
}

inline bool has_squirrel_bytecode_tag(const std::uint8_t* bytes,
                                     std::size_t size) noexcept {
    return size >= sizeof(std::uint16_t) && read_le16(bytes) == squirrel_bytecode_tag;
}

} // namespace kinoko::compat
