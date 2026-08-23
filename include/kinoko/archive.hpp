#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kinoko {

struct ArchiveEntry {
    std::string path;
    std::uint32_t archive_index = 0;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
};

struct ArchiveSummary {
    std::filesystem::path path;
    std::uint16_t entry_count = 0;
    std::uint32_t index_size = 0;
    std::uint64_t file_size = 0;
};

std::string normalize_asset_path(std::string_view path);

class DatArchive {
public:
    bool open(const std::filesystem::path& path,
              std::uint32_t archive_index,
              std::string& error);

    bool read(const ArchiveEntry& entry,
              std::vector<std::uint8_t>& bytes,
              std::string& error) const;

    const ArchiveSummary& summary() const { return summary_; }
    const std::vector<ArchiveEntry>& entries() const { return entries_; }

private:
    ArchiveSummary summary_;
    std::vector<ArchiveEntry> entries_;
};

struct AssetLocation {
    std::size_t archive_index = 0;
    std::size_t entry_index = 0;
};

class AssetStore {
public:
    bool mount(const std::filesystem::path& path, std::string& error);
    bool mount_standard_set(const std::filesystem::path& directory,
                            std::string& error);

    std::optional<ArchiveEntry> find(std::string_view path) const;
    bool read(std::string_view path,
              std::vector<std::uint8_t>& bytes,
              std::string& error) const;

    const std::vector<DatArchive>& archives() const { return archives_; }
    std::size_t entry_count() const { return locations_.size(); }
    std::size_t archive_count() const { return archives_.size(); }

private:
    std::vector<DatArchive> archives_;
    std::vector<std::string> mounted_paths_;
    std::unordered_map<std::string, AssetLocation> locations_;
};

} // namespace kinoko
