#include "kinoko/archive.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: kinoko_archive_inspect [--all] <archive.dat> [archive.dat ...]\n";
        return 2;
    }

    const bool print_all = std::string(argv[1]) == "--all";
    const int first_archive = print_all ? 2 : 1;
    if (first_archive >= argc) {
        std::cerr << "missing archive path\n";
        return 2;
    }

    int result = 0;
    for (int i = first_archive; i < argc; ++i) {
        kinoko::DatArchive archive;
        std::string error;
        if (!archive.open(std::filesystem::path(argv[i]),
                          static_cast<std::uint32_t>(i - first_archive), error)) {
            std::cerr << error << '\n';
            result = 1;
            continue;
        }
        const auto& summary = archive.summary();
        std::cout << summary.path.string()
                  << " entries=" << summary.entry_count
                  << " index=" << summary.index_size
                  << " file=" << summary.file_size << '\n';
        const auto& entries = archive.entries();
        const std::size_t preview_count = print_all
            ? entries.size() : std::min<std::size_t>(entries.size(), 12);
        for (std::size_t entry = 0; entry < preview_count; ++entry) {
            std::cout << "  " << entries[entry].path
                      << " offset=" << entries[entry].offset
                      << " size=" << entries[entry].size << '\n';
        }
    }
    return result;
}
