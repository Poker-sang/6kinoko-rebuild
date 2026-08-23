#include "kinoko/archive.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: kinoko_archive_smoke <data-directory>\n";
        return 2;
    }

    const std::filesystem::path directory(argv[1]);
    const std::uint16_t expected_counts[] = {0x0666u, 0x0034u, 0x008au};
    kinoko::AssetStore store;
    std::string error;
    if (!store.mount_standard_set(directory, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    if (store.archive_count() != 3) {
        std::cerr << "unexpected archive count: " << store.archive_count() << '\n';
        return 1;
    }

    std::size_t expected_total = 0;
    for (std::size_t i = 0; i < store.archives().size(); ++i) {
        const auto& archive = store.archives()[i];
        if (archive.summary().entry_count != expected_counts[i]) {
            std::cerr << "unexpected entry count in archive " << i << '\n';
            return 1;
        }
        expected_total += archive.entries().size();
        if (archive.entries().empty()) {
            std::cerr << "archive has no entries: " << archive.summary().path.string() << '\n';
            return 1;
        }

        std::vector<std::uint8_t> bytes;
        if (!archive.read(archive.entries().front(), bytes, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        if (bytes.size() != archive.entries().front().size) {
            std::cerr << "asset read size mismatch\n";
            return 1;
        }
    }

    if (store.entry_count() == 0 || store.entry_count() > expected_total) {
        std::cerr << "invalid deduplicated entry count\n";
        return 1;
    }
    const auto& first = store.archives().front().entries().front();
    std::vector<std::uint8_t> bytes;
    if (!store.read(first.path, bytes, error)) {
        std::cerr << error << '\n';
        return 1;
    }

    std::cout << "archive smoke ok: archives=" << store.archive_count()
              << " indexed=" << expected_total
              << " unique=" << store.entry_count()
              << " first=" << first.path
              << " bytes=" << bytes.size() << '\n';
    return 0;
}

