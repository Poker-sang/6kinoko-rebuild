#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/act_runtime.h"
#include <algorithm>
#include <array>
#include <climits>
#include <vector>
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
// 42BD00 registers the 17 serialized C2DLayout members.
Schema layout_schema{
    {"roll.x", {1,1,236}}, {"roll.y", {1,1,240}}, {"roll.z", {1,1,244}},
    {"cor_x", {1,1,248}}, {"cor_y", {1,1,252}}, {"cor_z", {1,1,256}},
    {"scale.x", {1,1,260}}, {"scale.y", {1,1,264}}, {"scale.z", {1,1,268}},
    {"cos_x", {1,1,272}}, {"cos_y", {1,1,276}}, {"cos_z", {1,1,280}},
    {"alpha", {1,1,284}}, {"blend", {0,0,288}},
    {"colorR", {0,0,292}}, {"colorG", {0,0,296}}, {"colorB", {0,0,300}}
};
// Original 43C6B0 registers trans.* and roll.* at the same offsets. Preserve
// that alias instead of inferring a different layout from the property names.
Schema layout3d_schema{
    {"trans.x", {1,1,16}}, {"trans.y", {1,1,20}}, {"trans.z", {1,1,24}},
    {"roll.x", {1,1,16}}, {"roll.y", {1,1,20}}, {"roll.z", {1,1,24}},
    {"scale.x", {1,1,28}}, {"scale.y", {1,1,32}}, {"scale.z", {1,1,36}}
};
// 434760 registers nine fields; blend is runtime state, not a property here.
Schema map_schema{
    {"layerType", {0,0,236}}, {"maxChipWidth", {0,0,240}},
    {"maxChipHeight", {0,0,244}}, {"mapChipLeft", {0,0,248}},
    {"mapChipTop", {0,0,252}}, {"mapChipRight", {0,0,256}},
    {"mapChipBottom", {0,0,260}}, {"alpha", {1,1,320}}, {"scale", {1,1,324}}
};
using MapRecord = std::array<int32_t, 8>;
uint32_t record_count(int32_t layout) {
    const auto begin = field<uint32_t>(layout+264), end = field<uint32_t>(layout+268);
    if (end < begin || (end-begin)%32 || (!begin && end) || (end-begin)/32 > 0x10000)
        throw std::bad_alloc();
    return (end-begin)/32;
}
void replace_buffer(int32_t slot, const void* bytes, size_t size) {
    kinoko::legacy::Allocation<unsigned char> allocation(
        size ? static_cast<unsigned char*>(std::malloc(size)) : nullptr);
    if (size && !allocation) throw std::bad_alloc();
    if (size) std::memcpy(allocation.get(), bytes, size);
    std::free(pointer<void>(field<int32_t>(slot)));
    field<int32_t>(slot) = address(allocation.release());
    field<uint32_t>(slot+4) = field<uint32_t>(slot+8) = field<uint32_t>(slot)+size;
}
// Original 435860/435B20, adapted to the source-owned MCD rather than an old
// MSVC tree. The ABI caches still contain flat records and an ID/index vector.
void prepare_map(int32_t layout) {
    const auto resource = field<int32_t>(layout+316);
    if (!resource || !field<int32_t>(resource+64)) return; // Writer ignores E_FAIL.
    const auto data = pointer<retdec_mcd_data>(field<int32_t>(resource+64));
    field<int32_t>(layout+284) = field<int32_t>(layout+280);
    field<int32_t>(layout+300) = field<int32_t>(layout+296);
    const auto count = record_count(layout);
    auto records = pointer<MapRecord>(field<int32_t>(layout+264));
    if (count) std::sort(records, records+count, [](const auto& a, const auto& b) {
        return a[1] < b[1] || (a[1] == b[1] && a[2] < b[2]);
    });
    std::vector<const retdec_mcd_chip*> chips;
    for (uint32_t i=0; i<data->chip_count; ++i) chips.push_back(&data->chips[i]);
    std::sort(chips.begin(), chips.end(), [](auto a, auto b) { return a->chip_id < b->chip_id; });
    auto max_id = field<int32_t>(layout+452);
    if (max_id < 0) for (auto chip : chips) max_id = std::max(max_id, static_cast<int32_t>(chip->chip_id));
    if (max_id > 0x100000) throw std::bad_alloc();
    std::vector<int32_t> indices(static_cast<size_t>(max_id+1), -1);
    std::vector<std::array<unsigned char,48>> cache(chips.size());
    for (size_t i=0; i<chips.size(); ++i) {
        if (chips[i]->chip_id >= indices.size()) throw std::bad_alloc();
        indices[chips[i]->chip_id] = static_cast<int32_t>(i);
        std::memcpy(cache[i].data(), chips[i]->bytes, 48);
    }
    replace_buffer(layout+404, cache.data(), cache.size()*48);
    replace_buffer(layout+436, indices.data(), indices.size()*4);
    field<int32_t>(layout+452) = max_id;
    field<int32_t>(layout+240) = field<int32_t>(layout+244) = INT_MIN;
    if (!count) {
        for (auto offset : {248,252,256,260}) field<int32_t>(layout+offset) = 0;
        return;
    }
    field<int32_t>(layout+248) = records[0][1];
    field<int32_t>(layout+252) = records[0][2];
    field<int32_t>(layout+256) = records[count-1][1];
    // Original 4359FF initializes bottom from the last X, not Y.
    field<int32_t>(layout+260) = records[count-1][1];
    for (uint32_t i=0; i<count; ++i) {
        const auto& record = records[i];
        auto chip = retdec_mcd_find_chip(data, static_cast<uint32_t>(record[0]));
        if (chip) {
            const int32_t width = retdec_mcd_i16(chip->bytes+12), height = retdec_mcd_i16(chip->bytes+14);
            field<int32_t>(layout+240) = std::max(field<int32_t>(layout+240), width);
            field<int32_t>(layout+244) = std::max(field<int32_t>(layout+244), height);
            const auto right = static_cast<int32_t>(static_cast<uint32_t>(record[1])+width);
            const auto bottom = static_cast<int32_t>(static_cast<uint32_t>(record[2])+height);
            field<int32_t>(layout+256) = std::max(field<int32_t>(layout+256), right);
            field<int32_t>(layout+260) = std::max(field<int32_t>(layout+260), bottom);
        }
        field<int32_t>(layout+248) = std::min(field<int32_t>(layout+248), record[1]);
        field<int32_t>(layout+252) = std::min(field<int32_t>(layout+252), record[2]);
    }
}
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

extern "C" int32_t __fastcall kinoko_method_read_layout_properties(
    int32_t layout, void*, int32_t holder, int32_t version) {
    if (!layout || !holder || version != 1) return 0;
    try {
        if (!read(layout, field<int32_t>(holder), layout_schema, false)) return 0;
        // 42C084 invalidates transforms after reading; this is not a texture.
        field<uint8_t>(layout+312) = 1;
        return 1;
    } catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_write_layout_properties(
    int32_t layout, void*, int32_t writer) {
    if (!layout || !writer) return 0;
    try { return write(layout, writer, layout_schema); }
    catch (...) { return 0; }
}

extern "C" int32_t function_43c860_this(int32_t layout, int32_t holder, int32_t version) {
    if (!layout || !holder || version != 1) return 0;
    try { return read(layout, field<int32_t>(holder), layout3d_schema, false); }
    catch (...) { return 0; }
}

extern "C" int32_t __fastcall kinoko_method_read_map_layout(
    int32_t layout, void*, int32_t holder, int32_t version) {
    if (!layout || !holder || version != 1) return 0;
    try {
        const auto reader = field<int32_t>(holder);
        if (!read(layout, reader, map_schema, false)) return 0;
        uint32_t count=0, size=0;
        if (!transfer(reader, count) || !transfer(reader, size) || count > 0x10000) return 0;
        const auto old_count = record_count(layout);
        if (count > 0x10000-old_count) return 0;
        std::vector<MapRecord> records;
        const auto old = pointer<MapRecord>(field<int32_t>(layout+264));
        if (old_count) records.assign(old, old+old_count);
        for (uint32_t i=0; i<count; ++i) {
            MapRecord record{};
            // Original reads min(size,32), without skipping any excess bytes.
            if (!transfer(reader, record.data(), std::min(size,32u))) return 0;
            record[5] = i;
            reinterpret_cast<unsigned char*>(record.data())[24] = 1;
            record[7] = 0x3f800000;
            records.push_back(record);
        }
        if (count) replace_buffer(layout+264, records.data(), records.size()*32);
        return 1;
    } catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_write_map_layout(
    int32_t layout, void*, int32_t writer) {
    if (!layout || !writer) return 0;
    try {
        prepare_map(layout);
        if (!write(layout, writer, map_schema)) return 0;
        auto count = record_count(layout);
        uint32_t size = 12;
        if (!transfer(writer, count) || !transfer(writer, size)) return 0;
        auto records = pointer<MapRecord>(field<int32_t>(layout+264));
        for (uint32_t i=0; i<count; ++i)
            if (!transfer(writer, records[i].data(), size)) return 0;
        return 1;
    } catch (...) { return 0; }
}
