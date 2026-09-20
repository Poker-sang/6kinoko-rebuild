// Property parsing and original byte-offset mappings for ACT records.
// Kept independent from loading/rendering so malformed-input contracts can
// exercise the real parser without linking unrelated game host services.
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using kinoko::legacy::pointer;
using kinoko::legacy::field;

void retdec_act_free_properties(struct retdec_act_property *properties,
                                        uint32_t count)
{
    uint32_t index;

    if (properties == nullptr)
        return;
    for (index = 0; index < count; ++index) {
        std::free(properties[index].name);
        std::free(properties[index].string);
    }
    std::free(properties);
}

int32_t retdec_act_read_u8(int32_t reader_ptr, uint8_t *value)
{
    return retdec_reader_read_exact(reader_ptr, value, 1);
}

int32_t retdec_act_read_u32(int32_t reader_ptr, uint32_t *value)
{
    return retdec_reader_read_exact(reader_ptr, value, sizeof(*value));
}

int32_t retdec_act_read_properties(
    int32_t reader_ptr, struct retdec_act_property **properties_out,
    uint32_t *count_out)
{
    static LONG trace_count;
    uint8_t has_properties;
    uint32_t count;
    uint32_t index;
    struct retdec_act_property *properties;

    if (properties_out == nullptr || count_out == nullptr)
        return 0;
    *properties_out = nullptr;
    *count_out = 0;
    if (!retdec_act_read_u8(reader_ptr, &has_properties)) {
        retdec_trace("act:property-flag-read-failed");
        return 0;
    }
    if (InterlockedIncrement(&trace_count) <= 32) {
        retdec_trace_i32("act:property-flag", (int32_t)has_properties);
        if (g765 != 0)
            retdec_trace_i32("act:property-position",
                             field<int32_t>(reader_ptr + 20));
    }
    if (has_properties == 0)
        return 1;
    if (!retdec_act_read_u32(reader_ptr, &count) || count > 1024u) {
        retdec_trace("act:property-count-read-failed");
        return 0;
    }
    if (trace_count <= 32)
        retdec_trace_i32("act:property-count", (int32_t)count);
    if (count == 0)
        return 1;

    properties = (struct retdec_act_property *)std::calloc(
        (size_t)count, sizeof(*properties));
    if (properties == nullptr)
        return 0;
    for (index = 0; index < count; ++index) {
        uint32_t name_length;

        if (!retdec_act_read_u32(reader_ptr, &name_length) ||
            name_length > 4096u) {
            retdec_act_free_properties(properties, count);
            return 0;
        }
        properties[index].name = (char *)std::malloc((size_t)name_length + 1u);
        if (properties[index].name == nullptr) {
            retdec_act_free_properties(properties, count);
            return 0;
        }
        if (name_length != 0 && !retdec_reader_read_exact(
                reader_ptr, properties[index].name, name_length)) {
            retdec_act_free_properties(properties, count);
            return 0;
        }
        properties[index].name[name_length] = 0;
        if (!retdec_act_read_u32(reader_ptr, &properties[index].type) ||
            properties[index].type > 3u) {
            retdec_act_free_properties(properties, count);
            return 0;
        }
    }

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];

        switch (property->type) {
        case 0:
            if (!retdec_act_read_u32(reader_ptr,
                                     (uint32_t *)&property->integer)) {
                retdec_act_free_properties(properties, count);
                return 0;
            }
            break;
        case 1:
            if (!retdec_reader_read_exact(reader_ptr, &property->real,
                                          sizeof(property->real))) {
                retdec_act_free_properties(properties, count);
                return 0;
            }
            break;
        case 2: {
            uint8_t value;
            if (!retdec_act_read_u8(reader_ptr, &value)) {
                retdec_act_free_properties(properties, count);
                return 0;
            }
            property->integer = value != 0;
            break;
        }
        case 3: {
            uint32_t string_length;
            if (!retdec_act_read_u32(reader_ptr, &string_length) ||
                string_length > 0x100000u) {
                retdec_act_free_properties(properties, count);
                return 0;
            }
            property->string = (char *)std::malloc((size_t)string_length + 1u);
            if (property->string == nullptr) {
                retdec_act_free_properties(properties, count);
                return 0;
            }
            if (string_length != 0 && !retdec_reader_read_exact(
                    reader_ptr, property->string, string_length)) {
                retdec_act_free_properties(properties, count);
                return 0;
            }
            property->string[string_length] = 0;
            property->string_length = string_length;
            break;
        }
        default:
            retdec_act_free_properties(properties, count);
            return 0;
        }
    }
    *properties_out = properties;
    *count_out = count;
    return 1;
}

int32_t retdec_act_property_integer(
    const struct retdec_act_property *property)
{
    return property->type == 1 ? (int32_t)property->real : property->integer;
}

float retdec_act_property_float(
    const struct retdec_act_property *property)
{
    return property->type == 1 ? property->real : (float)property->integer;
}

void retdec_act_assign_string(int32_t object_ptr, uint32_t offset,
                                     const struct retdec_act_property *property)
{
    if (property->type == 3)
        retdec_string_assign_n(
            pointer<int32_t>(object_ptr + (int32_t)offset),
            property->string != nullptr ? property->string : "",
            property->string_length);
}

void retdec_act_apply_cact(int32_t object_ptr,
                                  struct retdec_act_property *properties,
                                  uint32_t count)
{
    uint32_t index;

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];
        if (std::strcmp(property->name, "resolutionMs") == 0)
            field<int32_t>(object_ptr + 4) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "screenWidth") == 0)
            field<int32_t>(object_ptr + 8) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "screenHeight") == 0)
            field<int32_t>(object_ptr + 12) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "stName") == 0)
            retdec_act_assign_string(object_ptr, 16, property);
        else if (std::strcmp(property->name, "offsetX") == 0)
            field<float>(object_ptr + 88) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "offsetY") == 0)
            field<float>(object_ptr + 92) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "marginBottom") == 0)
            field<int32_t>(object_ptr + 84) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "marginLeft") == 0)
            field<int32_t>(object_ptr + 72) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "marginRight") == 0)
            field<int32_t>(object_ptr + 80) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "marginTop") == 0)
            field<int32_t>(object_ptr + 76) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "visible") == 0)
            field<uint8_t>(object_ptr + 96) =
                (uint8_t)(retdec_act_property_integer(property) != 0);
    }
}

void retdec_act_apply_script(int32_t object_ptr,
                                    struct retdec_act_property *properties,
                                    uint32_t count)
{
    uint32_t index;

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];
        if (std::strcmp(property->name, "filePath") == 0)
            retdec_act_assign_string(object_ptr, 64, property);
        else if (std::strcmp(property->name, "compiled") == 0)
            field<uint8_t>(object_ptr + 101) =
                (uint8_t)(retdec_act_property_integer(property) != 0);
    }
}

void retdec_act_apply_layer(int32_t object_ptr,
                                   struct retdec_act_property *properties,
                                   uint32_t count)
{
    uint32_t index;

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];
        if (std::strcmp(property->name, "resourceID") == 0)
            field<int32_t>(object_ptr + 0x60) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "layerID") == 0)
            field<int32_t>(object_ptr + 0x68) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "stName") == 0)
            retdec_act_assign_string(object_ptr, 0x70, property);
        else if (std::strcmp(property->name, "visible") == 0)
            field<uint8_t>(object_ptr + 0x8c) =
                (uint8_t)(retdec_act_property_integer(property) != 0);
        else if (std::strcmp(property->name, "debugOnly") == 0)
            field<uint8_t>(object_ptr + 0x8d) =
                (uint8_t)(retdec_act_property_integer(property) != 0);
        else if (std::strcmp(property->name, "dst_x") == 0)
            field<float>(object_ptr + 0x90) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "dst_y") == 0)
            field<float>(object_ptr + 0x94) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "dst_z") == 0)
            field<float>(object_ptr + 0x98) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "ox") == 0)
            field<float>(object_ptr + 0x9c) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "oy") == 0)
            field<float>(object_ptr + 0xa0) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "oz") == 0)
            field<float>(object_ptr + 0xa4) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "parentID") == 0)
            field<int32_t>(object_ptr + 0x6c) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "prev_x") == 0)
            field<float>(object_ptr + 0xa8) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "prev_y") == 0)
            field<float>(object_ptr + 0xac) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "prev_z") == 0)
            field<float>(object_ptr + 0xb0) =
                retdec_act_property_float(property);
    }
}

void retdec_act_apply_layout(int32_t object_ptr,
                                    struct retdec_act_property *properties,
                                    uint32_t count)
{
    uint32_t index;

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];
        if (std::strcmp(property->name, "alpha") == 0)
            field<float>(object_ptr + 0x11c) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "blend") == 0)
            field<int32_t>(object_ptr + 0x120) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "colorB") == 0)
            field<int32_t>(object_ptr + 0x12c) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "colorG") == 0)
            field<int32_t>(object_ptr + 0x128) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "colorR") == 0)
            field<int32_t>(object_ptr + 0x124) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "cor_x") == 0)
            field<float>(object_ptr + 0xf8) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "cor_y") == 0)
            field<float>(object_ptr + 0xfc) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "cor_z") == 0)
            field<float>(object_ptr + 0x100) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "cos_x") == 0)
            field<float>(object_ptr + 0x110) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "cos_y") == 0)
            field<float>(object_ptr + 0x114) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "cos_z") == 0)
            field<float>(object_ptr + 0x118) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "roll.x") == 0)
            field<float>(object_ptr + 0xec) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "roll.y") == 0)
            field<float>(object_ptr + 0xf0) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "roll.z") == 0)
            field<float>(object_ptr + 0xf4) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "scale.x") == 0)
            field<float>(object_ptr + 0x104) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "scale.y") == 0)
            field<float>(object_ptr + 0x108) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "scale.z") == 0)
            field<float>(object_ptr + 0x10c) =
                retdec_act_property_float(property);
    }
}

void retdec_act_apply_map_layout(
    int32_t object_ptr, struct retdec_act_property *properties,
    uint32_t count)
{
    uint32_t index;

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];
        if (std::strcmp(property->name, "layerType") == 0)
            field<int32_t>(object_ptr + 236) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "maxChipWidth") == 0)
            field<int32_t>(object_ptr + 240) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "maxChipHeight") == 0)
            field<int32_t>(object_ptr + 244) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "mapChipLeft") == 0)
            field<int32_t>(object_ptr + 248) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "mapChipRight") == 0)
            field<int32_t>(object_ptr + 256) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "mapChipTop") == 0)
            field<int32_t>(object_ptr + 252) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "mapChipBottom") == 0)
            field<int32_t>(object_ptr + 260) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "alpha") == 0)
            field<float>(object_ptr + 320) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "scale") == 0)
            field<float>(object_ptr + 324) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "blend") == 0)
            field<int32_t>(object_ptr + 328) =
                retdec_act_property_integer(property);
    }
}

void retdec_act_apply_resource(
    int32_t object_ptr, struct retdec_act_property *properties,
    uint32_t count)
{
    uint32_t index;

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];
        if (std::strcmp(property->name, "resourceID") == 0)
            field<int32_t>(object_ptr + 4) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "stName") == 0)
            retdec_act_assign_string(object_ptr, 8, property);
        else if (std::strcmp(property->name, "stTextureName") == 0)
            retdec_act_assign_string(object_ptr, 40, property);
        else if (std::strcmp(property->name, "image_width") == 0)
            field<int32_t>(object_ptr + 72) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "image_height") == 0)
            field<int32_t>(object_ptr + 76) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "src_x") == 0)
            field<float>(object_ptr + 80) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "src_y") == 0)
            field<float>(object_ptr + 84) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "src_width") == 0)
            field<float>(object_ptr + 88) =
                retdec_act_property_float(property);
        else if (std::strcmp(property->name, "src_height") == 0)
            field<float>(object_ptr + 92) =
                retdec_act_property_float(property);
    }
}

void retdec_act_apply_chip_resource(
    int32_t object_ptr, struct retdec_act_property *properties,
    uint32_t count)
{
    uint32_t index;

    for (index = 0; index < count; ++index) {
        struct retdec_act_property *property = &properties[index];
        if (std::strcmp(property->name, "resourceID") == 0)
            field<int32_t>(object_ptr + 4) =
                retdec_act_property_integer(property);
        else if (std::strcmp(property->name, "stName") == 0)
            retdec_act_assign_string(object_ptr, 8, property);
        else if (std::strcmp(property->name, "stChipFile") == 0)
            retdec_act_assign_string(object_ptr, 36, property);
    }
}
