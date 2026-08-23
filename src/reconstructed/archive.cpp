#include "kinoko/archive.hpp"

#include <algorithm>
#include <array>
#include <fstream>

namespace kinoko {
namespace {

class PackagePrng {
public:
    explicit PackagePrng(std::uint32_t seed) {
        state_[0] = seed;
        for (std::size_t i = 1; i < state_.size(); ++i) {
            state_[i] = 0x6c078965u * (state_[i - 1] ^ (state_[i - 1] >> 30))
                        + static_cast<std::uint32_t>(i);
        }
        index_ = 624;
    }

    std::uint32_t next() {
        if (index_ >= 624) {
            if (index_ == 625) {
                reseed(5489u);
            }
            twist();
            index_ = 0;
        }

        const std::uint32_t value = state_[index_];
        ++index_;

        std::uint32_t mixed = value ^ (value >> 11);
        mixed ^= (mixed & 0xff3a58adu) << 7;
        mixed ^= (mixed & 0xffffdf8cu) << 15;
        mixed ^= mixed >> 18;
        return mixed;
    }

private:
    void reseed(std::uint32_t seed) {
        state_[0] = seed;
        for (std::size_t i = 1; i < state_.size(); ++i) {
            state_[i] = 0x6c078965u * (state_[i - 1] ^ (state_[i - 1] >> 30))
                        + static_cast<std::uint32_t>(i);
        }
        index_ = 624;
    }

    void twist() {
        constexpr std::uint32_t kMatrix[2] = {0u, 0x9908b0dfu};
        for (std::size_t i = 0; i < 227; ++i) {
            const std::uint32_t mixed = state_[i]
                                        ^ ((state_[i] ^ state_[i + 1]) & 0x7fffffffu);
            state_[i] = state_[i + 397] ^ (mixed >> 1) ^ kMatrix[mixed & 1u];
        }
        for (std::size_t i = 227; i < 623; ++i) {
            const std::uint32_t mixed = state_[i]
                                        ^ ((state_[i] ^ state_[i + 1]) & 0x7fffffffu);
            state_[i] = state_[i - 227] ^ (mixed >> 1) ^ kMatrix[mixed & 1u];
        }
        const std::uint32_t mixed = state_[623]
                                    ^ ((state_[623] ^ state_[0]) & 0x7fffffffu);
        state_[623] = state_[396] ^ (mixed >> 1) ^ kMatrix[mixed & 1u];
    }

    std::array<std::uint32_t, 624> state_{};
    std::size_t index_ = 625;
};

std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset])
           | static_cast<std::uint16_t>(bytes[offset + 1] << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset])
           | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
           | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
           | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

void append_error(std::string& error, const std::string& message) {
    if (!error.empty()) {
        error += "; ";
    }
    error += message;
}

} // namespace

std::string normalize_asset_path(std::string_view path) {
    std::string normalized;
    normalized.reserve(path.size());
    for (const char value : path) {
        const char slash = value == '\\' ? '/' : value;
        normalized.push_back(static_cast<char>(
            slash >= 'A' && slash <= 'Z' ? slash + ('a' - 'A') : slash));
    }
    while (normalized.rfind("./", 0) == 0) {
        normalized.erase(0, 2);
    }
    return normalized;
}

bool DatArchive::open(const std::filesystem::path& path,
                      std::uint32_t archive_index,
                      std::string& error) {
    entries_.clear();
    summary_ = {};
    summary_.path = path;

    std::error_code file_error;
    const std::uint64_t file_size = std::filesystem::file_size(path, file_error);
    if (file_error) {
        error = "cannot stat " + path.string() + ": " + file_error.message();
        return false;
    }
    if (file_size < 6) {
        error = "DAT is shorter than its 6-byte header: " + path.string();
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open " + path.string();
        return false;
    }

    std::array<std::uint8_t, 6> header{};
    file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (file.gcount() != static_cast<std::streamsize>(header.size())) {
        error = "cannot read DAT header: " + path.string();
        return false;
    }

    const std::uint16_t entry_count = static_cast<std::uint16_t>(header[0])
                                      | static_cast<std::uint16_t>(header[1] << 8);
    const std::uint32_t index_size = static_cast<std::uint32_t>(header[2])
                                     | (static_cast<std::uint32_t>(header[3]) << 8)
                                     | (static_cast<std::uint32_t>(header[4]) << 16)
                                     | (static_cast<std::uint32_t>(header[5]) << 24);
    if (index_size > file_size - 6) {
        error = "DAT index exceeds file size: " + path.string();
        return false;
    }
    if (index_size > 256u * 1024u * 1024u) {
        error = "DAT index is unexpectedly large: " + path.string();
        return false;
    }

    std::vector<std::uint8_t> index(index_size);
    if (index_size != 0) {
        file.read(reinterpret_cast<char*>(index.data()), static_cast<std::streamsize>(index.size()));
        if (file.gcount() != static_cast<std::streamsize>(index.size())) {
            error = "cannot read DAT index: " + path.string();
            return false;
        }
    }

    PackagePrng prng(index_size + 6u);
    for (std::uint8_t& value : index) {
        value ^= static_cast<std::uint8_t>(prng.next());
    }

    std::uint8_t first_key = 0xc5u;
    std::uint8_t second_key = 0x89u;
    for (std::uint8_t& value : index) {
        value ^= first_key;
        first_key = static_cast<std::uint8_t>(first_key + second_key);
        second_key = static_cast<std::uint8_t>(second_key + 0x49u);
    }

    summary_.entry_count = entry_count;
    summary_.index_size = index_size;
    summary_.file_size = file_size;
    entries_.reserve(entry_count);

    std::size_t cursor = 0;
    for (std::uint32_t i = 0; i < entry_count; ++i) {
        if (cursor + 9 > index.size()) {
            error = "DAT index ended before entry " + std::to_string(i) + ": " + path.string();
            entries_.clear();
            return false;
        }
        // The original resource node stores the first DWORD as the file
        // offset and the second DWORD as the byte count.
        const std::uint32_t offset = read_u32(index, cursor);
        const std::uint32_t size = read_u32(index, cursor + 4);
        const std::size_t path_size = index[cursor + 8];
        cursor += 9;
        if (cursor + path_size > index.size()) {
            error = "DAT path exceeds index at entry " + std::to_string(i) + ": " + path.string();
            entries_.clear();
            return false;
        }
        if (static_cast<std::uint64_t>(offset) + size > file_size) {
            error = "DAT entry points outside file at entry " + std::to_string(i)
                    + " (size=" + std::to_string(size)
                    + ", offset=" + std::to_string(offset)
                    + ", path_length=" + std::to_string(path_size)
                    + "): " + path.string();
            entries_.clear();
            return false;
        }

        std::string asset_path(reinterpret_cast<const char*>(index.data() + cursor), path_size);
        entries_.push_back({normalize_asset_path(asset_path), archive_index, offset, size});
        cursor += path_size;
    }

    return true;
}

bool DatArchive::read(const ArchiveEntry& entry,
                      std::vector<std::uint8_t>& bytes,
                      std::string& error) const {
    if (entries_.empty() || entry.archive_index != entries_.front().archive_index) {
        error = "entry belongs to a different archive";
        return false;
    }
    if (static_cast<std::uint64_t>(entry.offset) + entry.size > summary_.file_size) {
        error = "entry exceeds archive bounds: " + entry.path;
        return false;
    }

    std::ifstream file(summary_.path, std::ios::binary);
    if (!file) {
        error = "cannot open archive for asset read: " + summary_.path.string();
        return false;
    }
    file.seekg(static_cast<std::streamoff>(entry.offset), std::ios::beg);
    if (!file) {
        error = "cannot seek to asset: " + entry.path;
        return false;
    }

    bytes.resize(entry.size);
    if (entry.size != 0) {
        file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (file.gcount() != static_cast<std::streamsize>(bytes.size())) {
            bytes.clear();
            error = "cannot read asset: " + entry.path;
            return false;
        }
    }
    return true;
}

} // namespace kinoko
