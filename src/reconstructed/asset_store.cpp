#include "kinoko/archive.hpp"

namespace kinoko {

bool AssetStore::mount(const std::filesystem::path& path, std::string& error) {
    DatArchive archive;
    const std::uint32_t archive_index = static_cast<std::uint32_t>(archives_.size());
    if (!archive.open(path, archive_index, error)) {
        return false;
    }

    archives_.push_back(std::move(archive));
    const DatArchive& mounted = archives_.back();
    for (std::size_t i = 0; i < mounted.entries().size(); ++i) {
        locations_[mounted.entries()[i].path] = {archives_.size() - 1, i};
    }
    mounted_paths_.push_back(path.string());
    return true;
}

bool AssetStore::mount_standard_set(const std::filesystem::path& directory,
                                    std::string& error) {
    bool ok = true;
    const char* names[] = {
        "6kinoko_a.dat",
        "6kinoko_b.dat",
        "6kinoko_c.dat",
    };
    for (const char* name : names) {
        std::string mount_error;
        if (!mount(directory / name, mount_error)) {
            ok = false;
            if (!error.empty()) {
                error += "\n";
            }
            error += mount_error;
        }
    }
    return ok;
}

std::optional<ArchiveEntry> AssetStore::find(std::string_view path) const {
    const auto normalized = normalize_asset_path(path);
    const auto location = locations_.find(normalized);
    if (location == locations_.end()) {
        return std::nullopt;
    }
    const AssetLocation& value = location->second;
    if (value.archive_index >= archives_.size()) {
        return std::nullopt;
    }
    const auto& entries = archives_[value.archive_index].entries();
    if (value.entry_index >= entries.size()) {
        return std::nullopt;
    }
    return entries[value.entry_index];
}

bool AssetStore::read(std::string_view path,
                      std::vector<std::uint8_t>& bytes,
                      std::string& error) const {
    const auto normalized = normalize_asset_path(path);
    const auto location = locations_.find(normalized);
    if (location == locations_.end()) {
        error = "asset not found: " + normalized;
        return false;
    }
    const AssetLocation& value = location->second;
    if (value.archive_index >= archives_.size()) {
        error = "asset archive index is invalid: " + normalized;
        return false;
    }
    const auto& entries = archives_[value.archive_index].entries();
    if (value.entry_index >= entries.size()) {
        error = "asset entry index is invalid: " + normalized;
        return false;
    }
    return archives_[value.archive_index].read(entries[value.entry_index], bytes, error);
}

} // namespace kinoko
