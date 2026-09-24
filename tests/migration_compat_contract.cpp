#include "kinoko/compat/resource_rules.hpp"
#include "kinoko/compat/archive_index.hpp"
#include <array>
#include <cstdio>
#include <string>

// Asset-free contract SOURCE. Building this target does not execute it.
#define CHECK(expression) do { if (!(expression)) { \
    std::fprintf(stderr, "migration compat line %d: %s\n", __LINE__, #expression); \
    return 1; } } while (false)

namespace compat = kinoko::compat;
constexpr std::uint8_t little_endian[] = {0x12, 0x34, 0x56, 0x78};
static_assert(compat::read_le16(little_endian) == 0x3412u);
static_assert(compat::read_le32(little_endian) == 0x78563412u);
static_assert(compat::archive_payload_key(0) == 0x23u);
static_assert(compat::archive_payload_key(0xffffffffu) == 0xffu);

int main() {
    struct PathCase { const char* input; const char* expected; };
    const PathCase paths[] = {
        {"", ""}, {".", "."}, {"./", ""}, {"./A\\B", "A/B"},
        {".\\A\\B", "./A/B"}, {"././A", "./A"}, {"A/../B", "A/../B"},
        {"A//B", "A//B"}, {"A.NUT", "A.NUT"}, {"\xc0\\A", "\xc0/A"}
    };
    for (const auto& row : paths)
        CHECK(compat::runtime_archive_lookup_path(row.input) == row.expected);

    const std::uint8_t bytecode[] = {0xfa, 0xfa, 0};
    const std::uint8_t text[] = {'a', 'b'};
    CHECK(!compat::has_squirrel_bytecode_tag(nullptr, 0));
    CHECK(!compat::has_squirrel_bytecode_tag(bytecode, 1));
    CHECK(compat::has_squirrel_bytecode_tag(bytecode, 2));
    CHECK(!compat::has_squirrel_bytecode_tag(text, 2));

    std::array<std::uint8_t, 4> data{0, 0xff, 0x12, 0x34};
    const auto original = data;
    compat::decode_archive_payload(data.data(), 2, 0x23);
    CHECK(data[0] == 0x23 && data[1] == 0xdc && data[2] == 0x12 && data[3] == 0x34);
    compat::decode_archive_payload(data.data(), 2, 0x23);
    CHECK(data == original);
    compat::decode_archive_payload(nullptr, 0, 0x23);
    // Synthetic index bytes only; no proprietary assets and no I/O.
    const std::uint8_t index_bytes[] = {0x78,0x56,0x34,0x12,4,0,0,0,3,'A','/','B'};
    compat::DatIndexCursor cursor(index_bytes, sizeof(index_bytes));
    compat::DatIndexEntry entry{};
    CHECK(cursor.next(entry));
    CHECK(entry.offset == 0x12345678u && entry.size == 4 && entry.path == "A/B");
    CHECK(cursor.position() == sizeof(index_bytes));
    CHECK(!cursor.next(entry) && entry.path == "A/B");
    compat::DatIndexCursor truncated(index_bytes, sizeof(index_bytes)-1);
    CHECK(!truncated.next(entry));
    CHECK(!truncated.next(entry));
    compat::DatIndexCursor missing(nullptr, 0);
    CHECK(!missing.next(entry));
    // State progression is part of the contract, including the zero-byte seed.
    std::mt19937 shared, expected;
    compat::decode_dat_index(nullptr, 0, shared);
    expected.seed(6);
    CHECK(shared() == expected());
    std::array<std::uint8_t, 8> encoded{};
    expected.seed(14);
    std::uint8_t key = 0xc5, step = 0x89;
    for (auto& byte : encoded) {
        byte = static_cast<std::uint8_t>(expected()) ^ key;
        key = static_cast<std::uint8_t>(key + step);
        step = static_cast<std::uint8_t>(step + 0x49);
    }
    auto decoded = encoded;
    compat::decode_dat_index(decoded.data(), static_cast<std::uint32_t>(decoded.size()), shared);
    CHECK((decoded == std::array<std::uint8_t, 8>{}));
    CHECK(shared() == expected());
    return 0;
}
