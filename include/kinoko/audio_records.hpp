#pragma once
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace kinoko::audio {
// These records are the verified native Win32 boundary, NOT VM objects.
// Retired vtable words are reserved layout only; no legacy objects live here.
// Unknown regions stay opaque. The DSP/decoder state itself lives in C++ owners.
struct PathRecord {
    union { char inline_text[16]; char* allocated_text; } storage;
    std::uint32_t length;
    std::uint32_t capacity;
    const char* c_str() const noexcept {
        return capacity < sizeof(storage.inline_text)
            ? storage.inline_text : storage.allocated_text;
    }
};
struct BufferRecord {
    PathRecord path;
    std::uint32_t reserved18;
    std::uint32_t playback_state;
    std::uint8_t ready;
    std::uint8_t reserved21[3];
    std::uint32_t retired_decoder_vtable;
    std::uint8_t decoder_storage[0x134c - 0x28];
    std::uint8_t looping;
    std::uint8_t reserved134d[3];
    std::uint32_t start_time;
    std::uint32_t fade_started;
    std::uint32_t fade_duration;
    float gain;
    float fade_from;
    float fade_to;
    float volume;
    std::uint8_t fade_pending;
    std::uint8_t reserved136d[3];
    std::uint32_t successor;
    std::uint32_t predecessor;
};
struct QueueNode {
    QueueNode* next;
    QueueNode* previous;
    std::uint32_t handle;
};
struct QueueRecord {
    QueueNode* head;
    std::uint32_t count;
    std::uint32_t reserved;
};
struct HandleTable {
    std::uint32_t retired_handle_vtable;
    BufferRecord** buffers_begin;
    BufferRecord** buffers_end;
    BufferRecord** buffers_capacity;
    std::uint32_t reserved10;
    std::uint32_t* generations_begin;
    std::uint32_t* generations_end;
    std::uint32_t* generations_capacity;
    std::uint32_t reserved20;
    QueueNode* live_handles;
    std::uint32_t live_count;
    std::uint32_t reserved2c;
    std::uint32_t next_generation;
    const void* lock_vtable;
    CRITICAL_SECTION lock;
};
struct alignas(8) ManagerRecord {
    std::uint8_t prefix[0x1c];
    const void* lock_vtable;
    CRITICAL_SECTION lock;
    HandleTable handles;
    QueueRecord active;
    QueueRecord pending;
    QueueRecord retired;
    float master_gain;
    float stream_gain;
    std::uint8_t tail[0x140 - 0xb4];
};
static_assert(sizeof(void*) == 4 && sizeof(CRITICAL_SECTION) == 24);
static_assert(sizeof(PathRecord) == 24 && sizeof(QueueNode) == 12);
static_assert(sizeof(BufferRecord) == 0x1378);
static_assert(sizeof(HandleTable) == 0x50 && sizeof(ManagerRecord) == 0x140);
static_assert(std::is_standard_layout_v<BufferRecord>);
static_assert(std::is_trivial_v<BufferRecord>);
#define KINOKO_AUDIO_FIELD(Type, Field, Offset) static_assert(offsetof(Type, Field) == Offset)
KINOKO_AUDIO_FIELD(PathRecord, capacity, 20);
KINOKO_AUDIO_FIELD(BufferRecord, playback_state, 0x1c);
KINOKO_AUDIO_FIELD(BufferRecord, ready, 0x20);
KINOKO_AUDIO_FIELD(BufferRecord, retired_decoder_vtable, 0x24);
KINOKO_AUDIO_FIELD(BufferRecord, looping, 0x134c);
KINOKO_AUDIO_FIELD(BufferRecord, start_time, 0x1350);
KINOKO_AUDIO_FIELD(BufferRecord, fade_started, 0x1354);
KINOKO_AUDIO_FIELD(BufferRecord, fade_duration, 0x1358);
KINOKO_AUDIO_FIELD(BufferRecord, gain, 0x135c);
KINOKO_AUDIO_FIELD(BufferRecord, fade_from, 0x1360);
KINOKO_AUDIO_FIELD(BufferRecord, fade_to, 0x1364);
KINOKO_AUDIO_FIELD(BufferRecord, volume, 0x1368);
KINOKO_AUDIO_FIELD(BufferRecord, fade_pending, 0x136c);
KINOKO_AUDIO_FIELD(BufferRecord, successor, 0x1370);
KINOKO_AUDIO_FIELD(BufferRecord, predecessor, 0x1374);
KINOKO_AUDIO_FIELD(HandleTable, buffers_begin, 4);
KINOKO_AUDIO_FIELD(HandleTable, buffers_end, 8);
KINOKO_AUDIO_FIELD(HandleTable, buffers_capacity, 0xc);
KINOKO_AUDIO_FIELD(HandleTable, generations_begin, 0x14);
KINOKO_AUDIO_FIELD(HandleTable, generations_end, 0x18);
KINOKO_AUDIO_FIELD(HandleTable, generations_capacity, 0x1c);
KINOKO_AUDIO_FIELD(HandleTable, live_handles, 0x24);
KINOKO_AUDIO_FIELD(HandleTable, next_generation, 0x30);
KINOKO_AUDIO_FIELD(HandleTable, lock, 0x38);
KINOKO_AUDIO_FIELD(ManagerRecord, lock, 0x20);
KINOKO_AUDIO_FIELD(ManagerRecord, handles, 0x38);
KINOKO_AUDIO_FIELD(ManagerRecord, active, 0x88);
KINOKO_AUDIO_FIELD(ManagerRecord, pending, 0x94);
KINOKO_AUDIO_FIELD(ManagerRecord, retired, 0xa0);
KINOKO_AUDIO_FIELD(ManagerRecord, master_gain, 0xac);
KINOKO_AUDIO_FIELD(ManagerRecord, stream_gain, 0xb0);
#undef KINOKO_AUDIO_FIELD
}
