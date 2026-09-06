#include "kinoko/archive.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string hex_prefix(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream output;
    const std::size_t count = std::min<std::size_t>(bytes.size(), 64);
    for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
            output << ' ';
        }
        output << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>(bytes[i]);
    }
    return output.str();
}

std::string ascii_prefix(const std::vector<std::uint8_t>& bytes) {
    std::string output;
    const std::size_t count = std::min<std::size_t>(bytes.size(), 64);
    output.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const char value = static_cast<char>(bytes[i]);
        output.push_back(value >= 0x20 && value <= 0x7e ? value : '.');
    }
    return output;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "usage: kinoko_asset_probe <data-directory> <asset-path> [output-file]\n";
        return 2;
    }

    kinoko::AssetStore store;
    std::string error;
    if (!store.mount_standard_set(std::filesystem::path(argv[1]), error)) {
        std::cerr << error << '\n';
        return 1;
    }

    const auto entry = store.find(argv[2]);
    if (!entry) {
        std::cerr << "asset not found: " << argv[2] << '\n';
        return 1;
    }
    std::vector<std::uint8_t> bytes;
    if (!store.read(argv[2], bytes, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    if (argc == 4) {
        std::ofstream output(argv[3], std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        if (!output) {
            std::cerr << "cannot write asset: " << argv[3] << '\n';
            return 1;
        }
    }

    std::cout << "path=" << entry->path << '\n'
              << "archive=" << entry->archive_index << '\n'
              << "offset=" << entry->offset << '\n'
              << "size=" << entry->size << '\n'
              << "hex=" << hex_prefix(bytes) << '\n'
              << "ascii=" << ascii_prefix(bytes) << '\n';
    return 0;
}
