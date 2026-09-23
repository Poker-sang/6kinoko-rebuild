#pragma once
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace kinoko::act {
// CActScript's suffix, shared by its loader, writer and destructor. The
// callback/string prefix is owned by the existing script implementation.
struct ScriptPayloadRecord {
    std::array<uint8_t, 92> prefix;
    void *bytes;
    uint32_t size;
    uint8_t loaded, compiled;
    std::array<uint8_t, 2> padding;
};
using ScriptPayloadView = kinoko::native::RecordView<ScriptPayloadRecord>;
static_assert(sizeof(void *) == 4 && sizeof(ScriptPayloadRecord) == 104);
static_assert(offsetof(ScriptPayloadRecord, bytes) == 92);
static_assert(offsetof(ScriptPayloadRecord, size) == 96);
static_assert(offsetof(ScriptPayloadRecord, loaded) == 100);
static_assert(offsetof(ScriptPayloadRecord, compiled) == 101);
}
