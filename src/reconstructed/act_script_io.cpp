#include "kinoko/act_runtime.h"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/file_io_layout.h"
#include "kinoko/act_host.h"
#include "kinoko/act_script_payload.hpp"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/upstream_bindings.hpp"
#include <sqstdaux.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::legacy::field;
// 415E20 registers these native members. 4175F0 updates the read schema;
// omitted properties keep their native values, unknown/mismatched ones consume
// their serialized values without assigning them. std::map supplies key order.
std::map<std::string, uint32_t> script_schema{{"compiled",2}, {"filePath",3}};
// 4175F0's stream has a vtable at +0, transfer at slot +12 and seek at +20.
bool transfer(KinokoArchiveReader* stream, void* bytes, uint32_t size) {
    return stream && (stream->methods->transfer(stream,bytes,size)&0xff)!=0;
}
template<class T> bool transfer(KinokoArchiveReader* stream, T& value) { return transfer(stream, &value, sizeof(value)); }
int32_t seek(KinokoArchiveReader* writer, int32_t offset, int32_t origin) {
    return static_cast<int32_t>(writer->methods->seek(writer,offset,origin));
}
bool read_string(KinokoArchiveReader* reader, std::string& value, uint32_t maximum) {
    uint32_t length = 0;
    if (!transfer(reader, length) || length > maximum) return false;
    value.resize(length);
    return !length || transfer(reader, value.data(), length);
}
bool write_string(KinokoArchiveReader* writer, const char* text, uint32_t length) {
    return transfer(writer, length) && (!length || transfer(writer, const_cast<char*>(text), length));
}
SQInteger write_bytecode(SQUserPointer context, SQUserPointer bytes, SQInteger size) {
    return transfer(static_cast<KinokoArchiveReader*>(context), bytes, size) ? size : 0;
}
void quiet_print(HSQUIRRELVM, const SQChar*, ...) {}
}

extern "C" int32_t kinoko_act_read_script_properties(void* script, KinokoArchiveReader* reader) {
    if (!script || !reader) return 0;
    try {
        uint8_t has_schema = 1;
        if (!transfer(reader, has_schema)) return 0;
        if (has_schema) {
            uint32_t count = 0;
            if (!transfer(reader, count) || count > 1024) return 0;
            std::map<std::string, uint32_t> incoming;
            for (uint32_t i = 0; i < count; ++i) {
                std::string name;
                uint32_t type = 0;
                if (!read_string(reader, name, 4096) || !transfer(reader, type) || type > 3) return 0;
                incoming[name] = type;
            }
            script_schema = std::move(incoming);
        }
        for (const auto& entry : script_schema) {
            if (entry.second == 3) {
                std::string text;
                if (!read_string(reader, text, 0x100000)) return 0;
                if (entry.first == "filePath")
                    kinoko::legacy::StringView(kinoko::act::ScriptStorageView(script).bytes(&kinoko::act::ScriptStorageRecord::file_name)).assign(text.data(), static_cast<uint32_t>(text.size()));
            } else if (entry.second == 2) {
                uint8_t value;
                if (!transfer(reader, value)) return 0;
                if (entry.first == "compiled") kinoko::act::ScriptPayloadView(script).set(&kinoko::act::ScriptPayloadRecord::compiled, value);
            } else {
                uint32_t ignored;
                if (!transfer(reader, ignored)) return 0;
            }
        }
        return 1;
    } catch (...) { return 0; }
}

extern "C" int32_t __fastcall kinoko_method_read_act_script(void* script, void*, KinokoArchiveReader** holder, int32_t version) {
    if (!script || !holder || version != 1) return 0;
    const auto reader = *holder;
    if (!kinoko_act_read_script_properties((void*)(uintptr_t)(script), reader)) return 0;
    uint32_t size = 0;
    if (!transfer(reader, size) || size > 0x1000000) return 0;
    void* bytes = std::calloc(1, size ? size : 1);
    if (!bytes) return 0;
    kinoko::act::ScriptPayloadView payload(script);
    std::free(payload.get(&kinoko::act::ScriptPayloadRecord::bytes));
    payload.set(&kinoko::act::ScriptPayloadRecord::bytes, bytes);
    payload.set(&kinoko::act::ScriptPayloadRecord::size, size);
    if (size && !transfer(reader, bytes, size)) return 0;
    payload.set(&kinoko::act::ScriptPayloadRecord::loaded, uint8_t{1});
    return 1;
}

extern "C" int32_t __fastcall kinoko_method_write_act_script(void* script, void*, KinokoArchiveReader* writer) {
    if (!script || !writer) return 0;
    kinoko::act::ScriptPayloadView payload(script);
    const auto was_compiled = payload.get(&kinoko::act::ScriptPayloadRecord::compiled);
    payload.set(&kinoko::act::ScriptPayloadRecord::compiled, static_cast<uint8_t>(kinoko_act_script_output_compiled()));
    uint8_t has_schema = !kinoko_act_script_output_compiled();
    if (!transfer(writer, has_schema)) return 0;
    if (has_schema) {
        uint32_t count = 2, boolean_type = 2, string_type = 3;
        if (!transfer(writer, count) || !write_string(writer,"compiled",8) || !transfer(writer,boolean_type) ||
            !write_string(writer,"filePath",8) || !transfer(writer,string_type)) return 0;
    }
    auto path = kinoko::legacy::StringView(kinoko::act::ScriptStorageView(script).bytes(&kinoko::act::ScriptStorageRecord::file_name));
    auto compiled_flag = payload.get(&kinoko::act::ScriptPayloadRecord::compiled);
    if (!transfer(writer, compiled_flag) || !write_string(writer, path.data(), path.length())) return 0;
    const auto bytes = payload.get(&kinoko::act::ScriptPayloadRecord::bytes);
    auto size = payload.get(&kinoko::act::ScriptPayloadRecord::size);
    if (was_compiled) {
        // 4165A3 writes the stored compiled buffer directly, without another
        // size prefix, and does not inspect that virtual write's result.
        transfer(writer, bytes, size);
    } else if (!kinoko_act_script_output_compiled()) {
        if (!transfer(writer, size) || !transfer(writer, bytes, size)) return 0;
    } else {
        const auto start = seek(writer, 0, 1);
        uint32_t placeholder = 0;
        if (!transfer(writer, placeholder)) return 0;
        auto* vm = sq_open(1024);
        if (!vm) return 0;
        sqstd_seterrorhandlers(vm);
        sq_setprintfunc(vm, quiet_print);
        // Original constructs std::string from a C string, excluding its NUL.
        const auto* text = static_cast<const char *>(bytes);
        const auto* end = text ? static_cast<const char*>(std::memchr(text, 0, size)) : nullptr;
        const auto length = end ? static_cast<size_t>(end - text) : size;
        const bool compiled = kinoko::script::upstream::sqrat_compile_and_write(
            vm, text ? text : "", length, write_bytecode, writer);
        sq_close(vm);
        if (!compiled) return 0;
        // 416681 includes the four-byte placeholder in this length.
        auto length_with_prefix = static_cast<uint32_t>(seek(writer, 0, 1) - start);
        seek(writer, start, 0);
        if (!transfer(writer, length_with_prefix)) return 0;
        seek(writer, 0, 2);
    }
    payload.set(&kinoko::act::ScriptPayloadRecord::compiled, was_compiled);
    return 1;
}

