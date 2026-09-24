#include "kinoko/compat/resource_rules.hpp"
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
static_assert(compat::legacy_script_path_capacity == 260);

int main() {
    struct PathCase { const char* input; const char* expected; };
    const PathCase paths[] = {
        {"", ""}, {".", "."}, {"./", ""}, {"./A\\B", "A/B"},
        {".\\A\\B", "./A/B"}, {"././A", "./A"}, {"A/../B", "A/../B"},
        {"A//B", "A//B"}, {"A.NUT", "A.NUT"}, {"\xc0\\A", "\xc0/A"}
    };
    for (const auto& row : paths)
        CHECK(compat::runtime_archive_lookup_path(row.input) == row.expected);

    std::string plain(300, 'x');
    CHECK(compat::select_script_lookup_path(plain, false) && plain == std::string(300, 'x'));
    for (const auto* input : {"", "a", "abc"}) {
        std::string path(input);
        CHECK(!compat::select_script_lookup_path(path, true));
        CHECK(path == input);
    }
    std::string source = "data/script/boot.nut";
    CHECK(compat::select_script_lookup_path(source, true) && source == "data/script/boot.cv4");
    std::string non_suffix = "hello.txt";
    CHECK(compat::select_script_lookup_path(non_suffix, true) && non_suffix == "hello.cv4");
    std::string four = "ABCD";
    CHECK(compat::select_script_lookup_path(four, true) && four == ".cv4");
    std::string maximum(259, 'x');
    CHECK(compat::select_script_lookup_path(maximum, true));
    CHECK(maximum == std::string(255, 'x') + ".cv4");
    std::string rejected(260, 'x');
    CHECK(!compat::select_script_lookup_path(rejected, true));
    CHECK(rejected == std::string(260, 'x'));

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
    return 0;
}
