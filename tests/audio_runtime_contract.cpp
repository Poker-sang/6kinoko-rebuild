// Exercise the production implementation, including its private ownership
// boundaries. This target compiles it once, without the generated game host.
#include "../src/reconstructed/audio_runtime.cpp"
#include <cstdio>
#include <vector>
#include <type_traits>
#include "directsound_fixture.hpp"

namespace {
std::vector<unsigned char> fixture;
int fixture_reader[4]{};
std::size_t fixture_offset = 0;
int reader_destroys = 0;
int critical_section_identity{};
#define CHECK(value) do { if (!(value)) { \
    std::fprintf(stderr, "audio contract line %d: %s\n", __LINE__, #value); return 1; \
} } while (0)

using Buffer = AudioTestBuffer;
static_assert(!std::is_copy_constructible_v<BgmTrack>);
static_assert(std::is_nothrow_move_constructible_v<BgmTrack>);
}

extern "C" {
int32_t g637 = 0, g765 = 1, g876 = 0, g878 = 0;
char* g877 = nullptr;
char g874 = 0;
const KinokoAudioHostSymbols* kinoko_audio_host_symbols(void) {
    static const KinokoAudioHostSymbols symbols{&critical_section_identity, "test"};
    return &symbols;
}
int32_t function_407370(int32_t slot, const char*) {
    fixture_offset = 0;
    fixture_reader[1] = 1;
    fixture_reader[3] = static_cast<int32_t>(fixture.size());
    *kinoko::legacy::pointer<int32_t>(slot) = kinoko::legacy::address(fixture_reader);
    return 1;
}
int32_t function_407300(int32_t) { return 0; }
int32_t retdec_reader_read_exact(int32_t, void* output, uint32_t size) {
    if (fixture_offset + size > fixture.size()) return 0;
    std::memcpy(output, fixture.data() + fixture_offset, size);
    fixture_offset += size;
    return 1;
}
void retdec_destroy_reader(int32_t*) { ++reader_destroys; }
uint32_t retdec_safe_c_string_length(const char* text) { return text ? static_cast<uint32_t>(std::strlen(text)) : 0; }
int32_t retdec_string_assign_n(int32_t* object, const char* text, uint32_t size) {
    auto& path = *reinterpret_cast<PathRecord*>(object);
    if (path.capacity >= 16) std::free(path.storage.allocated_text);
    if (size < 16) {
        path.capacity = 15;
        std::memcpy(path.storage.inline_text, text, size);
        path.storage.inline_text[size] = 0;
    } else {
        path.capacity = size;
        path.storage.allocated_text = static_cast<char*>(std::malloc(size + 1));
        std::memcpy(path.storage.allocated_text, text, size);
        path.storage.allocated_text[size] = 0;
    }
    path.length = size;
    return kinoko::legacy::address(object);
}
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_hresult(const char*, long) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
}

int main() {
    QueueRecord queue{};
    CHECK(retdec_audio_manager_list_init(queue));
    CHECK(queue.head->next == queue.head && queue.head->previous == queue.head);
    std::uint32_t value = 77;
    CHECK(!retdec_audio_list_pop(queue.head, &value) && value == 77);
    for (std::uint32_t n = 0; n < 128; ++n) retdec_audio_list_push(queue.head, n);
    for (std::uint32_t n = 0; n < 128; ++n) {
        CHECK(retdec_audio_list_pop(queue.head, &value));
        CHECK(value == n);
    }
    CHECK(queue.head->next == queue.head && queue.head->previous == queue.head);
    retdec_audio_clear_queue(queue.head);
    CHECK(queue.head == nullptr);

    retdec_audio_manager_construct();
    CHECK(g_retdec_audio_manager_initialized);
    auto& manager = *retdec_audio_manager_this();
    CHECK(manager.master_gain == 1 && manager.stream_gain == 1);
    std::uint32_t first = 0, second = 0;
    CHECK(retdec_audio_handle_create(&manager.handles, &first));
    CHECK(retdec_audio_handle_create(&manager.handles, &second));
    CHECK(first == 0x10000 && second == 0x20001);
    auto* record = retdec_audio_handle_lookup(&manager.handles, first);
    CHECK(record && record->gain == 1 && record->playback_state == 3 && !record->ready);
    CHECK(record->path.capacity == 15 && record->path.length == 0);
    CHECK(!retdec_audio_handle_lookup(&manager.handles, first + 0x10000));
    CHECK(!retdec_audio_handle_lookup(&manager.handles, 0xffff));
    const char path[] = "long-path-owned-by-buffer.cv3";
    retdec_string_assign_n(reinterpret_cast<int32_t*>(&record->path), path, sizeof(path) - 1);
    CHECK(std::strcmp(retdec_audio_buffer_path(record), path) == 0);

    Buffer source;
    {
        BgmTrack track;
        track.buffer.reset(&source);
        track.handle = 29;
        track.channels = 1;
        track.volume = 1;
        track.buffer_bytes = 16;
        track.write_offset = 12;
        const unsigned char samples[]{1, 2, 3, 4, 5, 6, 7, 8};
        CHECK(retdec_bgm_write_buffer(&track, samples, sizeof(samples)));
        CHECK(source.locks == 1 && source.unlocks == 1);
        CHECK(std::memcmp(source.bytes.data() + 12, samples, 4) == 0);
        CHECK(std::memcmp(source.bytes.data(), samples + 4, 4) == 0);
        CHECK(track.write_offset == 4);
        source.fail_lock = true;
        CHECK(!retdec_bgm_write_buffer(&track, samples, sizeof(samples)));
        CHECK(source.unlocks == 1 && track.write_offset == 4);
        source.fail_lock = false;
        auto moved = std::move(track);
        CHECK(!track.buffer && moved.buffer.get() == &source && source.releases == 0);
        retdec_bgm_release_state(&moved);
        CHECK(source.releases == 1 && source.stops == 1 && !moved.buffer);
        retdec_bgm_release_state(&moved);
        CHECK(source.releases == 1);
    }
    Buffer sound;
    g_retdec_se_entry_count = 1;
    g_retdec_se_entries[0].id = 123;
    g_retdec_se_entries[0].buffer.reset(&sound);
    sound.status = DSBSTATUS_PLAYING;
    CHECK(function_470980(123) == 1);
    CHECK(sound.stops == 1 && sound.plays == 1 && sound.position == 0);
    retdec_se_entries_release();
    CHECK(sound.releases == 1 && g_retdec_se_entry_count == 0);
    retdec_se_entries_release();
    CHECK(sound.releases == 1);

    // CV3 is the original packed 18-byte WAVEFORMATEX + 4-byte payload length.
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM; format.nChannels = 1;
    format.nSamplesPerSec = 22050; format.nAvgBytesPerSec = 44100;
    format.nBlockAlign = 2; format.wBitsPerSample = 16;
    fixture.resize(26);
    std::memcpy(fixture.data(), &format, sizeof(format));
    const unsigned char pcm[]{0x11, 0x22, 0x33, 0x44};
    const std::uint32_t payload = sizeof(pcm);
    std::memcpy(fixture.data() + 18, &payload, 4);
    std::memcpy(fixture.data() + 22, pcm, 4);
    unsigned char* decoded = nullptr;
    DWORD decoded_size = 0;
    WAVEFORMATEX parsed{};
    CHECK(retdec_se_parse_wave_asset("fixture.cv3", &parsed, &decoded, &decoded_size));
    CHECK(decoded_size == 4 && parsed.nSamplesPerSec == 22050);
    CHECK(std::memcmp(decoded, pcm, 4) == 0);
    std::free(decoded);
    fixture.resize(24);
    decoded = nullptr;
    CHECK(!retdec_se_parse_wave_asset("fixture.cv3", &parsed, &decoded, &decoded_size));
    CHECK(decoded == nullptr && reader_destroys == 2);

    // Real Windows event/thread contracts, no audio device or original assets.
    CHECK(function_40a3d0() != 0);
    CHECK(g_retdec_audio_threads_initialized);
    CHECK(function_40b3a0() == 1);
    CHECK(!g_retdec_audio_threads_initialized && !g_retdec_audio_update_thread);
    CHECK(!g_retdec_audio_loader_thread && !g_retdec_audio_queue_event && !g_retdec_audio_stop_event);
    CHECK(!g_retdec_audio_manager_initialized);
    retdec_audio_manager_construct();
    CHECK(g_retdec_audio_manager_initialized);
    CHECK(function_40b3a0() == 1);
    std::puts("PASS: audio layouts, handles, FIFO, buffer ownership, native SDK calls, CV3 and worker teardown");
}
