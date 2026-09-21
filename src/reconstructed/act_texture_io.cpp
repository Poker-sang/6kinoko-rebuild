#include "kinoko/native_buffer.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/boost_hash.h"
#include "kinoko/string_layout.h"
#include <algorithm>
#include <array>
#include <climits>
#include <vector>
#include <cstdint>
#include <map>
#include <string>

extern "C" unsigned char g673;
#include "kinoko/string_layout.h"

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
// 425350: CActTimeLine has two integers followed by a vector of integer pairs.
Schema timeline_schema{{"beginTime", {0,0,4}}, {"timeLength", {0,0,8}}};
// 41E790, including the two byte-sized booleans and previous-position fields.
Schema layer_schema{
    {"resourceID",{0,0,96}}, {"layerID",{0,0,104}}, {"parentID",{0,0,108}},
    {"stName",{3,3,112}}, {"visible",{2,2,140}}, {"debugOnly",{2,2,141}},
    {"dst_x",{1,1,144}}, {"dst_y",{1,1,148}}, {"dst_z",{1,1,152}},
    {"ox",{1,1,156}}, {"oy",{1,1,160}}, {"oz",{1,1,164}},
    {"prev_x",{1,1,168}}, {"prev_y",{1,1,172}}, {"prev_z",{1,1,176}}
};
Schema key_schema{{"scriptFunction",{3,3,8}}};
// 427750: marginLeft/Top/Right/Bottom are +72/+76/+80/+84.
Schema act_schema{
    {"resolutionMs",{0,0,4}}, {"screenWidth",{0,0,8}}, {"screenHeight",{0,0,12}},
    {"stName",{3,3,16}}, {"offsetX",{1,1,88}}, {"offsetY",{1,1,92}},
    {"marginLeft",{0,0,72}}, {"marginTop",{0,0,76}},
    {"marginRight",{0,0,80}}, {"marginBottom",{0,0,84}}, {"visible",{2,2,96}}
};
// 43F770 registers alignment with the bool template 443CC0 AND offset 128,
// aliasing addEdge (43F97D), despite the runtime integer living at 132.
Schema string_layout_schema{
    {"stText",{3,3,4}}, {"stBackQueue",{3,3,32}}, {"stFontFaceName",{3,3,60}},
    {"fontHeight",{0,0,88}}, {"fontWeight",{0,0,92}},
    {"colorR",{0,0,96}}, {"colorG",{0,0,100}}, {"colorB",{0,0,104}},
    {"baseR",{0,0,108}}, {"baseG",{0,0,112}}, {"baseB",{0,0,116}},
    {"charactorSpace",{0,0,120}}, {"lineSpace",{0,0,124}},
    {"addEdge",{2,2,128}}, {"alignment",{2,2,128}},
    {"scaleX",{1,1,136}}, {"scaleY",{1,1,140}}, {"wordBreakWidth",{0,0,144}},
    {"alpha",{1,1,152}}, {"blend",{0,0,156}}
};
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
    kinoko_native_buffer_replace(slot,bytes,static_cast<uint32_t>(size));
}
// Original 435860/435B20, adapted to the source-owned MCD rather than an old
// MSVC tree. The ABI caches still contain flat records and an ID/index vector.
void rebuild_map_cache(int32_t layout) {
    const auto resource = field<int32_t>(layout+316);
    if (!resource || !field<int32_t>(resource+64)) return; // Writer ignores E_FAIL.
    const auto data = pointer<retdec_mcd_data>(field<int32_t>(resource+64));
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
}
void prepare_map(int32_t layout) {
    const auto resource = field<int32_t>(layout+316);
    if (!resource || !field<int32_t>(resource+64)) return;
    const auto data = pointer<retdec_mcd_data>(field<int32_t>(resource+64));
    field<int32_t>(layout+284) = field<int32_t>(layout+280);
    field<int32_t>(layout+300) = field<int32_t>(layout+296);
    const auto count = record_count(layout);
    auto records = pointer<MapRecord>(field<int32_t>(layout+264));
    if (count) std::sort(records, records+count, [](const auto& a, const auto& b) {
        return a[1] < b[1] || (a[1] == b[1] && a[2] < b[2]);
    });
    rebuild_map_cache(layout);
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
            uint8_t value;
            if (!transfer(reader, value)) return false;
            if (assign) field<uint8_t>(resource+property.offset) = value;
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
        } else if (!transfer(writer, pointer<void>(resource+property.offset),
                             property.type == 2 ? 1 : 4)) return false;
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

extern "C" int32_t __fastcall kinoko_method_map_set_layer(
    int32_t layout, void*, int32_t layer) {
    constexpr int32_t fail = static_cast<int32_t>(0x80004005u);
    if (!layout || !layer) return fail;
    try {
        const auto resource = field<int32_t>(layer+100);
        field<int32_t>(layout+316) = 0;
        if (!resource || field<uint8_t>(layout+460)) {
            // Original one-shot suppression is distinct from an invalid type.
            field<uint8_t>(layout+460) = 0;
        } else {
            if (field<int32_t>(resource) != address(kinoko_act_host_symbols()->chip_resource_vtable))
                return fail;
            field<int32_t>(layout+316) = resource;
            // Source MCD loading already loads each texture once. The original
            // sorted/unique texture preload therefore needs no second acquire.
            rebuild_map_cache(layout);
        }
        field<int32_t>(layout+312) = layer;
        field<int32_t>(layout+284) = field<int32_t>(layout+280);
        if (!field<int32_t>(layout+316)) return 0;
        const auto count = record_count(layout);
        const auto data = pointer<retdec_mcd_data>(field<int32_t>(resource+64));
        if (!data && count) return fail;
        auto records = pointer<MapRecord>(field<int32_t>(layout+264));
        std::vector<int32_t> chip_refs, texture_refs;
        // 434380 clears only the chip-reference vector; texture refs append.
        auto begin = field<uint32_t>(layout+296), end = field<uint32_t>(layout+300);
        if (end < begin || (end-begin)%4 || (end-begin)/4 > 0x10000 || (!begin && end)) return fail;
        if (end != begin) texture_refs.assign(pointer<int32_t>(begin), pointer<int32_t>(end));
        for (uint32_t i=0; i<count; ++i) {
            auto chip = retdec_mcd_find_chip(data, static_cast<uint32_t>(records[i][0]));
            auto texture = chip ? retdec_mcd_find_texture(data, kinoko::legacy::load<uint32_t>(chip->bytes+4)) : nullptr;
            chip_refs.push_back(chip ? address(chip->bytes) : 0);
            texture_refs.push_back(address(texture));
        }
        replace_buffer(layout+280, chip_refs.data(), chip_refs.size()*4);
        replace_buffer(layout+296, texture_refs.data(), texture_refs.size()*4);
        return 0;
    } catch (...) { return fail; }
}


namespace {
using TimelinePair = std::array<int32_t,2>;
std::vector<TimelinePair> timeline_pairs(int32_t timeline) {
    const auto begin=field<uint32_t>(timeline+12),end=field<uint32_t>(timeline+16);
    if (end<begin || (end-begin)%8 || (!begin && end) || (end-begin)/8>0x10000)
        throw std::bad_alloc();
    if (begin==end) return {};
    return {pointer<TimelinePair>(begin),pointer<TimelinePair>(end)};
}
int32_t __fastcall read_timeline(int32_t timeline,void*,int32_t holder,int32_t version) {
    if (!timeline || !holder || version!=1) return 0;
    try {
        const auto reader=field<int32_t>(holder);
        if (!read(timeline,reader,timeline_schema,false)) return 0;
        uint32_t count=0;
        if (!transfer(reader,count) || count>0x10000) return 0;
        auto pairs=timeline_pairs(timeline);
        if (count>0x10000-pairs.size()) return 0;
        for (uint32_t i=0;i<count;++i) {
            TimelinePair pair{};
            if (!transfer(reader,pair[0]) || !transfer(reader,pair[1])) return 0;
            pairs.push_back(pair);
        }
        // 425420 appends; preserve existing records on a repeated read. Unlike
        // the original partial append, a truncated batch leaves the vector intact.
        if (count) replace_buffer(timeline+12,pairs.data(),pairs.size()*sizeof(TimelinePair));
        return 1;
    } catch (...) { return 0; }
}
int32_t __fastcall write_timeline(int32_t timeline,void*,int32_t writer) {
    if (!timeline || !writer) return 0;
    try {
        const auto pairs=timeline_pairs(timeline);
        if (!write(timeline,writer,timeline_schema)) return 0;
        auto count=static_cast<uint32_t>(pairs.size());
        if (!transfer(writer,count)) return 0;
        for (auto pair:pairs)
            if (!transfer(writer,pair[0]) || !transfer(writer,pair[1])) return 0;
        return 1;
    } catch (...) { return 0; }
}
int32_t __fastcall query_timeline(int32_t timeline,void*,int32_t type,int32_t output) {
    if (!output) return 0;
    const bool match=type && std::strcmp(pointer<const char>(type+9),"?AVCActTimeLine@@")==0;
    field<int32_t>(output)=match?timeline:0;
    return match;
}
void clear_timeline(int32_t timeline) {
    kinoko_native_buffer_destroy(timeline+12);
    std::memset(pointer<void>(timeline+12),0,12);
}
int32_t __fastcall delete_timeline(int32_t timeline,void*,int32_t flags) {
    if (!timeline) return 0;
    if (flags&2) {
        const auto count=field<uint32_t>(timeline-4);
        for (auto i=count;i>0;--i) clear_timeline(timeline+(i-1)*28);
        if (flags&1) std::free(pointer<void>(timeline-4));
        return timeline-4;
    }
    clear_timeline(timeline);
    if (flags&1) std::free(pointer<void>(timeline));
    return timeline;
}
int32_t __fastcall destroy_timeline(int32_t timeline,void*) {
    return delete_timeline(timeline,nullptr,1);
}
int32_t __fastcall clone_timeline(int32_t timeline,void*) {
    if (!timeline) return 0;
    try {
        const auto pairs=timeline_pairs(timeline);
        kinoko::legacy::Allocation<unsigned char> owner(pointer<unsigned char>(kinoko_act_new_timeline()));
        if (!owner) return 0;
        const auto result=address(owner.get());
        field<int32_t>(result+4)=field<int32_t>(timeline+4);
        field<int32_t>(result+8)=field<int32_t>(timeline+8);
        replace_buffer(result+12,pairs.data(),pairs.size()*sizeof(TimelinePair));
        return address(owner.release());
    } catch (...) { return 0; }
}
const void* const timeline_methods[]={
    reinterpret_cast<const void*>(write_timeline),reinterpret_cast<const void*>(read_timeline),
    reinterpret_cast<const void*>(query_timeline),reinterpret_cast<const void*>(destroy_timeline),
    reinterpret_cast<const void*>(delete_timeline),reinterpret_cast<const void*>(clone_timeline)
};
}
extern "C" const void* kinoko_act_timeline_vtable(void) { return timeline_methods; }
extern "C" int32_t kinoko_act_new_timeline(void) {
    const auto result=address(std::calloc(1,28));
    if (result) field<int32_t>(result)=address(timeline_methods);
    return result;
}
extern "C" int32_t kinoko_act_load_timeline(int32_t timeline,int32_t reader,int32_t version) {
    return read_timeline(timeline,nullptr,address(&reader),version);
}


namespace {
uint32_t type_hash(const char* name,size_t length) {
    return static_cast<uint32_t>(kinoko_boost_hash_range(address(name),address(name+length)));
}
uint32_t type_hash(const char* name) { return type_hash(name,std::strlen(name)); }
struct TypeName {
    unsigned char bytes[28]{};
    TypeName() { field<uint32_t>(address(bytes)+20)=15; }
    ~TypeName() {
        const kinoko::legacy::StringView text(bytes);
        if (text.is_heap()) std::free(text.data());
    }
};
bool object_hash(int32_t layout,uint32_t& hash,int32_t type_slot) {
    const auto table=field<int32_t>(layout);
    const auto* name=kinoko_act_serialized_type_name(layout);
    if (name) { hash=type_hash(name); return true; }
    // Original 426740 obtains the remaining registered types through virtual
    // GetType/GetName. Keep this boundary until their registries are migrated;
    // do not confuse modern compiler RTTI spelling with the original raw name.
    if (!table) return false;
    const auto binder=retdec_call_thiscall0_result(pointer<void>(layout),field<void*>(table+type_slot));
    if (!binder || !field<int32_t>(binder)) return false;
    TypeName result;
    retdec_call_thiscall1_result(pointer<void>(binder),field<void*>(field<int32_t>(binder)+4),address(result.bytes));
    const kinoko::legacy::StringView text(result.bytes);
    hash=type_hash(text.data(),text.length());
    return true;
}
bool write_layer_list(int32_t layer,int32_t offset,int32_t writer,const char* type) {
    auto count=field<uint32_t>(layer+offset+4);
    const auto head=field<int32_t>(layer+offset);
    if (count>0x10000 || !head || !transfer(writer,count)) return false;
    auto node=field<int32_t>(head);
    auto hash=type_hash(type);
    for (uint32_t i=0;i<count;++i) {
        if (!node || node==head || !transfer(writer,hash)) return false;
        const auto object=field<int32_t>(node+8);
        // Original 41F990 ignores each element writer's return value, but
        // propagates failure to emit the list count/type and final script.
        if (object) retdec_call_thiscall1_result(pointer<void>(object),
            field<void*>(field<int32_t>(object)),writer);
        node=field<int32_t>(node);
    }
    return node==head;
}
}
extern "C" int32_t __fastcall kinoko_method_write_act_layer(int32_t layer,void*,int32_t writer) {
    if (!layer || !writer) return 0;
    try {
        if (!write(layer,writer,layer_schema) ||
            !write_layer_list(layer,180,writer,".?AVCActKey@@") ||
            !write_layer_list(layer,192,writer,".?AVCActTimeLine@@")) return 0;
        const auto script=layer+204;
        return (retdec_call_thiscall1_result(pointer<void>(script),
            field<void*>(field<int32_t>(script)),writer)&0xff)!=0;
    } catch (...) { return 0; }
}
extern "C" int32_t kinoko_act_read_layer_properties(int32_t layer,int32_t reader) {
    if (!layer || !reader) return 0;
    try { return read(layer,reader,layer_schema,false); }
    catch (...) { return 0; }
}
extern "C" int32_t kinoko_act_read_key_properties(int32_t key,int32_t reader) {
    if (!key || !reader) return 0;
    try { return read(key,reader,key_schema,false); }
    catch (...) { return 0; }
}
extern "C" int32_t kinoko_act_read_map_properties(int32_t layout,int32_t reader) {
    if (!layout || !reader) return 0;
    try { return read(layout,reader,map_schema,false); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_read_act_layer(int32_t layer,void*,int32_t holder,int32_t version) {
    return layer && holder && version==1 ? retdec_act_load_layer(layer,field<int32_t>(holder),version) : 0;
}
extern "C" int32_t __fastcall kinoko_method_read_act_key(int32_t key,void*,int32_t holder,int32_t version) {
    return key && holder && version==1 ? retdec_act_load_key(key,field<int32_t>(holder),version) : 0;
}
extern "C" int32_t __fastcall kinoko_method_write_act_key(int32_t key,void*,int32_t writer) {
    if (!key || !writer) return 0;
    try {
        if (!write(key,writer,key_schema)) return 0;
        const auto layout=field<int32_t>(key+4);
        uint8_t present=layout!=0;
        if (!transfer(writer,present)) return 0;
        if (!layout) return 1;
        uint32_t hash=0;
        if (!object_hash(layout,hash,16) || !transfer(writer,hash)) return 0;
        return (retdec_call_thiscall1_result(pointer<void>(layout),
            field<void*>(field<int32_t>(layout)),writer)&0xff)!=0;
    } catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_write_string_layout(int32_t layout,void*,int32_t writer) {
    if (!layout || !writer) return 0;
    try { return write(layout,writer,string_layout_schema); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_read_string_layout(int32_t layout,void*,int32_t holder,int32_t version) {
    if(!layout || !holder || version!=1) return 0;
    try {
        if(!read(layout,field<int32_t>(holder),string_layout_schema,false)) return 0;
        // 43FB75..43FBAF moves stText+stBackQueue into the pending queue,
        // then clears stText. This preserves embedded NULs (unlike rebuild).
        kinoko::legacy::StringView text(pointer<void>(layout+4)),queue(pointer<void>(layout+32));
        std::string pending(text.data(),text.length());
        pending.append(queue.data(),queue.length());
        queue.assign(pending.data(),static_cast<uint32_t>(pending.size()));
        text.assign("",0);
        return 1;
    } catch(...) { return 0; }
}

extern "C" int32_t kinoko_act_read_properties(int32_t act,int32_t reader) {
    if (!act || !reader) return 0;
    try { return read(act,reader,act_schema,false); }
    catch (...) { return 0; }
}
extern "C" int32_t __fastcall kinoko_method_read_act(int32_t act,void*,int32_t holder,int32_t version) {
    return act && holder && version==1 ? retdec_act_load(act,field<int32_t>(holder),version) : 0;
}
namespace {
std::vector<int32_t> object_vector(int32_t slot) {
    const auto begin=field<uint32_t>(slot),end=field<uint32_t>(slot+4);
    if (end<begin || (end-begin)%4 || (!begin && end) || (end-begin)/4>0x10000)
        throw std::bad_alloc();
    if (begin==end) return {};
    return {pointer<int32_t>(begin),pointer<int32_t>(end)};
}
}
extern "C" int32_t __fastcall kinoko_method_write_act(int32_t act,void*,int32_t writer) {
    if (!act || !writer) return 0;
    try {
        if (!write(act,writer,act_schema)) return 0;
        const auto script=act+100;
        if (!(retdec_call_thiscall1_result(pointer<void>(script),
            field<void*>(field<int32_t>(script)),writer)&0xff)) return 0;
        auto layers=object_vector(act+208);
        // 4287D9/428822 omit only debugOnly==1 when g673==1. Values other
        // than one are deliberately not treated as true by this filter.
        if (g673==1) layers.erase(std::remove_if(layers.begin(),layers.end(),[](auto layer) {
            return layer && field<uint8_t>(layer+141)==1;
        }),layers.end());
        auto count=static_cast<uint32_t>(layers.size());
        if (!transfer(writer,count)) return 0;
        auto hash=type_hash(".?AVCActLayer@@");
        for (auto layer:layers) {
            if (!transfer(writer,hash)) return 0;
            if (layer) retdec_call_thiscall1_result(pointer<void>(layer),
                field<void*>(field<int32_t>(layer)),writer);
        }
        const auto resources=object_vector(act+224);
        count=static_cast<uint32_t>(resources.size());
        if (!transfer(writer,count)) return 0;
        for (auto resource:resources) {
            if (!resource || !object_hash(resource,hash,20) || !transfer(writer,hash)) return 0;
            retdec_call_thiscall1_result(pointer<void>(resource),
                field<void*>(field<int32_t>(resource)),writer);
        }
        return 1;
    } catch (...) { return 0; }
}

extern "C" {
int32_t function_4072d0(int32_t receiver, int32_t bytes, int32_t size);
int32_t function_410b90(int32_t receiver, int32_t bytes, int32_t size);
}
// These recovered read bodies already take explicit receivers. A virtual
// caller still passes this in ECX; never put the cdecl body directly in a slot.
extern "C" int32_t __fastcall kinoko_method_read_file(int32_t receiver, void*, int32_t bytes, int32_t size) {
    return function_4072d0(receiver,bytes,size);
}
extern "C" int32_t __fastcall kinoko_method_read_package(int32_t receiver, void*, int32_t bytes, int32_t size) {
    return function_410b90(receiver,bytes,size);
}

