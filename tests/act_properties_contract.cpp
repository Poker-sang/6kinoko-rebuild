#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using kinoko::legacy::address;
using kinoko::legacy::pointer;
namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
struct Reader { const unsigned char* bytes; size_t size; size_t position = 0; };
struct StringWrite { int32_t destination = 0; std::string value; unsigned calls = 0; } last_string;
std::vector<unsigned char> fixture;
template<class T> void append(T value) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
    fixture.insert(fixture.end(), bytes, bytes + sizeof value);
}
void name(const char* text, uint32_t type) {
    const auto length = static_cast<uint32_t>(std::strlen(text));
    append(length); fixture.insert(fixture.end(), text, text + length); append(type);
}
void parse_contract() {
    fixture.clear(); append<uint8_t>(1); append<uint32_t>(4);
    name("integer", 0); name("real", 1); name("boolean", 2); name("string", 3);
    append<int32_t>(-42); append<float>(3.25f); append<uint8_t>(7);
    append<uint32_t>(3); append<uint8_t>('A'); append<uint8_t>(0); append<uint8_t>('B');
    Reader reader{fixture.data(), fixture.size()};
    retdec_act_property* properties = nullptr; uint32_t count = 0;
    require(retdec_act_read_properties(address(&reader), &properties, &count) == 1, "actual parser succeeds");
    require(count == 4 && reader.position == fixture.size(), "two-pass format consumed exactly");
    require(properties[0].integer == -42 && properties[1].real == 3.25f && properties[2].integer == 1, "typed scalar values");
    require(properties[3].string_length == 3 && std::memcmp(properties[3].string, "A\0B", 3) == 0 && properties[3].string[3] == 0, "string bytes and terminator");
    retdec_act_free_properties(properties, count);
    for (size_t length = 0; length < fixture.size(); ++length) {
        Reader truncated{fixture.data(), length};
        properties = reinterpret_cast<retdec_act_property*>(1); count = 99;
        require(retdec_act_read_properties(address(&truncated), &properties, &count) == 0, "every truncation rejected");
        require(properties == nullptr && count == 0, "failed parse never publishes partial ownership");
    }
    const auto valid = fixture;
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        fixture = valid; fixture.resize(fixture.size() - 1);
        Reader truncated{fixture.data(), fixture.size()};
        require(!retdec_act_read_properties(address(&truncated), &properties, &count), "repeat partial string read cleanup");
    }
    auto reject = [&](size_t offset, uint32_t value) {
        fixture = valid; std::memcpy(fixture.data() + offset, &value, sizeof value);
        Reader invalid{fixture.data(), fixture.size()};
        require(!retdec_act_read_properties(address(&invalid), &properties, &count), "invalid property limit/type rejected");
        require(!properties && !count, "invalid input outputs reset");
    };
    reject(1, 1025); reject(5, 4097); reject(5 + 4 + 7, 4); reject(valid.size() - 7, 0x100001);
    fixture = {0}; reader = {fixture.data(), fixture.size()};
    require(retdec_act_read_properties(address(&reader), &properties, &count) && !properties && !count, "absent block");
    fixture.clear(); append<uint8_t>(1); append<uint32_t>(0); reader = {fixture.data(), fixture.size()};
    require(retdec_act_read_properties(address(&reader), &properties, &count) && !properties && !count, "empty block");
    require(!retdec_act_read_properties(address(&reader), nullptr, &count), "null output rejected");
    require(!retdec_act_read_properties(address(&reader), &properties, nullptr), "null count rejected");
}
using Apply = void(*)(int32_t, retdec_act_property*, uint32_t);
enum class Kind { Integer, Real, Boolean, String };
// Independent expected offsets retained from the pre-migration loader. Compare
// every byte, not just the selected field, so adjacent fields remain protected.
struct Expected { Apply apply; const char* name; uint32_t offset; Kind kind; };
const Expected expected[] = {
    {retdec_act_apply_cact, "resolutionMs", 4, Kind::Integer},
    {retdec_act_apply_cact, "screenWidth", 8, Kind::Integer},
    {retdec_act_apply_cact, "screenHeight", 12, Kind::Integer},
    {retdec_act_apply_cact, "stName", 16, Kind::String},
    {retdec_act_apply_cact, "offsetX", 88, Kind::Real},
    {retdec_act_apply_cact, "offsetY", 92, Kind::Real},
    {retdec_act_apply_cact, "marginBottom", 84, Kind::Integer},
    {retdec_act_apply_cact, "marginLeft", 72, Kind::Integer},
    {retdec_act_apply_cact, "marginRight", 80, Kind::Integer},
    {retdec_act_apply_cact, "marginTop", 76, Kind::Integer},
    {retdec_act_apply_cact, "visible", 96, Kind::Boolean},
    {retdec_act_apply_script, "filePath", 64, Kind::String},
    {retdec_act_apply_script, "compiled", 101, Kind::Boolean},
    {retdec_act_apply_layer, "resourceID", 0x60, Kind::Integer},
    {retdec_act_apply_layer, "layerID", 0x68, Kind::Integer},
    {retdec_act_apply_layer, "stName", 0x70, Kind::String},
    {retdec_act_apply_layer, "visible", 0x8c, Kind::Boolean},
    {retdec_act_apply_layer, "debugOnly", 0x8d, Kind::Boolean},
    {retdec_act_apply_layer, "dst_x", 0x90, Kind::Real},
    {retdec_act_apply_layer, "dst_y", 0x94, Kind::Real},
    {retdec_act_apply_layer, "dst_z", 0x98, Kind::Real},
    {retdec_act_apply_layer, "ox", 0x9c, Kind::Real},
    {retdec_act_apply_layer, "oy", 0xa0, Kind::Real},
    {retdec_act_apply_layer, "oz", 0xa4, Kind::Real},
    {retdec_act_apply_layer, "parentID", 0x6c, Kind::Integer},
    {retdec_act_apply_layer, "prev_x", 0xa8, Kind::Real},
    {retdec_act_apply_layer, "prev_y", 0xac, Kind::Real},
    {retdec_act_apply_layer, "prev_z", 0xb0, Kind::Real},
    {retdec_act_apply_layout, "alpha", 0x11c, Kind::Real},
    {retdec_act_apply_layout, "blend", 0x120, Kind::Integer},
    {retdec_act_apply_layout, "colorB", 0x12c, Kind::Integer},
    {retdec_act_apply_layout, "colorG", 0x128, Kind::Integer},
    {retdec_act_apply_layout, "colorR", 0x124, Kind::Integer},
    {retdec_act_apply_layout, "cor_x", 0xf8, Kind::Real},
    {retdec_act_apply_layout, "cor_y", 0xfc, Kind::Real},
    {retdec_act_apply_layout, "cor_z", 0x100, Kind::Real},
    {retdec_act_apply_layout, "cos_x", 0x110, Kind::Real},
    {retdec_act_apply_layout, "cos_y", 0x114, Kind::Real},
    {retdec_act_apply_layout, "cos_z", 0x118, Kind::Real},
    {retdec_act_apply_layout, "roll.x", 0xec, Kind::Real},
    {retdec_act_apply_layout, "roll.y", 0xf0, Kind::Real},
    {retdec_act_apply_layout, "roll.z", 0xf4, Kind::Real},
    {retdec_act_apply_layout, "scale.x", 0x104, Kind::Real},
    {retdec_act_apply_layout, "scale.y", 0x108, Kind::Real},
    {retdec_act_apply_layout, "scale.z", 0x10c, Kind::Real},
    {retdec_act_apply_map_layout, "layerType", 236, Kind::Integer},
    {retdec_act_apply_map_layout, "maxChipWidth", 240, Kind::Integer},
    {retdec_act_apply_map_layout, "maxChipHeight", 244, Kind::Integer},
    {retdec_act_apply_map_layout, "mapChipLeft", 248, Kind::Integer},
    {retdec_act_apply_map_layout, "mapChipRight", 256, Kind::Integer},
    {retdec_act_apply_map_layout, "mapChipTop", 252, Kind::Integer},
    {retdec_act_apply_map_layout, "mapChipBottom", 260, Kind::Integer},
    {retdec_act_apply_map_layout, "alpha", 320, Kind::Real},
    {retdec_act_apply_map_layout, "scale", 324, Kind::Real},
    {retdec_act_apply_map_layout, "blend", 328, Kind::Integer},
    {retdec_act_apply_resource, "resourceID", 4, Kind::Integer},
    {retdec_act_apply_resource, "stName", 8, Kind::String},
    {retdec_act_apply_resource, "stTextureName", 40, Kind::String},
    {retdec_act_apply_resource, "image_width", 72, Kind::Integer},
    {retdec_act_apply_resource, "image_height", 76, Kind::Integer},
    {retdec_act_apply_resource, "src_x", 80, Kind::Real},
    {retdec_act_apply_resource, "src_y", 84, Kind::Real},
    {retdec_act_apply_resource, "src_width", 88, Kind::Real},
    {retdec_act_apply_resource, "src_height", 92, Kind::Real},
    {retdec_act_apply_chip_resource, "resourceID", 4, Kind::Integer},
    {retdec_act_apply_chip_resource, "stName", 8, Kind::String},
    {retdec_act_apply_chip_resource, "stChipFile", 36, Kind::String},
};
void property_contract() {
    for (const auto& e : expected) {
        for (uint32_t type = 0; type < 4; ++type) {
            std::array<unsigned char, 512> actual, reference;
            actual.fill(0xcd); reference = actual;
            retdec_act_property property{const_cast<char*>(e.name), type, -9, 12.5f, const_cast<char*>("A\0B"), 3};
            last_string = {};
            e.apply(address(actual.data()), &property, 1);
            const int32_t integer = type == 1 ? 12 : -9;
            const float real = type == 1 ? 12.5f : -9.0f;
            const uint8_t boolean = integer != 0;
            if (e.kind == Kind::Integer) std::memcpy(reference.data() + e.offset, &integer, sizeof integer);
            if (e.kind == Kind::Real) std::memcpy(reference.data() + e.offset, &real, sizeof real);
            if (e.kind == Kind::Boolean) std::memcpy(reference.data() + e.offset, &boolean, sizeof boolean);
            const bool string = e.kind == Kind::String && type == 3;
            require(last_string.calls == (string ? 1u : 0u), "string type coercion rule");
            if (string) require(last_string.destination == address(actual.data() + e.offset) && last_string.value == std::string("A\0B", 3), "exact string member and byte length");
            require(actual == reference, e.name);
        }
        std::array<unsigned char, 512> actual{};
        retdec_act_property unknown{const_cast<char*>("not-a-property"), 0, 42, 0, nullptr, 0};
        const auto reference = actual;
        e.apply(address(actual.data()), &unknown, 1);
        require(actual == reference, "unknown names do not write any fields");
    }
    std::array<unsigned char, 512> actual{};
    retdec_act_property duplicate[2] = {
        {const_cast<char*>("screenWidth"), 0, 640, 0, nullptr, 0},
        {const_cast<char*>("screenWidth"), 0, 800, 0, nullptr, 0}
    };
    retdec_act_apply_cact(address(actual.data()), duplicate, 2);
    int32_t width; std::memcpy(&width, actual.data() + 8, sizeof width);
    require(width == 800, "last duplicate wins in serialized order");
    for (int32_t value : {0, -1, 1}) {
        retdec_act_property visible{const_cast<char*>("visible"), 0, value, 0, nullptr, 0};
        retdec_act_apply_cact(address(actual.data()), &visible, 1);
        require(actual[96] == (value != 0), "boolean is normalized to one byte");
    }
}
}
extern "C" {
int32_t g765 = 0;
int32_t retdec_reader_read_exact(int32_t id, void* output, uint32_t count) {
    auto& reader = *pointer<Reader>(id);
    if (reader.position > reader.size || count > reader.size - reader.position) return 0;
    std::memcpy(output, reader.bytes + reader.position, count); reader.position += count; return 1;
}
int32_t retdec_string_assign_n(int32_t* destination, const char* bytes, uint32_t count) {
    last_string.destination = address(destination); last_string.value.assign(bytes, count); ++last_string.calls; return address(destination);
}
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
}
int main() {
    parse_contract(); property_contract();
    std::printf("PASS: ACT two-pass parser, all truncations, limits, ownership outputs, %zu property mappings and all coercions\n", sizeof expected / sizeof *expected);
}
