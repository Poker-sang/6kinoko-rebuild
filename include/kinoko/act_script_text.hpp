#pragma once
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <string>

namespace kinoko::act {
// 415B80/415EA0/415F60: callbacks and unknown words stay constructor-owned.
struct ScriptTextRecord {
    const void *vtable;
    std::array<unsigned char, 60> callbacks;
    kinoko::legacy::StringRecord file_name;
    uint32_t unknown88;
    char *buffer;
    int32_t size;
    uint8_t dirty, compiled;
    std::array<unsigned char, 2> padding;
};
using ScriptTextView = kinoko::native::RecordView<ScriptTextRecord>;
static_assert(sizeof(ScriptTextRecord) == 104);
static_assert(offsetof(ScriptTextRecord, file_name) == 64);
static_assert(offsetof(ScriptTextRecord, buffer) == 92);
static_assert(offsetof(ScriptTextRecord, size) == 96);
static_assert(offsetof(ScriptTextRecord, compiled) == 101);

// 416700 assignment used by 41ECA0. Never copies live callback records.
inline void assign_script_payload(ScriptTextView destination, ScriptTextView source) {
    const kinoko::legacy::StringView input(source.bytes(&ScriptTextRecord::file_name));
    kinoko::legacy::StringView(destination.bytes(&ScriptTextRecord::file_name)).assign(input.data(), input.length());
    const auto size = source.get(&ScriptTextRecord::size);
    destination.set(&ScriptTextRecord::size, size);
    destination.set(&ScriptTextRecord::dirty, uint8_t{1});
    destination.set(&ScriptTextRecord::compiled, source.get(&ScriptTextRecord::compiled));
    if (size < 0) return;
    kinoko::legacy::Allocation<char> buffer(static_cast<char *>(std::calloc(size ? size : 1, 1)));
    if (!buffer) throw std::bad_alloc();
    if (auto *bytes = source.get(&ScriptTextRecord::buffer))
        std::memcpy(buffer.get(), bytes, static_cast<size_t>(size));
    std::free(destination.get(&ScriptTextRecord::buffer));
    destination.set(&ScriptTextRecord::buffer, buffer.release());
}

// GetText(415EA0) followed by SetText(415F60), including ignoring GetText's
// E_FAIL for compiled input. SetText does not change the destination flag.
inline void copy_script_text(ScriptTextView destination, ScriptTextView source) {
    const std::string text = source.get(&ScriptTextRecord::compiled)
        ? "/* This script is compiled. Can't read this. Don't edit this.*/"
        : source.get(&ScriptTextRecord::buffer);
    kinoko::legacy::StringView(destination.bytes(&ScriptTextRecord::file_name)).assign("", 0);
    kinoko::legacy::Allocation<char> buffer(static_cast<char *>(std::malloc(text.size() + 1)));
    if (!buffer) throw std::bad_alloc();
    std::memcpy(buffer.get(), text.c_str(), text.size() + 1);
    std::free(destination.get(&ScriptTextRecord::buffer));
    destination.set(&ScriptTextRecord::buffer, buffer.release());
    destination.set(&ScriptTextRecord::size, static_cast<int32_t>(text.size() + 1));
    destination.set(&ScriptTextRecord::dirty, uint8_t{1});
}
}
