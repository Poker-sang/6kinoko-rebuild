#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/legacy_string.hpp"
#include <cstdint>
#include <map>
#include <string>

extern "C" unsigned char g673;

namespace {
using kinoko::legacy::address;
using kinoko::legacy::field;
using kinoko::legacy::pointer;
struct Property {
    uint32_t type;
    uint32_t read_type;
    uint32_t offset;
};
// Original TUserData<CActResource2D> owns descriptors by name. Native value
// ownership replaces its decompiled map/shared-pointer allocation machinery.
// Type 12 is an unknown member; read type 24 is absent from the latest schema.
using Schema = std::map<std::string, Property>;
Schema make_schema() { return {
    {"image_height", {0,0,76}}, {"image_width", {0,0,72}},
    {"resourceID", {0,0,4}}, {"src_height", {1,1,92}},
    {"src_width", {1,1,88}}, {"src_x", {1,1,80}}, {"src_y", {1,1,84}},
    {"stName", {3,3,8}}, {"stTextureName", {3,3,40}}
}; }
Schema texture_schema = make_schema();
// 4493F0 registers the same inherited fields, but 449AB0 uses a distinct
// TUserData<CActRenderTarget> schema. Never let one class's header alter another.
Schema render_target_schema = make_schema();
// 42F2A0: chip resources serialize the base ID/name and the MCD filename.
Schema chip_schema{{"resourceID", {0,0,4}}, {"stName", {3,3,8}},
                   {"stChipFile", {3,3,36}}};
bool transfer(int32_t stream, void* bytes, uint32_t size) {
    return stream && (retdec_call_thiscall2_result(pointer<void>(stream),
        field<void*>(field<int32_t>(stream)+12), address(bytes), size) & 0xff) != 0;
}
template<class T> bool transfer(int32_t stream, T& value) {
    return transfer(stream, &value, sizeof(value));
}
bool read_string(int32_t stream, std::string& text, uint32_t limit) {
    uint32_t size = 0;
    if (!transfer(stream, size) || size > limit) return false;
    text.resize(size);
    return !size || transfer(stream, text.data(), size);
}
bool write_string(int32_t stream, const char* text, uint32_t size) {
    return transfer(stream, size) && (!size || transfer(stream, const_cast<char*>(text), size));
}
bool read(int32_t resource, int32_t reader, Schema& schema, bool texture) {
    uint8_t has_schema = 1;
    if (!transfer(reader, has_schema)) return false;
    if (has_schema) {
        uint32_t count = 0;
        if (!transfer(reader, count) || count > 1024) return false;
        // 4475C8 marks every old entry absent; known native descriptors survive.
        for (auto& entry : schema) entry.second.read_type = 24;
        for (uint32_t i=0; i<count; ++i) {
            std::string name;
            uint32_t type = 0;
            if (!read_string(reader, name, 4096) || !transfer(reader, type) || type > 3) return false;
            auto result = schema.emplace(name, Property{12, type+12, 0});
            auto& property = result.first->second;
            property.read_type = property.type == type ? type : type+12;
        }
    }
    // Original 44D4C0 consumes values in map key order, not header order.
    for (const auto& entry : schema) {
        const auto& property = entry.second;
        if (property.read_type == 24) continue;
        const bool assign = property.read_type < 12;
        const auto type = assign ? property.read_type : property.read_type-12;
        if (type == 3) {
            std::string text;
            if (!read_string(reader, text, 0x100000)) return false;
            if (assign) kinoko::legacy::StringView(pointer<void>(resource+property.offset)).assign(
                text.data(), static_cast<uint32_t>(text.size()));
        } else if (type == 2) {
            uint8_t ignored;
            if (!transfer(reader, ignored)) return false;
        } else {
            uint32_t bits;
            if (!transfer(reader, bits)) return false;
            if (assign) field<uint32_t>(resource+property.offset) = bits;
        }
    }
    // 446A84: serialized crop rectangles disable constructor auto-size.
    if (texture) field<uint8_t>(resource+96) = 0;
    return true;
}
bool write(int32_t resource, int32_t writer, const Schema& schema) {
    uint8_t has_schema = !g673;
    if (!transfer(writer, has_schema)) return false;
    if (has_schema) {
        auto count = static_cast<uint32_t>(schema.size());
        if (!transfer(writer, count)) return false;
        for (const auto& entry : schema) {
            auto type = entry.second.type;
            if (!write_string(writer, entry.first.data(), static_cast<uint32_t>(entry.first.size())) ||
                !transfer(writer, type)) return false;
        }
    }
    for (const auto& entry : schema) {
        const auto& property = entry.second;
        if (property.type == 12) continue;
        if (property.type == 3) {
            const kinoko::legacy::StringView text(pointer<void>(resource+property.offset));
            if (!write_string(writer, text.data(), text.length())) return false;
        } else if (!transfer(writer, pointer<void>(resource+property.offset), 4)) return false;
    }
    return true;
}
}

extern "C" int32_t __fastcall kinoko_method_read_texture_resource(
    int32_t resource, void*, int32_t holder, int32_t version) {
    if (!resource || !holder || version != 1) return 0;
    try { return read(resource, field<int32_t>(holder), texture_schema, true); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_write_texture_resource(
    int32_t resource, void*, int32_t writer) {
    if (!resource || !writer) return 0;
    try { return write(resource, writer, texture_schema); }
    catch (...) { return 0; }
}

extern "C" int32_t __fastcall kinoko_method_read_render_target(
    int32_t resource, void*, int32_t holder, int32_t version) {
    if (!resource || !holder || version != 1) return 0;
    try { return read(resource, field<int32_t>(holder), render_target_schema, true); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_write_render_target(
    int32_t resource, void*, int32_t writer) {
    if (!resource || !writer) return 0;
    try { return write(resource, writer, render_target_schema); }
    catch (...) { return 0; }
}

extern "C" int32_t __fastcall kinoko_method_read_chip_resource(
    int32_t resource, void*, int32_t holder, int32_t version) {
    if (!resource || !holder || version != 1) return 0;
    try { return read(resource, field<int32_t>(holder), chip_schema, false); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_write_chip_resource(
    int32_t resource, void*, int32_t writer) {
    if (!resource || !writer) return 0;
    try { return write(resource, writer, chip_schema); }
    catch (...) { return 0; }
}
