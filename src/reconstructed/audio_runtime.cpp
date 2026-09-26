#include "kinoko/audio_runtime.h"
#include "kinoko/audio_host.h"
#include "kinoko/audio_records.hpp"
#include "kinoko/audio_math.h"
#include "kinoko/com_owner.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/windows_owner.hpp"
#include <mmsystem.h>
#include <dsound.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

// The original codec uses memory callbacks, not host FILE* or new search paths.
#include "kinoko/vorbis_decoder.hpp"

using float32_t = float;
using float80_t = long double;
using namespace kinoko::audio;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::windows::CriticalLock;

static int32_t run_audio_update_worker();
static int32_t run_audio_loader_worker();
static int32_t service_audio_tick();

namespace {
constexpr DWORD RETDEC_BGM_BUFFER_BYTES = 0x100000u;
constexpr DWORD RETDEC_BGM_CHUNK_BYTES = 0x8000u;
constexpr int RETDEC_BGM_MAX_CHANNELS = 8;
constexpr DWORD RETDEC_BGM_GUARD_BYTES = 16u;
constexpr int RETDEC_SE_MAX_ENTRIES = 128;
static_assert(sizeof(WAVEFORMATEX) == 18 && sizeof(DSBUFFERDESC) == 36);

// The track exclusively owns its secondary buffer, decoder and decoder input.
// Moving transfers those owners; copying is impossible. Native ABI buffer
// records only describe requests and never own these resources.
struct BgmTrack {
    BgmTrack() = default;
    BgmTrack(const BgmTrack&) = delete;
    BgmTrack& operator=(const BgmTrack&) = delete;
    BgmTrack(BgmTrack&&) noexcept = default;
    BgmTrack& operator=(BgmTrack&& other) noexcept {
        if (this != &other) {
            // Close the decoder before releasing its borrowed input memory.
            this->~BgmTrack();
            new (this) BgmTrack(std::move(other));
        }
        return *this;
    }
    kinoko::legacy::Allocation<unsigned char> encoded_data;
    kinoko::legacy::Allocation<short> decoded_samples;
    kinoko::legacy::Allocation<short> decode_scratch;
    std::unique_ptr<VorbisDecoder> decoder;
    kinoko::ComOwner<IDirectSoundBuffer> buffer;
    DWORD buffer_bytes = 0;
    DWORD encoded_bytes = 0;
    DWORD decoded_bytes = 0;
    DWORD sample_rate = 0;
    WORD channels = 0;
    std::uint32_t handle = 0;
    DWORD loop_start_frame = 0;
    DWORD loop_end_frame = 0;
    DWORD source_frame = 0;
    DWORD write_offset = 0;
    DWORD write_window_start = 0;
    DWORD play_offset = 0;
    DWORD buffered_bytes = 0;
    DWORD start_time = 0;
    int source_ended = 0;
    int looping = 0;
    int started = 0;
    float volume = 0;
    float fade_from = 0;
    float fade_to = 0;
    DWORD fade_started = 0;
    DWORD fade_duration = 0;
    int playing = 0;
    bool retire_after_fade = false;
    bool retirement_requested = false;
};
struct SoundSlot {
    kinoko::ComOwner<IDirectSoundBuffer> buffer;
    DWORD buffer_bytes = 0;
    int in_use = 0;
};
struct SoundEntry {
    int id = 0;
    kinoko::ComOwner<IDirectSoundBuffer> buffer;
    DWORD buffer_bytes = 0;
};
struct SoundPool {
    SoundSlot stream_slots[32];
    float master_volume = 0;
    int initialized = 0;
};
struct AudioDevice {
    HMODULE module = nullptr;
    kinoko::ComOwner<IDirectSound8> device;
    kinoko::ComOwner<IDirectSoundBuffer> primary;
    kinoko::ComOwner<IDirectSound3DListener> listener;
    void reset() noexcept {
        listener.reset();
        primary.reset();
        device.reset();
        if (const auto old = std::exchange(module, nullptr)) FreeLibrary(old);
    }
    ~AudioDevice() { reset(); }
    AudioDevice() = default;
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;
};
// Declared before tracks: the device outlives all secondary-buffer owners.
AudioDevice g_audio_device;
ManagerRecord g_retdec_audio_manager_state{};
int g_retdec_audio_manager_initialized = 0;
SoundPool g_retdec_se_pool;
SoundEntry g_retdec_se_entries[RETDEC_SE_MAX_ENTRIES];
int g_retdec_se_entry_count = 0;
BgmTrack g_retdec_bgm_track;
std::list<BgmTrack> fading_tracks;
inline int32_t& active_bgm_slot = kinoko_active_bgm_slot;
inline char& packed_assets_slot = kinoko_packed_assets;
inline int32_t& primary_device_slot = kinoko_audio_primary_device_slot;
inline char*& dsound_device_slot = kinoko_audio_device_slot;
inline int32_t& listener_slot = kinoko_audio_listener_slot;
// Own both workers, their wake events and the lock protecting playback state.
// Events and the lock outlive the joined workers, including partial startup.
struct AudioWorkers {
    kinoko::windows::HandleOwner queue_event, stop_event, update_thread, loader_thread;
    volatile LONG running = 0;
    bool initialized = false;
    CRITICAL_SECTION lock{};
    AudioWorkers() { InitializeCriticalSection(&lock); }
    bool is_running() { return InterlockedCompareExchange(&running, 0, 0) != 0; }
    void notify_loader() { if (queue_event) SetEvent(queue_event.get()); }
    void stop_and_join() {
        InterlockedExchange(&running, 0);
        if (stop_event) SetEvent(stop_event.get());
        notify_loader();
        for (auto* thread : {&update_thread, &loader_thread}) {
            if (*thread) {
                WaitForSingleObject(thread->get(), INFINITE);
                thread->reset();
            }
        }
        queue_event.reset();
        stop_event.reset();
        initialized = false;
    }
    ~AudioWorkers() { stop_and_join(); DeleteCriticalSection(&lock); }
    AudioWorkers(const AudioWorkers&) = delete;
    AudioWorkers& operator=(const AudioWorkers&) = delete;
} audio_workers;
float g_retdec_audio_master_volume = 1.0f;
// The original C ABI publishes the currently selected BGM handle here. The
// track pool owns playback resources; this slot is only its active identity.
int32_t& active_bgm_handle() { return active_bgm_slot; }
bool packed_sound_assets() { return packed_assets_slot != 0; }
void sync_audio_device_aliases() noexcept {
    // Read-only borrows for not-yet-migrated C entry points, never extra owners.
    primary_device_slot = address(g_audio_device.primary.get());
    dsound_device_slot = reinterpret_cast<char*>(g_audio_device.device.get());
    listener_slot = address(g_audio_device.listener.get());
}
DWORD WINAPI audio_update_worker(void*) { return run_audio_update_worker(); }
DWORD WINAPI audio_loader_worker(void*) { return run_audio_loader_worker(); }
}
static void kinoko_trace_audio_text(const char *label, const char *value);
static LONG kinoko_audio_volume_db(float gain);
static void kinoko_release_dsound_buffer(IDirectSoundBuffer* buffer) noexcept;
static int kinoko_create_secondary_buffer(const WAVEFORMATEX* format,
                                          DWORD buffer_bytes,
                                          IDirectSoundBuffer** result);
static int kinoko_read_asset_bytes(const char *path,
                                   unsigned char **data,
                                   DWORD *size);
static uint32_t kinoko_bgm_read_u32(const unsigned char *bytes);
static int kinoko_bgm_read_loop_points(const char *path,
                                       DWORD *loop_start,
                                       DWORD *loop_end);
static int kinoko_decode_bgm(const char *path,
                             short **samples,
                             DWORD *sample_bytes,
                             DWORD *sample_rate,
                             WORD *channels);
static int kinoko_fill_dsound_buffer(IDirectSoundBuffer *buffer,
                                     const void *samples,
                                     DWORD sample_bytes);
static void kinoko_set_dsound_volume(IDirectSoundBuffer* buffer, float gain);
static void kinoko_bgm_write_guard(void *memory, DWORD size);
static int kinoko_bgm_check_guard(const void *memory, DWORD size);
static void kinoko_bgm_release_state(BgmTrack *track);
static BgmTrack *kinoko_bgm_find_track(uint32_t handle);
static void kinoko_bgm_apply_state_volume(BgmTrack *track,
                                          float gain);
static int kinoko_bgm_write_buffer(BgmTrack *track,
                                   const void *samples, DWORD bytes);
static int kinoko_bgm_decode_loop_frames(BgmTrack *track,
                                         short *output,
                                         DWORD requested_frames);
static DWORD kinoko_bgm_decode_chunk(BgmTrack *track,
                                     unsigned char *output, DWORD bytes);
static int kinoko_bgm_fill_chunk(BgmTrack *track, DWORD bytes);
static void kinoko_bgm_release_track_locked(void);
static void kinoko_bgm_release_track(void);
static void kinoko_bgm_release_all_tracks_locked(void);
static void kinoko_bgm_release_all_tracks(void);
static void kinoko_se_entries_release(void);
static void kinoko_se_pool_release(void);
static int kinoko_se_parse_wave_asset(const char *path,
                                      WAVEFORMATEX *format,
                                      unsigned char **samples,
                                      DWORD *sample_bytes);
static int kinoko_se_replace_extension(const char *source, char *path,
                                       size_t path_size);
static int kinoko_se_load_entry(int id, const char *source);
static int kinoko_se_pool_initialize(void);
static void kinoko_se_pool_set_volume(float gain);
static void kinoko_bgm_apply_track_volume(float gain);
static void kinoko_bgm_update_fade_locked(void);
void kinoko_bgm_update_fade(void);
static void kinoko_bgm_begin_fade_locked(BgmTrack *track,
                                         DWORD duration, float target,
                                         DWORD start_delay, bool retire_after_fade = false);
static void kinoko_bgm_begin_fade(DWORD duration, float target);
static void kinoko_bgm_begin_fade_for_handle(uint32_t handle,
                                             DWORD duration,
                                             DWORD start_delay,
                                             float target, bool retire_after_fade = false);
static void kinoko_bgm_stop_for_handle(uint32_t handle);
static void kinoko_bgm_release_for_handle(uint32_t handle);
static void retire_playback_request(uint32_t handle);
static int kinoko_bgm_prepare_track_default_math(uint32_t handle, const char *path,
                                    int looping, float32_t volume);
static int kinoko_bgm_prepare_track(uint32_t handle, const char *path,
                                    int looping, float32_t volume);
static void kinoko_bgm_process_pending_locked(void);
static void kinoko_bgm_start_track(BgmTrack *track);
static void kinoko_bgm_service_track(BgmTrack *track);
static void kinoko_bgm_service_all_locked(void);
static void kinoko_bgm_archive_current_track(void);
static void kinoko_bgm_stop(int reset_position);
static ManagerRecord* kinoko_audio_manager_this() noexcept;
static bool kinoko_audio_manager_list_init(QueueRecord& queue);
static void kinoko_audio_list_push(HandleQueue* list, std::uint32_t value);
static bool kinoko_audio_list_pop(HandleQueue* list, std::uint32_t* value);
static const char* kinoko_audio_buffer_path(const BufferRecord* buffer) noexcept;
static void kinoko_audio_buffer_initialize(BufferRecord* buffer) noexcept;
static BufferRecord* kinoko_audio_handle_lookup(HandleTable* manager,
                                               std::uint32_t handle);
static bool kinoko_audio_handle_create(HandleTable* manager, std::uint32_t* output);
static void kinoko_audio_manager_construct();
static std::uint32_t* allocate_playback_handle(ManagerRecord* manager,
                                              std::uint32_t* output);
static int32_t prepare_playback_request(ManagerRecord* this_ptr,
                                          int32_t handle,
                                          const char* source,
                                          int32_t buffer_flag,
                                          int32_t queue_mode,
                                          float32_t volume);
static int32_t schedule_playback_start(ManagerRecord* this_ptr,
                                          int32_t handle,
                                          int32_t delay);
static int32_t fade_out_playback(ManagerRecord* this_ptr,
                                          int32_t handle,
                                          int32_t duration,
                                          int32_t start_delay,
                                          float32_t target);
static void kinoko_loadse_blob(const char *source);

// The ABI records are table-owned allocations. Request/fade queues carry only
// handles; they must never free a BufferRecord or a decoder a second time.
static void kinoko_audio_clear_queue(HandleQueue*& head) noexcept {
    delete std::exchange(head,nullptr);
}
static void kinoko_audio_clear_handle_allocations(HandleTable& table) noexcept {
    delete std::exchange(table.storage,nullptr);
    kinoko_audio_clear_queue(table.live_handles);
}
static void kinoko_audio_manager_destroy() noexcept {
    if (!g_retdec_audio_manager_initialized) return;
    auto& state = g_retdec_audio_manager_state;
    kinoko_audio_clear_queue(state.active.head);
    kinoko_audio_clear_queue(state.pending.head);
    kinoko_audio_clear_queue(state.retired.head);
    kinoko_audio_clear_handle_allocations(state.handles);
    DeleteCriticalSection(&state.handles.lock);
    DeleteCriticalSection(&state.lock);
    state = ManagerRecord{};
    g_retdec_audio_manager_initialized = 0;
}

static void kinoko_trace_audio_text(const char *label, const char *value)
{
    char message[512];

    if (label == NULL || value == NULL)
        return;
    wsprintfA(message, "%s:%s", label, value);
    kinoko_trace(message);
}

static LONG kinoko_audio_volume_db(float gain)
{
    double value;

    if (!(gain > 0.0020000001f))
        return -10000;
    value = 3322.0001220703125 * log10((double)gain);
    if (value < -10000.0)
        return -10000;
    if (value > 0.0)
        return 0;
    return (LONG)value;
}

static void kinoko_release_dsound_buffer(IDirectSoundBuffer* buffer) noexcept {
    if (buffer) buffer->Release();
}


static int kinoko_create_secondary_buffer(const WAVEFORMATEX* format,
                                          DWORD buffer_bytes,
                                          IDirectSoundBuffer** result) {
    if (!result) return 0;
    *result = nullptr;
    auto* device = g_audio_device.device.get();
    if (!device || !format || !buffer_bytes) return 0;
    DSBUFFERDESC description{};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLVOLUME |
        DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    description.dwBufferBytes = buffer_bytes;
    description.lpwfxFormat = const_cast<WAVEFORMATEX*>(format);
    kinoko_trace_i32("audio:create-bytes", static_cast<int32_t>(buffer_bytes));
    kinoko_trace_i32("audio:create-rate", static_cast<int32_t>(format->nSamplesPerSec));
    kinoko_trace_i32("audio:create-channels", static_cast<int32_t>(format->nChannels));
    const auto hr = device->CreateSoundBuffer(&description, result, nullptr);
    kinoko_trace_hresult("audio:secondary-create-hr", hr);
    if (FAILED(hr) || !*result) {
        *result = nullptr;
        return 0;
    }
    return 1;
}


static int kinoko_read_asset_bytes(const char *path,
                                   unsigned char **data,
                                   DWORD *size)
{
    KinokoArchiveReader *reader_slot = nullptr;
    KinokoArchiveReader *reader;
    DWORD asset_size;
    unsigned char *contents;

    if (data == NULL || size == NULL || path == NULL)
        return 0;
    *data = NULL;
    *size = 0;
    if (!kinoko_reader_open(&reader_slot, path))
        return 0;

    reader = reader_slot;
    if (reader == NULL) {
        kinoko_reader_close(reader);
        return 0;
    }
    SetLastError(NO_ERROR);
    asset_size = kinoko_reader_size(reader);
    if (asset_size == 0 || (asset_size == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)) {
        kinoko_reader_close(reader);
        return 0;
    }

    contents = (unsigned char *)malloc(asset_size);
    if (contents == NULL ||
        !kinoko_reader_read_exact(reader_slot, contents, asset_size)) {
        free(contents);
        kinoko_reader_close(reader);
        return 0;
    }
    kinoko_reader_close(reader);
    *data = contents;
    *size = asset_size;
    return 1;
}

static uint32_t kinoko_bgm_read_u32(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static int kinoko_bgm_read_loop_points(const char *path,
                                       DWORD *loop_start,
                                       DWORD *loop_end)
{
    char sidecar[MAX_PATH];
    unsigned char *data = NULL;
    DWORD size = 0;
    DWORD start = 0;
    DWORD length = 0;
    DWORD index;
    size_t path_length;

    if (path == NULL || loop_start == NULL || loop_end == NULL)
        return 0;
    path_length = strlen(path);
    if (path_length < 4 || path_length >= sizeof(sidecar))
        return 0;
    memcpy(sidecar, path, path_length + 1);
    sidecar[path_length - 3] = 's';
    sidecar[path_length - 2] = 'f';
    sidecar[path_length - 1] = 'l';
    if (!kinoko_read_asset_bytes(sidecar, &data, &size))
        return 0;

    for (index = 0; index + 8 <= size; ++index) {
        const unsigned char *chunk = data + index;
        DWORD chunk_size = kinoko_bgm_read_u32(chunk + 4);

        if (memcmp(chunk, "cue ", 4) == 0 && chunk_size >= 28 &&
            chunk_size <= size - index - 8 &&
            kinoko_bgm_read_u32(chunk + 8) != 0) {
            const unsigned char *cue_point = chunk + 12;
            start = kinoko_bgm_read_u32(cue_point + 4);
        }
        if (memcmp(chunk, "ltxt", 4) == 0 && chunk_size >= 8 &&
            chunk_size <= size - index - 8)
            length = kinoko_bgm_read_u32(chunk + 12);
    }
    free(data);
    if (start == 0 || length == 0 || start > UINT32_MAX - length)
        return 0;
    *loop_start = start;
    *loop_end = start + length;
    return *loop_end > *loop_start;
}

static int kinoko_decode_bgm(const char *path,
                             short **samples,
                             DWORD *sample_bytes,
                             DWORD *sample_rate,
                             WORD *channels)
{
    unsigned char *encoded = NULL;
    DWORD encoded_size = 0;
    std::unique_ptr<VorbisDecoder> decoder;
    VorbisDecoder::Format info;
    unsigned int frame_count;
    size_t short_count;
    short *decoded = NULL;
    int error = 0;
    int frame_cursor = 0;
    int frame_total;

    if (samples == NULL || sample_bytes == NULL || sample_rate == NULL ||
        channels == NULL)
        return 0;
    *samples = NULL;
    *sample_bytes = 0;
    *sample_rate = 0;
    *channels = 0;
    if (!kinoko_read_asset_bytes(path, &encoded, &encoded_size))
        return 0;

    decoder = VorbisDecoder::open(encoded, encoded_size, error);
    if (decoder == NULL) {
        free(encoded);
        kinoko_trace_i32("audio:vorbis-open-error", error);
        return 0;
    }
    info = decoder->format();
    frame_count = static_cast<unsigned int>(decoder->frame_count());
    if (info.channels <= 0 || info.channels > 8 || info.sample_rate == 0 ||
        frame_count == 0) {
        decoder.reset();
        free(encoded);
        return 0;
    }

    short_count = (size_t)frame_count * (size_t)info.channels;
    if (short_count > (size_t)0x7fffffff ||
        short_count > (size_t)0xffffffffu / sizeof(short)) {
        decoder.reset();
        free(encoded);
        return 0;
    }
    decoded = (short *)malloc(short_count * sizeof(short));
    if (decoded == NULL) {
        decoder.reset();
        free(encoded);
        return 0;
    }

    frame_total = (int)frame_count;
    while (frame_cursor < frame_total) {
        int remaining_frames = frame_total - frame_cursor;
        int got = decoder->read_frames(decoded +
            (size_t)frame_cursor * (size_t)info.channels, remaining_frames);
        if (got <= 0)
            break;
        frame_cursor += got;
    }
    decoder.reset();
    free(encoded);
    if (frame_cursor <= 0) {
        free(decoded);
        return 0;
    }

    *samples = decoded;
    *sample_bytes = (DWORD)((size_t)frame_cursor *
                            (size_t)info.channels * sizeof(short));
    *sample_rate = info.sample_rate;
    *channels = (WORD)info.channels;
    return 1;
}

static int kinoko_fill_dsound_buffer(IDirectSoundBuffer *buffer,
                                     const void *samples,
                                     DWORD sample_bytes)
{

    void *part1 = NULL;
    void *part2 = NULL;
    DWORD part1_bytes = 0;
    DWORD part2_bytes = 0;
    DWORD first_copy;
    HRESULT hr;

    if (buffer == NULL || samples == NULL || sample_bytes == 0)
        return 0;
    hr = (buffer)->Lock(0, sample_bytes, &part1, &part1_bytes, &part2, &part2_bytes, 0);
    if (FAILED(hr)) {
        kinoko_trace_hresult("audio:secondary-lock-hr", hr);
        return 0;
    }
    first_copy = part1_bytes < sample_bytes ? part1_bytes : sample_bytes;
    if (first_copy != 0)
        memcpy(part1, samples, first_copy);
    if (part2_bytes != 0 && first_copy < sample_bytes) {
        DWORD second_copy = part2_bytes < sample_bytes - first_copy
            ? part2_bytes : sample_bytes - first_copy;
        memcpy(part2, (const unsigned char *)samples + first_copy,
               second_copy);
    }
    hr = (buffer)->Unlock(part1, part1_bytes, part2, part2_bytes);
    if (FAILED(hr)) {
        kinoko_trace_hresult("audio:secondary-unlock-hr", hr);
        return 0;
    }
    return first_copy + part2_bytes >= sample_bytes;
}

static void kinoko_set_dsound_volume(IDirectSoundBuffer* buffer, float gain) {
    if (buffer) buffer->SetVolume(kinoko_audio_volume_db(gain));
}


static void kinoko_bgm_write_guard(void *memory, DWORD size)
{
    DWORD index;
    unsigned char *bytes = (unsigned char *)memory;

    if (bytes == NULL)
        return;
    for (index = 0; index < RETDEC_BGM_GUARD_BYTES; ++index)
        bytes[size + index] = 0xA5u;
}

static int kinoko_bgm_check_guard(const void *memory, DWORD size)
{
    DWORD index;
    const unsigned char *bytes = (const unsigned char *)memory;

    if (bytes == NULL)
        return 0;
    for (index = 0; index < RETDEC_BGM_GUARD_BYTES; ++index) {
        if (bytes[size + index] != 0xA5u)
            return 0;
    }
    return 1;
}

static void kinoko_bgm_release_state(BgmTrack *track)
{
    if (track == NULL)
        return;
    if (track->buffer.get() != NULL) {
        DWORD status = 0;
        int wait_count;
        kinoko_trace_i32("bgm:release-handle", (int32_t)track->handle);

                    (track->buffer.get())->Stop();
        {
            for (wait_count = 0; wait_count < 100; ++wait_count) {
                if (FAILED((track->buffer.get())->GetStatus(&status)) ||
                    (status & 1u) == 0)
                    break;
                Sleep(1);
            }
        }
        track->buffer.reset();
    }
    if (track->decoder.get() != NULL)
        track->decoder.reset();
    /* BGM owns the dynamically created DirectSound buffer and its decoder. */
    track->encoded_data.reset();
    track->decoded_samples.reset();
    track->decode_scratch.reset();
    *track = BgmTrack{};
}

static BgmTrack *kinoko_bgm_find_track(uint32_t handle)
{
    int index;

    if (handle == 0)
        return NULL;
    if (g_retdec_bgm_track.buffer.get() != NULL &&
        g_retdec_bgm_track.handle == handle)
        return &g_retdec_bgm_track;
    for (auto& track : fading_tracks)
        if (track.buffer && track.handle == handle) return &track;
    return NULL;
}

static void kinoko_bgm_apply_state_volume(BgmTrack *track,
                                          float gain)
{
    float effective_gain;

    if (track == NULL || track->buffer.get() == NULL)
        return;
    track->volume = gain;
    effective_gain = gain * g_retdec_audio_master_volume;
    kinoko_set_dsound_volume(track->buffer.get(), effective_gain);
}

static int kinoko_bgm_write_buffer(BgmTrack *track,
                                   const void *samples, DWORD bytes)
{

    void *part1 = NULL;
    void *part2 = NULL;
    DWORD part1_bytes = 0;
    DWORD part2_bytes = 0;
    DWORD first_copy;
    DWORD second_copy;
    HRESULT hr;

    if (track == NULL || track->buffer.get() == NULL || samples == NULL ||
        bytes == 0 || track->buffer_bytes == 0 ||
        bytes > track->buffer_bytes)
        return 0;
    kinoko_trace_i32("bgm:write-handle", (int32_t)track->handle);
    kinoko_trace_i32("bgm:write-offset", (int32_t)track->write_offset);
    kinoko_trace_i32("bgm:write-bytes", (int32_t)bytes);
    hr = (track->buffer.get())->Lock(track->write_offset, bytes, &part1, &part1_bytes, &part2, &part2_bytes, 0);
    if (FAILED(hr)) {
        kinoko_trace_hresult("audio:stream-lock-hr", hr);
        return 0;
    }
    kinoko_trace_i32("bgm:lock-part1", (int32_t)part1_bytes);
    kinoko_trace_i32("bgm:lock-part2", (int32_t)part2_bytes);
    first_copy = part1_bytes < bytes ? part1_bytes : bytes;
    second_copy = bytes - first_copy;
    if (second_copy > part2_bytes)
        second_copy = part2_bytes;
    if (first_copy != 0)
        memcpy(part1, samples, first_copy);
    if (second_copy != 0)
        memcpy(part2, (const unsigned char *)samples + first_copy,
               second_copy);
    hr = (track->buffer.get())->Unlock(part1, part1_bytes, part2, part2_bytes);
    if (FAILED(hr)) {
        kinoko_trace_hresult("audio:stream-unlock-hr", hr);
        return 0;
    }
    track->write_offset = (track->write_offset + bytes) &
                          (track->buffer_bytes - 1);
    return first_copy + second_copy == bytes;
}

static int kinoko_bgm_decode_loop_frames(BgmTrack *track,
                                         short *output,
                                         DWORD requested_frames)
{
    DWORD request_frames;
    DWORD seek_frame;
    int got;

    if (track == NULL || track->decoder.get() == NULL || output == NULL ||
        requested_frames == 0)
        return 0;

    /* 412240 reads at most 4096 PCM bytes, then seeks past the loop start
       by any overshoot.  Track delivered samples, not decoder read-ahead. */
    request_frames = 4096u / ((DWORD)track->channels * sizeof(short));
    if (request_frames > requested_frames)
        request_frames = requested_frames;
    got = track->decoder->read_frames(output, static_cast<int>(request_frames));
    if (got > 0) {
        track->source_frame += (DWORD)got;
        if (track->source_frame <= track->loop_end_frame)
            return got;
        seek_frame = track->source_frame - track->loop_end_frame +
                     track->loop_start_frame;
        if (!track->decoder->seek_frame(seek_frame)) {
            track->source_ended = 1;
            return got;
        }
        track->source_frame = seek_frame;
        kinoko_trace_i32("bgm:loop-seek-frame", (int32_t)seek_frame);
    }
    return got;
}

static DWORD kinoko_bgm_decode_chunk(BgmTrack *track,
                                     unsigned char *output, DWORD bytes)
{
    DWORD requested_frames;
    DWORD written_frames = 0;
    int channels;

    if (track == NULL || track->decoder.get() == NULL || output == NULL ||
        bytes == 0)
        return 0;
    channels = track->channels;
    if (channels <= 0 || channels > RETDEC_BGM_MAX_CHANNELS)
        return 0;
    /* The original 412240 supplies ov_read with (little-endian, 2 bytes,
       signed) and a 4096-byte limit. Let upstream perform its own PCM
       clipping/conversion; do not duplicate the codec's floating-point path. */
    requested_frames = bytes / ((DWORD)channels * sizeof(short));
    while (written_frames < requested_frames) {
        int got;
        short *destination = (short *)output +
            (size_t)written_frames * (size_t)channels;

        if (track->looping && track->loop_end_frame >
                track->loop_start_frame) {
            got = kinoko_bgm_decode_loop_frames(
                track, destination, requested_frames - written_frames);
        } else {
            got = track->decoder->read_frames(destination, static_cast<int>(
                (std::min<DWORD>)(requested_frames - written_frames, 4096u / (channels * sizeof(short)))));
        }
        if (got <= 0) {
            if (!track->looping || track->source_ended) {
                if (!track->source_ended)
                    kinoko_trace_i32("bgm:source-ended", (int32_t)track->handle);
                track->source_ended = 1;
                break;
            }
            if (!track->decoder->seek_frame(0)) {
                track->source_ended = 1;
                break;
            }
            track->source_frame = 0;
            continue;
        }
        written_frames += (DWORD)got;
    }
    return written_frames * (DWORD)channels * sizeof(short);
}

static int kinoko_bgm_fill_chunk(BgmTrack *track, DWORD bytes)
{
    DWORD decoded;

    if (track == NULL || track->decoded_samples.get() == NULL || bytes == 0 ||
        bytes > RETDEC_BGM_CHUNK_BYTES)
        return 0;
    if (!kinoko_bgm_check_guard(track->decode_scratch.get(),
                                (RETDEC_BGM_CHUNK_BYTES / sizeof(short)) *
                                RETDEC_BGM_MAX_CHANNELS * sizeof(short)) ||
        !kinoko_bgm_check_guard(track->decoded_samples.get(),
                                RETDEC_BGM_CHUNK_BYTES)) {
        kinoko_trace("bgm:decode-guard-before-failed");
        return 0;
    }
    decoded = kinoko_bgm_decode_chunk(track, (unsigned char *)
                                      track->decoded_samples.get(), bytes);
    if (decoded < bytes)
        memset((unsigned char *)track->decoded_samples.get() + decoded, 0,
               bytes - decoded);
    if (!kinoko_bgm_check_guard(track->decode_scratch.get(),
                                (RETDEC_BGM_CHUNK_BYTES / sizeof(short)) *
                                RETDEC_BGM_MAX_CHANNELS * sizeof(short)) ||
        !kinoko_bgm_check_guard(track->decoded_samples.get(),
                                RETDEC_BGM_CHUNK_BYTES)) {
        kinoko_trace("bgm:decode-guard-after-failed");
        return 0;
    }
#if defined(RETDEC_DIAGNOSTIC_NO_BGM_WRITE)
    kinoko_trace("bgm:write-skipped");
    return 1;
#else
    if (!kinoko_bgm_write_buffer(track, track->decoded_samples.get(), bytes))
        return 0;
    return 1;
#endif
}

static void kinoko_bgm_release_track_locked(void)
{
    kinoko_bgm_release_state(&g_retdec_bgm_track);
}

static void kinoko_bgm_release_track(void)
{
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_release_track_locked();

}

static void kinoko_bgm_release_all_tracks_locked(void)
{
    int index;

    kinoko_bgm_release_state(&g_retdec_bgm_track);
    for (auto& track : fading_tracks) kinoko_bgm_release_state(&track);
    fading_tracks.clear();
    active_bgm_handle() = 0;
}

static void kinoko_bgm_release_all_tracks(void)
{
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_release_all_tracks_locked();

}

static void kinoko_se_entries_release() {
    for (int index = 0; index < g_retdec_se_entry_count; ++index) {
        auto& entry = g_retdec_se_entries[index];
        if (entry.buffer) entry.buffer->Stop();
        entry.buffer.reset();
        entry.buffer_bytes = 0;
    }
    for (auto& entry : g_retdec_se_entries) entry = SoundEntry{};
    g_retdec_se_entry_count = 0;
}

static void kinoko_se_pool_release() {
    kinoko_se_entries_release();
    for (auto& slot : g_retdec_se_pool.stream_slots) slot = SoundSlot{};
    g_retdec_se_pool.initialized = 0;
}

static int kinoko_se_parse_wave_asset(const char *path,
                                      WAVEFORMATEX *format,
                                      unsigned char **samples,
                                      DWORD *sample_bytes)
{
    if (!path || !format || !samples || !sample_bytes) return 0;
    *samples = nullptr;
    *sample_bytes = 0;
    ZeroMemory(format, sizeof(*format));
    unsigned char *raw = nullptr;
    DWORD size = 0;
    if (!kinoko_read_asset_bytes(path, &raw, &size)) return 0;
    kinoko::legacy::Allocation<unsigned char> data(raw);

    const size_t path_length = std::strlen(path);
    if (path_length >= 4 && _stricmp(path + path_length - 4, ".cv3") == 0) {
        // Packed SE: WAVEFORMATEX + DWORD byte count + PCM payload.
        if (size < 22) return 0;
        std::memcpy(format, data.get(), sizeof(*format));
        const DWORD payload_bytes = kinoko_bgm_read_u32(data.get() + 18);
        if (!payload_bytes || payload_bytes > size - 22 ||
            format->wFormatTag != 1 || !format->nChannels ||
            !format->nSamplesPerSec || !format->nBlockAlign ||
            !format->wBitsPerSample) return 0;
        *samples = static_cast<unsigned char *>(std::malloc(payload_bytes));
        if (!*samples) return 0;
        std::memcpy(*samples, data.get() + 22, payload_bytes);
        *sample_bytes = payload_bytes;
        return 1;
    }

    // Loose RIFF/WAVE allows metadata chunks between fmt and data. A malformed
    // chunk aborts rather than looking for a later data chunk.
    if (size < 12 || std::memcmp(data.get(), "RIFF", 4) != 0 ||
        std::memcmp(data.get() + 8, "WAVE", 4) != 0) return 0;
    DWORD offset = 12;
    bool have_format = false;
    while (offset <= size && size - offset >= 8) {
        const unsigned char *chunk = data.get() + offset;
        const DWORD chunk_bytes = kinoko_bgm_read_u32(chunk + 4);
        const DWORD available = size - offset - 8;
        if (chunk_bytes > available) return 0;
        if (std::memcmp(chunk, "fmt ", 4) == 0 && chunk_bytes >= 16) {
            ZeroMemory(format, sizeof(*format));
            std::memcpy(format, chunk + 8,
                        chunk_bytes >= sizeof(*format) ? sizeof(*format) : chunk_bytes);
            have_format = format->wFormatTag == 1 && format->nChannels &&
                format->nSamplesPerSec && format->nBlockAlign && format->wBitsPerSample;
        } else if (std::memcmp(chunk, "data", 4) == 0 && have_format) {
            *samples = static_cast<unsigned char *>(std::malloc(chunk_bytes));
            if (!*samples) return 0;
            std::memcpy(*samples, chunk + 8, chunk_bytes);
            *sample_bytes = chunk_bytes;
            return chunk_bytes != 0;
        }
        offset += 8 + chunk_bytes + (chunk_bytes & 1u);
    }
    return 0;
}

static int kinoko_se_replace_extension(const char *source, char *path,
                                       size_t path_size)
{
    size_t length;

    if (source == NULL || path == NULL || path_size == 0)
        return 0;
    length = strlen(source);
    if (length + 1 > path_size)
        return 0;
    memcpy(path, source, length + 1);
    if (packed_sound_assets() && length >= 4 &&
        _stricmp(path + length - 4, ".wav") == 0) {
        path[length - 3] = 'c';
        path[length - 2] = 'v';
        path[length - 1] = '3';
    }
    return 1;
}

static int kinoko_se_load_entry(int id, const char *source)
{
    char path[MAX_PATH];
    WAVEFORMATEX format;
    unsigned char *samples = NULL;
    DWORD sample_bytes = 0;
    IDirectSoundBuffer *buffer = NULL;

    HRESULT hr;
    int index;

    if (source == NULL || id < 0 ||
        !kinoko_se_replace_extension(source, path, sizeof(path)) ||
        !kinoko_se_parse_wave_asset(path, &format, &samples, &sample_bytes)) {
        kinoko_trace_i32("loadse:entry-failed", id);
        return 0;
    }
    if (!kinoko_create_secondary_buffer(&format, sample_bytes, &buffer) ||
        !kinoko_fill_dsound_buffer(buffer, samples, sample_bytes)) {
        kinoko_release_dsound_buffer(buffer);
        free(samples);
        kinoko_trace_i32("loadse:buffer-failed", id);
        return 0;
    }
    free(samples);
    kinoko_set_dsound_volume(buffer, g_retdec_se_pool.master_volume);

    hr = (buffer)->SetCurrentPosition(0);
    if (FAILED(hr)) {
        kinoko_release_dsound_buffer(buffer);
        return 0;
    }

    for (index = 0; index < g_retdec_se_entry_count; ++index) {
        if (g_retdec_se_entries[index].id == id) {
            g_retdec_se_entries[index].buffer.reset(buffer);
            g_retdec_se_entries[index].buffer_bytes = sample_bytes;
            return 1;
        }
    }
    if (g_retdec_se_entry_count >= RETDEC_SE_MAX_ENTRIES) {
        kinoko_release_dsound_buffer(buffer);
        return 0;
    }
    g_retdec_se_entries[g_retdec_se_entry_count].id = id;
    g_retdec_se_entries[g_retdec_se_entry_count].buffer.reset(buffer);
    g_retdec_se_entries[g_retdec_se_entry_count].buffer_bytes = sample_bytes;
    ++g_retdec_se_entry_count;
    kinoko_trace_i32("loadse:entry-loaded", id);
    return 1;
}

static int kinoko_se_pool_initialize(void)
{
    WAVEFORMATEX format;
    int index;
    int created = 0;

    if (g_retdec_se_pool.initialized)
        return 1;
    g_retdec_se_pool = SoundPool{};
    ZeroMemory(&format, sizeof(format));
    format.wFormatTag = 1;
    format.nChannels = 1;
    format.nSamplesPerSec = 44100;
    format.nAvgBytesPerSec = 88200;
    format.nBlockAlign = 2;
    format.wBitsPerSample = 16;

    for (index = 0; index < 32; ++index) {
        if (kinoko_create_secondary_buffer(&format, 0x40000,
                                            g_retdec_se_pool.stream_slots[
                                                index].buffer.put())) {
            g_retdec_se_pool.stream_slots[index].buffer_bytes = 0x40000;
            g_retdec_se_pool.stream_slots[index].in_use = 0;
            ++created;
        }
    }
    g_retdec_se_pool.master_volume = 1.0f;
    g_retdec_se_pool.initialized = 1;
    kinoko_trace_i32("40b520:secondary-created", created);
    return created != 0;
}

static void kinoko_se_pool_set_volume(float gain)
{
    int index;

    if (gain < 0.0f)
        gain = 0.0f;
    if (gain > 1.0f)
        gain = 1.0f;
    g_retdec_se_pool.master_volume = gain;
    for (index = 0; index < 32; ++index)
        kinoko_set_dsound_volume(g_retdec_se_pool.stream_slots[index].buffer.get(),
                                 gain);
}

static void kinoko_bgm_apply_track_volume(float gain)
{
    int index;

    if (gain < 0.0f)
        gain = 0.0f;
    if (gain > 1.0f)
        gain = 1.0f;
    g_retdec_audio_master_volume = gain;
    kinoko_bgm_apply_state_volume(&g_retdec_bgm_track,
                                  g_retdec_bgm_track.volume);
    for (auto& track : fading_tracks)
        kinoko_bgm_apply_state_volume(&track, track.volume);
}

// 409C12..409CE6: ordinary fades never imply release, even at zero gain.
static void update_track_fade(BgmTrack& track, DWORD now) {
    if (!track.buffer || !track.fade_duration || track.retirement_requested ||
        track.start_time || now <= track.fade_started) return;
    const DWORD elapsed = now - track.fade_started;
    if (elapsed >= track.fade_duration) {
        kinoko_bgm_apply_state_volume(&track, track.fade_to);
        track.fade_duration = 0;
        track.retirement_requested = track.retire_after_fade;
    } else {
        const float ratio = static_cast<float>(elapsed) / track.fade_duration;
        kinoko_bgm_apply_state_volume(&track,
            track.fade_from + (track.fade_to - track.fade_from) * ratio);
    }
}
static void kinoko_bgm_update_fade_locked(void) {
    const DWORD now = timeGetTime();
    update_track_fade(g_retdec_bgm_track, now);
    for (auto& track : fading_tracks) update_track_fade(track, now);
}

void kinoko_bgm_update_fade(void)
{
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_update_fade_locked();

}

static void kinoko_bgm_begin_fade_locked(BgmTrack *track,
                                         DWORD duration, float target,
                                         DWORD start_delay, bool retire_after_fade)
{
    if (track == NULL || track->buffer.get() == NULL)
        return;
    // 4098AD changes gain immediately without replacing an existing envelope.
    if (duration == 0) {
        kinoko_bgm_apply_state_volume(track, target);
        return;
    }
    track->retire_after_fade = retire_after_fade;
    track->fade_from = track->volume;
    track->fade_to = target;
    track->fade_started = timeGetTime() + start_delay;
    track->fade_duration = duration;
}

static void kinoko_bgm_begin_fade(DWORD duration, float target)
{
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_update_fade_locked();
    kinoko_bgm_begin_fade_locked(&g_retdec_bgm_track, duration, target, 0);

}

static void kinoko_bgm_begin_fade_for_handle(uint32_t handle,
                                             DWORD duration,
                                             DWORD start_delay,
                                             float target, bool retire_after_fade)
{
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_update_fade_locked();
    kinoko_bgm_begin_fade_locked(kinoko_bgm_find_track(handle), duration,
                                 target, start_delay, retire_after_fade);

}

// Original 40A8D0 toggles the hardware state without rewinding the ring.
static void kinoko_bgm_toggle_pause(uint32_t handle)
{
    CriticalLock lock(&audio_workers.lock);
    auto* track = kinoko_bgm_find_track(handle);
    if (!track || !track->buffer.get()) return;
    DWORD status = 0;
    track->buffer.get()->GetStatus(&status);
    if (status & DSBSTATUS_PLAYING) {
        if (SUCCEEDED(track->buffer.get()->Stop())) track->playing = 0;
    } else if (SUCCEEDED(track->buffer.get()->Play(0, 0, DSBPLAY_LOOPING))) {
        track->started = 1;
        track->playing = 1;
    }
}

static void kinoko_bgm_stop_for_handle(uint32_t handle)
{
    BgmTrack *track;


    CriticalLock lock(&audio_workers.lock);
    track = kinoko_bgm_find_track(handle);
    if (track != NULL && track->buffer.get() != NULL) {

                    (track->buffer.get())->Stop();
                    (track->buffer.get())->SetCurrentPosition(0);
        track->started = 0;
        track->playing = 0;
        track->play_offset = 0;
    }

}

static void kinoko_bgm_release_for_handle(uint32_t handle)
{
    BgmTrack *track;

    CriticalLock lock(&audio_workers.lock);
    track = kinoko_bgm_find_track(handle);
    if (track != NULL) track->retirement_requested = true;
    retire_playback_request(handle);
    if (handle == active_bgm_handle())
        active_bgm_handle() = 0;

}

static int kinoko_bgm_prepare_track_default_math(uint32_t handle, const char *path,
                                    int looping, float32_t volume)
{
    unsigned char *encoded = NULL;
    DWORD encoded_size = 0;
    std::unique_ptr<VorbisDecoder> decoder;
    VorbisDecoder::Format info;
    int error = 0;
    BgmTrack *track = &g_retdec_bgm_track;
    IDirectSoundBuffer *buffer;
    unsigned char *scratch = NULL;
    short *decoded_scratch = NULL;
    WAVEFORMATEX format;

    kinoko_trace_audio_text("bgm:prepare-path", path);
    kinoko_trace_i32("bgm:prepare-handle", (int32_t)handle);
    if (path == NULL || !g_audio_device.device || handle == 0 ||
        track->buffer.get() != NULL)
        return 0;
    if (!kinoko_read_asset_bytes(path, &encoded, &encoded_size)) {
        kinoko_trace("bgm:asset-read-failed");
        return 0;
    }
    kinoko_trace_i32("bgm:encoded-bytes", (int32_t)encoded_size);
    decoder = VorbisDecoder::open(encoded, encoded_size, error);
    if (decoder == NULL) {
        free(encoded);
        kinoko_trace_i32("audio:vorbis-open-error", error);
        return 0;
    }
    info = decoder->format();
    kinoko_trace_i32("bgm:sample-rate", (int32_t)info.sample_rate);
    kinoko_trace_i32("bgm:channels", (int32_t)info.channels);
    if (info.sample_rate != 44100 || info.channels <= 0 ||
        info.channels > RETDEC_BGM_MAX_CHANNELS) {
        decoder.reset();
        free(encoded);
        kinoko_trace("audio:unsupported-vorbis-format");
        return 0;
    }

    ZeroMemory(&format, sizeof(format));
    format.wFormatTag = 1;
    format.nChannels = (WORD)info.channels;
    format.nSamplesPerSec = info.sample_rate;
    format.nBlockAlign = (WORD)(info.channels * sizeof(short));
    format.nAvgBytesPerSec = info.sample_rate * format.nBlockAlign;
    format.wBitsPerSample = 16;
    if (!kinoko_create_secondary_buffer(&format, RETDEC_BGM_BUFFER_BYTES,
                                        &buffer)) {
        kinoko_trace("bgm:buffer-create-failed");
        decoder.reset();
        free(encoded);
        return 0;
    }
    scratch = (unsigned char *)malloc(
        (RETDEC_BGM_CHUNK_BYTES / sizeof(short)) *
        RETDEC_BGM_MAX_CHANNELS * sizeof(short) + RETDEC_BGM_GUARD_BYTES);
    if (scratch != NULL)
        decoded_scratch = (short *)malloc(RETDEC_BGM_CHUNK_BYTES +
                                          RETDEC_BGM_GUARD_BYTES);
    if (scratch == NULL || decoded_scratch == NULL) {
        kinoko_trace("bgm:scratch-alloc-failed");
        kinoko_release_dsound_buffer(buffer);
        free(scratch);
        free(decoded_scratch);
        decoder.reset();
        free(encoded);
        return 0;
    }
    kinoko_bgm_write_guard(
        scratch,
        (RETDEC_BGM_CHUNK_BYTES / sizeof(short)) *
        RETDEC_BGM_MAX_CHANNELS * sizeof(short));
    kinoko_bgm_write_guard(decoded_scratch, RETDEC_BGM_CHUNK_BYTES);
    *track = BgmTrack{};
    track->handle = handle;
    track->buffer.reset(buffer);
    track->buffer_bytes = RETDEC_BGM_BUFFER_BYTES;
    track->encoded_data.reset(encoded);
    track->encoded_bytes = encoded_size;
    track->decode_scratch.reset((short *)scratch);
    track->decoded_samples.reset(decoded_scratch);
    track->decoder = std::move(decoder);
    track->sample_rate = info.sample_rate;
    track->channels = (WORD)info.channels;
    track->looping = looping != 0;
    /* 412203 always loads SFL markers, overriding the whole-file fallback
       selected by the PlayBgm argument, including when that argument is 0. */
    if (kinoko_bgm_read_loop_points(path, &track->loop_start_frame,
                                    &track->loop_end_frame))
        track->looping = 1;
    kinoko_trace_i32("bgm:looping", track->looping);
    kinoko_trace_i32("bgm:loop-start-frame", (int32_t)track->loop_start_frame);
    kinoko_trace_i32("bgm:loop-end-frame", (int32_t)track->loop_end_frame);
    track->volume = volume;
    track->write_offset = 0;
    track->write_window_start = 0;
    track->play_offset = 0;
    track->buffered_bytes = 0;

    /* 4096D0 creates the 1 MiB stream buffer and 4099C0 supplies one 0x8000
       byte block before playback.  The worker keeps the ring filled after
       that initial block. */
    kinoko_trace("bgm:initial-fill");
    if (!kinoko_bgm_fill_chunk(track, RETDEC_BGM_CHUNK_BYTES)) {
        kinoko_trace("bgm:initial-fill-failed");
        kinoko_bgm_release_track_locked();
        return 0;
    }
    track->buffered_bytes = RETDEC_BGM_CHUNK_BYTES;
    kinoko_bgm_apply_state_volume(track, track->volume);
    kinoko_trace("bgm:prepared");
    return 1;
}

static int kinoko_bgm_prepare_track(uint32_t handle, const char *path,
                                    int looping, float32_t volume)
{
    return kinoko_prepare_audio(kinoko_bgm_prepare_track_default_math,
                                handle, path, looping, volume);
}

static void retire_playback_request(uint32_t handle) {
    auto& manager = g_retdec_audio_manager_state;
    if (!kinoko_audio_handle_lookup(&manager.handles, handle)) return;
    if (manager.active.head) manager.active.head->remove(handle);
    if (manager.retired.head && std::find(manager.retired.head->begin(),
            manager.retired.head->end(), handle) == manager.retired.head->end())
        manager.retired.head->push_back(handle);
    audio_workers.notify_loader();
}

static void release_retired_requests_locked() {
    auto& manager = g_retdec_audio_manager_state;
    uint32_t handle;
    while (kinoko_audio_list_pop(manager.retired.head, &handle)) {
        if (auto* track = kinoko_bgm_find_track(handle)) kinoko_bgm_release_state(track);
        if (kinoko_audio_handle_lookup(&manager.handles, handle)) {
            manager.handles.storage->buffers[handle & 0xffffu].reset();
            manager.handles.live_handles->remove(handle);
            --manager.handles.live_count;
        }
        fading_tracks.remove_if([](const BgmTrack& track) { return !track.buffer; });
        if (static_cast<uint32_t>(active_bgm_handle()) == handle) active_bgm_handle() = 0;
    }
}

static void kinoko_bgm_process_pending_locked(void)
{
    auto* list = g_retdec_audio_manager_state.pending.head;
    std::uint32_t handle;

    while (kinoko_audio_list_pop(list, &handle)) {
        BufferRecord* buffer = kinoko_audio_handle_lookup(
            &kinoko_audio_manager_this()->handles, (uint32_t)handle);
        const char *path;
        float volume;
        int looping;
        BgmTrack *track;

        if (buffer == 0)
            continue;
        path = kinoko_audio_buffer_path(buffer);
        kinoko_trace_audio_text("bgm:queued-path", path);
        volume = buffer->volume;
        looping = buffer->looping;
        if (path == NULL)
            continue;

        kinoko_bgm_update_fade_locked();
        kinoko_bgm_archive_current_track();
        if (!kinoko_bgm_prepare_track((uint32_t)handle, path, looping, volume)) {
            retire_playback_request(handle);
            continue;
        }
        buffer->ready = 1;
        track = &g_retdec_bgm_track;
        track->start_time = buffer->start_time;
        kinoko_audio_list_push(
            g_retdec_audio_manager_state.active.head, handle);
    }
}

static void kinoko_bgm_start_track(BgmTrack *track)
{

    HRESULT hr;

    if (track == NULL || track->buffer.get() == NULL || track->started)
        return;
            (track->buffer.get())->SetCurrentPosition(0);
    kinoko_trace("bgm:play");
    hr = (track->buffer.get())->Play(0, 0, 1);
    kinoko_trace_hresult("audio:stream-play-hr", hr);
    if (SUCCEEDED(hr)) {
        track->started = 1;
        track->playing = 1;
        track->play_offset = 0;
    }
}

static void kinoko_bgm_service_track(BgmTrack *track)
{

    DWORD play_cursor;
    DWORD write_cursor;
    DWORD consumed;
    DWORD write_boundary;
    DWORD write_limit;
    int in_write_window;

    if (track == NULL || track->buffer.get() == NULL)
        return;
    /* 409A11 stops a non-looping stream on the update after a short read.
       Waiting for the ring to drain can let the hardware read stale PCM. */
    if (track->source_ended && !track->looping) {
        kinoko_trace_i32("bgm:stop-at-eof", (int32_t)track->handle);
        track->retirement_requested = true;
        return;
    }
    if (!track->started) {
        const DWORD now = timeGetTime();
        if (track->start_time == 0 || now > track->start_time) {
            kinoko_bgm_start_track(track);
            track->start_time = 0;
            if (track->fade_started < now) track->fade_started = now;
        }
        if (!track->started)
            return;
    }
    play_cursor = 0;
    write_cursor = 0;
    if (FAILED((track->buffer.get())->GetCurrentPosition(&play_cursor, &write_cursor)))
        return;
    if (track->started && track->play_offset == 0) {
        kinoko_trace_i32("bgm:play-cursor", (int32_t)play_cursor);
        kinoko_trace_i32("bgm:write-cursor", (int32_t)write_cursor);
    }
    play_cursor &= track->buffer_bytes - 1;
    write_cursor &= track->buffer_bytes - 1;
    consumed = play_cursor >= track->play_offset
        ? play_cursor - track->play_offset
        : track->buffer_bytes - track->play_offset + play_cursor;
    if (consumed > track->buffered_bytes)
        consumed = track->buffered_bytes;
    track->buffered_bytes -= consumed;
    track->play_offset = play_cursor;
    /* 4099C0 gates the next block with DirectSound's write cursor.  The two
       fields at 1340/1344 are the next and previous 0x8000-byte boundaries;
       advancing the previous boundary is what prevents a fast worker from
       repeatedly overwriting data ahead of the hardware cursor. */
    in_write_window = 0;
    if (write_cursor >= track->write_window_start) {
        write_limit = track->write_offset;
        if (write_limit <= track->write_window_start)
            write_limit += track->buffer_bytes;
        if (write_cursor < write_limit)
            in_write_window = 1;
    }
    if (in_write_window &&
        !(track->source_ended && !track->looping)) {
        write_boundary = track->write_offset;
        if (!kinoko_bgm_fill_chunk(track, RETDEC_BGM_CHUNK_BYTES))
            return;
        kinoko_trace_i32("bgm:service-filled", (int32_t)track->handle);
        track->write_window_start = write_boundary;
        track->buffered_bytes += RETDEC_BGM_CHUNK_BYTES;
    }
}

static void kinoko_bgm_service_all_locked(void)
{
#if !defined(RETDEC_DIAGNOSTIC_NO_BGM_SERVICE)
    kinoko_bgm_update_fade_locked();
    auto* active = g_retdec_audio_manager_state.active.head;
    if (!active) return;
    // 40AAE0 traverses active handles, not every slot in the backing store.
    for (auto cursor = active->begin(); cursor != active->end();) {
        const auto handle = *cursor++;
        auto* track = kinoko_bgm_find_track(handle);
        if (!track) continue;
        if (!track->retirement_requested) kinoko_bgm_service_track(track);
        if (track->retirement_requested) retire_playback_request(handle);
    }
#endif
}

static void kinoko_bgm_archive_current_track(void) {
    if (!g_retdec_bgm_track.buffer) return;
    // The BGM manager uses handle lists. The 32 fixed slots belong to the SE
    // pool, not to BGM: never evict an unrelated fading stream at slot 32.
    fading_tracks.emplace_back(std::move(g_retdec_bgm_track));
    g_retdec_bgm_track = BgmTrack{};
}

static void kinoko_bgm_stop(int reset_position)
{


    if (g_retdec_bgm_track.buffer.get() == NULL)
        return;

            (g_retdec_bgm_track.buffer.get())->Stop();
    if (reset_position)
        (g_retdec_bgm_track.buffer.get())->SetCurrentPosition(0);
    g_retdec_bgm_track.playing = 0;
}

static ManagerRecord* kinoko_audio_manager_this() noexcept {
    return &g_retdec_audio_manager_state;
}


static bool kinoko_audio_manager_list_init(QueueRecord& queue) {
    try { queue.head=new HandleQueue;return true; } catch(const std::bad_alloc&) { return false; }
}
static void kinoko_audio_list_push(HandleQueue* list, std::uint32_t value) {
    if(list) list->push_back(value);
}
static bool kinoko_audio_list_pop(HandleQueue* list,std::uint32_t* value) {
    if(!list || list->empty()) return false;
    if(value) *value=list->front();list->pop_front();return true;
}

static const char* kinoko_audio_buffer_path(const BufferRecord* buffer) noexcept {
    return buffer ? buffer->path.c_str() : nullptr;
}


static void kinoko_audio_buffer_initialize(BufferRecord* buffer) noexcept {
    buffer->gain = 1.0f;
    buffer->playback_state = 3;
}


static BufferRecord* kinoko_audio_handle_lookup(HandleTable* manager,
                                               std::uint32_t handle) {
    if (!manager) return nullptr;
    const auto index = handle & 0xffffu;
    const auto generation = handle >> 16;
    if(!manager->storage || index>=manager->storage->buffers.size()) return nullptr;
    if(manager->storage->generations[index]!=generation) return nullptr;
    return manager->storage->buffers[index].get();
}

static bool kinoko_audio_handle_create(HandleTable* manager,std::uint32_t* output) {
    if(!manager || !output) return false;
    try {
        if(!manager->storage) manager->storage=new BufferStore;
        auto& store=*manager->storage;
        const size_t count=store.buffers.size();
        if(count>0xffffu) return false;
        auto buffer=std::make_unique<BufferRecord>();
        kinoko_audio_buffer_initialize(buffer.get());
        auto generation=(manager->next_generation+1)&0xffffu;
        if(!generation) generation=1;
        const uint32_t handle=static_cast<uint32_t>(count)|(generation<<16);
        store.buffers.reserve(count+1);store.generations.reserve(count+1);
        kinoko_audio_list_push(manager->live_handles,handle);
        store.generations.push_back(generation);store.buffers.push_back(std::move(buffer));
        manager->next_generation=generation;
        ++manager->live_count;*output=handle;return true;
    } catch(const std::bad_alloc&) { return false; }
}


static void kinoko_audio_manager_construct() {
    if (g_retdec_audio_manager_initialized) return;
    auto& state = g_retdec_audio_manager_state;
    state = ManagerRecord{};
    // Allocate every list before initializing either OS synchronization object.
    // A failed construction owns nothing and can be retried without leaks.
    QueueRecord handles{};
    if (!kinoko_audio_manager_list_init(handles) ||
        !kinoko_audio_manager_list_init(state.active) ||
        !kinoko_audio_manager_list_init(state.pending) ||
        !kinoko_audio_manager_list_init(state.retired)) {
        kinoko_audio_clear_queue(handles.head);
        kinoko_audio_clear_queue(state.active.head);
        kinoko_audio_clear_queue(state.pending.head);
        kinoko_audio_clear_queue(state.retired.head);
        state = ManagerRecord{};
        return;
    }
    const auto* symbols = kinoko_audio_host_symbols();
    state.handles.live_handles = handles.head;
    state.handles.lock_vtable = symbols->critical_section_vtable;
    state.lock_vtable = symbols->critical_section_vtable;
    InitializeCriticalSection(&state.lock);
    InitializeCriticalSection(&state.handles.lock);
    state.master_gain = state.stream_gain = 1.0f;
    g_retdec_audio_manager_initialized = 1;
}


static std::uint32_t* allocate_playback_handle(ManagerRecord* manager,
                                              std::uint32_t* output) {
    CriticalLock lock(&audio_workers.lock);
    if (!output) return nullptr;
    *output = 0;
    if (!g_retdec_audio_manager_initialized) kinoko_audio_manager_construct();
    if (g_retdec_audio_manager_initialized)
        kinoko_audio_handle_create(&manager->handles, output);
    return output;
}


static int32_t prepare_playback_request(ManagerRecord* this_ptr,
                                          int32_t handle,
                                          const char* source,
                                          int32_t buffer_flag,
                                          int32_t queue_mode,
                                          float32_t volume)
{
    CriticalLock lock(&audio_workers.lock);
    BufferRecord* buffer;
    const char *path = source;
    uint32_t path_length;
    float32_t gain;
    int prepared = 0;

    kinoko_trace_audio_text("470220:request-path", path);
    kinoko_trace_i32("470220:request-queue", queue_mode);
    if (this_ptr == 0 || source == 0) {
        return 1;
    }
    buffer = kinoko_audio_handle_lookup(&this_ptr->handles,
                                        (uint32_t)handle);
    kinoko_trace_i32("470220:handle", handle);
    kinoko_trace_i32("470220:buffer", address(buffer));
    if (buffer == 0) {
        return 1;
    }
    path_length = kinoko_safe_c_string_length(path);
    kinoko_trace_i32("470220:path-length", (int32_t)path_length);
    buffer->path.value->assign(path, path_length);
    kinoko_trace("470220:path-assigned");
    buffer->ready = 0;
    gain = this_ptr->stream_gain *
           this_ptr->master_gain;
    buffer->gain = gain;
    buffer->volume = volume;
    buffer->fade_to = gain;
    buffer->fade_from = gain;
    buffer->looping =
        (unsigned char)buffer_flag;
    buffer->fade_pending = 0;
    buffer->playback_state = 3;
    buffer->fade_started = 0;
    buffer->fade_duration = 0;
    buffer->successor = 0;

    if (queue_mode != 0) {
        kinoko_audio_list_push(
            g_retdec_audio_manager_state.pending.head, handle);
        if (audio_workers.queue_event.get() != NULL)
            SetEvent(audio_workers.queue_event.get());
        return 1;
    }

    /* a5 == 0 is the original synchronous path: prepare the new buffer
       before it is inserted into the active list. */
    kinoko_bgm_update_fade_locked();
    kinoko_bgm_archive_current_track();
    prepared = kinoko_bgm_prepare_track(
        (uint32_t)handle, kinoko_audio_buffer_path(buffer),
        buffer_flag, volume);
    if (prepared) {
        buffer->ready = 1;
        kinoko_audio_list_push(
            g_retdec_audio_manager_state.active.head, handle);
        kinoko_trace("470220:prepared-sync");
    } else {
        retire_playback_request(handle);
        kinoko_trace("470220:prepare-failed");
    }

    return 1;
}

static int32_t schedule_playback_start(ManagerRecord* this_ptr,
                                          int32_t handle,
                                          int32_t delay)
{
    CriticalLock lock(&audio_workers.lock);
    BufferRecord* buffer;
    BgmTrack *track;
    DWORD start_time;

    if (this_ptr == 0) {
        return 0;
    }
    kinoko_trace_i32("470220:start-delay", delay);
    buffer = kinoko_audio_handle_lookup(&this_ptr->handles,
                                        (uint32_t)handle);
    if (buffer == 0) {
        return 0;
    }
    track = kinoko_bgm_find_track((uint32_t)handle);
    buffer->playback_state = 0;
    if (delay != 0) {
        start_time = timeGetTime() + (uint32_t)delay;
        buffer->start_time = start_time;
        if (track != NULL)
            track->start_time = start_time;
    } else {
        if (track != NULL &&
            buffer->ready != 0) {
            track->start_time = 0;
            kinoko_bgm_start_track(track);
        } else {
            start_time = timeGetTime();
            buffer->start_time = start_time;
        }
    }

    return 1;
}

static int32_t fade_out_playback(ManagerRecord* this_ptr,
                                          int32_t handle,
                                          int32_t duration,
                                          int32_t start_delay,
                                          float32_t target)
{
    (void)this_ptr;
    (void)target;
    kinoko_bgm_begin_fade_for_handle((uint32_t)handle,
                                     duration > 0 ? (DWORD)duration : 0,
                                     static_cast<DWORD>(start_delay),
                                     0.0f, true);
    return 1;
}


HANDLE kinoko_audio_start_workers(void) {
    if (audio_workers.initialized)
        return audio_workers.queue_event.get();
    audio_workers.queue_event.reset(CreateEventA(NULL, FALSE, FALSE, NULL));
    audio_workers.stop_event.reset(CreateEventA(NULL, TRUE, FALSE, NULL));
    if (audio_workers.queue_event.get() == NULL ||
        audio_workers.stop_event.get() == NULL) {
        if (audio_workers.queue_event.get() != NULL)
            audio_workers.queue_event.reset();
        if (audio_workers.stop_event.get() != NULL)
            audio_workers.stop_event.reset();
        audio_workers.queue_event.reset(NULL);
        audio_workers.stop_event.reset(NULL);
        kinoko_trace("40a3d0:audio-events-failed");
        return 0;
    }
    InterlockedExchange(&audio_workers.running, 1);
    audio_workers.update_thread.reset(CreateThread(
        NULL, 0, audio_update_worker, NULL, 0, NULL));
    if (audio_workers.update_thread.get() != NULL)
        SetThreadPriority(audio_workers.update_thread.get(), 15);
    audio_workers.loader_thread.reset(CreateThread(
        NULL, 0, audio_loader_worker, NULL, 0, NULL));
    if (audio_workers.update_thread.get() == NULL ||
        audio_workers.loader_thread.get() == NULL) {
        audio_workers.stop_and_join();
        kinoko_trace("40a3d0:audio-threads-failed");
        return 0;
    }
    audio_workers.initialized = 1;
    kinoko_trace("40a3d0:audio-threads-ready");
    return audio_workers.queue_event.get();
}

int32_t kinoko_audio_stop_workers(void) {
    audio_workers.stop_and_join();
    // 40A460 joins before releasing active, pending and retired resources.
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_release_all_tracks_locked();
    auto& manager = g_retdec_audio_manager_state;
    for (auto* queue : {&manager.active, &manager.pending, &manager.retired}) {
        if (queue->head) queue->head->clear();
        queue->count = 0;
    }
    delete std::exchange(manager.handles.storage, nullptr);
    if (manager.handles.live_handles) manager.handles.live_handles->clear();
    manager.handles.live_count = 0;
    return 1;
}


int32_t kinoko_audio_set_bgm_volume(float gain) {
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_update_fade_locked();
    kinoko_bgm_apply_track_volume(gain);

    return 1;
}

int32_t run_audio_update_worker(void) {
    if (SUCCEEDED(CoInitialize(NULL))) {
        while (InterlockedCompareExchange(&audio_workers.running, 0, 0)) {
            if (audio_workers.stop_event.get() == NULL ||
                WaitForSingleObject(audio_workers.stop_event.get(), 16) ==
                    WAIT_OBJECT_0)
                break;
            service_audio_tick();
        }
        CoUninitialize();
    }
    return 0;
}

int32_t run_audio_loader_worker(void) {
    if (SUCCEEDED(CoInitialize(NULL))) {
        while (InterlockedCompareExchange(&audio_workers.running, 0, 0)) {
            HANDLE handles[2];
            DWORD wait_result;

            handles[0] = audio_workers.queue_event.get();
            handles[1] = audio_workers.stop_event.get();
            if (handles[0] == NULL || handles[1] == NULL)
                break;
            wait_result = WaitForMultipleObjects(2, handles, FALSE,
                                                 INFINITE);
            if (wait_result != WAIT_OBJECT_0)
                break;
            CriticalLock lock(&audio_workers.lock);
            if (audio_workers.is_running()) {
                kinoko_bgm_process_pending_locked();
                release_retired_requests_locked();
            }

        }
        CoUninitialize();
    }
    return 0;
}

int32_t service_audio_tick(void) {
    if (!InterlockedCompareExchange(&audio_workers.running, 0, 0))
        return 0;
    CriticalLock lock(&audio_workers.lock);
    kinoko_bgm_service_all_locked();
    return 0;
}


int32_t kinoko_audio_shutdown_resources(void) {
    kinoko_audio_stop_workers(); // join both workers before touching any owned resource
    kinoko_bgm_release_all_tracks();
    kinoko_se_pool_release();
    kinoko_audio_manager_destroy();
    return 1;
}

int32_t kinoko_audio_initialize_sound_pool(void) {
    return kinoko_se_pool_initialize();
}

int32_t kinoko_audio_set_sound_volume(float gain) {
    kinoko_se_pool_set_volume(gain);
    return 1;
}

int32_t kinoko_audio_initialize_device(HWND hwnd, int32_t options) {
    g_audio_device.reset();
    sync_audio_device_aliases();
    auto& owner = g_audio_device;
    kinoko_trace("411d80:pre-cocreate");
    // Use the SDK GUID objects, not the first DWORD of a split RetDec global.
    auto hr = CoCreateInstance(CLSID_DirectSound8, nullptr, CLSCTX_INPROC_SERVER,
        IID_IDirectSound8, reinterpret_cast<void**>(owner.device.put()));
    kinoko_trace_hresult("411d80:cocreate-hr", hr);
    bool initialized = false;
    if (FAILED(hr) || !owner.device) {
        kinoko_trace("411d80:pre-directsoundcreate8");
        owner.module = LoadLibraryA("dsound.dll");
        using Create = HRESULT (WINAPI*)(LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN);
        const auto create = owner.module ? reinterpret_cast<Create>(
            GetProcAddress(owner.module, "DirectSoundCreate8")) : nullptr;
        hr = create ? create(nullptr, owner.device.put(), nullptr) : E_FAIL;
        kinoko_trace_hresult("411d80:directsoundcreate8-hr", hr);
        initialized = SUCCEEDED(hr) && static_cast<bool>(owner.device);
    }
    if (FAILED(hr) || !owner.device) {
        MessageBoxA(nullptr, kinoko_audio_host_symbols()->device_error_message,
                    "DSound-Error", MB_OK);
        owner.reset();
        return 0;
    }
    if (!initialized) {
        kinoko_trace("411d80:pre-initialize");
        hr = owner.device->Initialize(nullptr);
        kinoko_trace_hresult("411d80:initialize-hr", hr);
        if (FAILED(hr)) { owner.reset(); return 0; }
    } else {
        kinoko_trace("411d80:initialize-skipped");
    }
    kinoko_trace("411d80:pre-cooperative-level");
    hr = owner.device->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
    if (FAILED(hr)) hr = owner.device->SetCooperativeLevel(hwnd, DSSCL_NORMAL);
    kinoko_trace_hresult("411d80:cooperative-level-hr", hr);
    if (FAILED(hr)) { owner.reset(); return 0; }
    DSCAPS caps{};
    caps.dwSize = sizeof(caps);
    owner.device->GetCaps(&caps);
    DSBUFFERDESC description{};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_LOCSOFTWARE;
    if (options & 1) description.dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME;
    kinoko_trace("411d80:pre-create-primary");
    hr = owner.device->CreateSoundBuffer(&description, owner.primary.put(), nullptr);
    kinoko_trace_hresult("411d80:create-primary-hr", hr);
    if (FAILED(hr) || !owner.primary) { owner.reset(); return 0; }
    if (options & 1) {
        hr = owner.primary->QueryInterface(IID_IDirectSound3DListener,
            reinterpret_cast<void**>(owner.listener.put()));
        kinoko_trace_hresult("411d80:listener-hr", hr);
        if (FAILED(hr) || !owner.listener) { owner.reset(); return 0; }
    }
    kinoko_trace("411d80:pre-play-primary");
    hr = owner.primary->Play(0, 0, DSBPLAY_LOOPING);
    kinoko_trace_hresult("411d80:play-primary-hr", hr);
    sync_audio_device_aliases();
    kinoko_trace("411d80:done");
    return 1;
}


int32_t kinoko_audio_shutdown_device(void) {
    const auto result = address(g_audio_device.device.get());
    kinoko_audio_shutdown_resources();
    // Shutdown has already joined the audio workers. Release the listener,
    // primary buffer and device in that order before unloading the module.
    g_audio_device.reset();
    sync_audio_device_aliases();
    return result;
}


int32_t kinoko_audio_initialize_playback(void) {
    kinoko_trace("4701e0:audio-begin");
    kinoko_audio_initialize_sound_pool();
    kinoko_audio_set_sound_volume(0.80000001L);
    kinoko_audio_start_workers();
    int32_t result = kinoko_audio_set_bgm_volume(0.80000001L);
    kinoko_trace("4701e0:audio-ready");
    return result;
}

int32_t kinoko_audio_play_bgm(const char* path, int32_t a2, int32_t a3, int32_t a4) {
    auto* manager = kinoko_audio_manager_this();
    std::uint32_t new_handle = 0;

    (void)a3;
    if (active_bgm_handle() != 0)
        fade_out_playback(manager, active_bgm_handle(), 1000, 0, 1.0f);
    allocate_playback_handle(manager, &new_handle);
    active_bgm_handle() = new_handle;
    /* 470257 forwards arg_C, the fourth argument, as the loop flag. */
    prepare_playback_request(manager, active_bgm_handle(), path, a4, 0, 1.0f);
    schedule_playback_start(manager, active_bgm_handle(), a2);
    return 0;
}

int32_t kinoko_audio_play_bgm_margin(const char* path, int32_t a2, int32_t a3, int32_t a4,
                        int32_t a5) {
    auto* manager = kinoko_audio_manager_this();
    std::uint32_t new_handle = 0;

    if (active_bgm_handle() != 0)
        fade_out_playback(manager, active_bgm_handle(), 1000, a3, 1.0f);
    allocate_playback_handle(manager, &new_handle);
    active_bgm_handle() = new_handle;
    prepare_playback_request(manager, active_bgm_handle(), path, a5, 0, 1.0f);
    schedule_playback_start(manager, active_bgm_handle(), a2);
    return 0;
}

int32_t kinoko_audio_pause_bgm(void) {
    if (active_bgm_handle() != 0) {
        kinoko_bgm_toggle_pause((uint32_t)active_bgm_handle());
    }
    return 0;
}

int32_t kinoko_audio_fade_bgm(int32_t a1, int32_t a2) {
    if (active_bgm_handle() != 0) {
        float target = (float)a2 / 100.0f;
        kinoko_bgm_begin_fade_for_handle((uint32_t)active_bgm_handle(),
                                         a1 > 0 ? (DWORD)a1 : 0, 0,
                                         target);
    }
    return 0;
}

int32_t kinoko_audio_stop_bgm(void) {
    if (active_bgm_handle() != 0) {
        uint32_t handle = (uint32_t)active_bgm_handle();
        kinoko_bgm_stop_for_handle(handle);
        kinoko_bgm_release_for_handle(handle);
    }
    return 0;
}

int32_t kinoko_audio_play_sound(int32_t id) {
    kinoko_trace_i32("470980:se-id", id);
    for (int index = 0; index < g_retdec_se_entry_count; ++index) {
        const auto& entry = g_retdec_se_entries[index];
        if (entry.id != id) continue;
        auto* buffer = entry.buffer.get();
        if (!buffer) return 0;
        DWORD status = 0;
        buffer->GetStatus(&status);
        if (status & DSBSTATUS_PLAYING) buffer->Stop();
        buffer->SetCurrentPosition(0);
        kinoko_trace_i32("470980:se-bytes", static_cast<int32_t>(entry.buffer_bytes));
        // No process-memory dump of a COM implementation on the sound path.
        const auto hr = buffer->Play(0, 0, 0);
        kinoko_trace_hresult("470980:play-hr", hr);
        return SUCCEEDED(hr) ? 1 : 0;
    }
    return 0;
}


static void kinoko_loadse_blob(const char *source)
{
    char path[MAX_PATH];
    size_t length;
    KinokoArchiveReader *reader_slot = nullptr;
    KinokoArchiveReader *reader;
    uint32_t size;
    unsigned char *blob;
    uint32_t index;
    unsigned char key = 0x8bu;
    unsigned char step = 0x71u;
    int loaded = 0;

    if (source == NULL)
        return;
    kinoko_se_entries_release();
    length = strlen(source);
    if (length < 4 || length + 1 > sizeof(path))
        return;
    memcpy(path, source, length + 1);
    if (_stricmp(path + length - 4, ".csv") == 0) {
        path[length - 2] = 'v';
        path[length - 1] = '1';
    } else if (_stricmp(path + length - 4, ".cv1") != 0) {
        return;
    }

    if (kinoko_reader_open(&reader_slot, path) == 0) {
        kinoko_trace("loadse:reader-failed");
        return;
    }
    reader = reader_slot;
    size = kinoko_reader_size(reader);
    kinoko_trace_squirrel_name("loadse:path", (int32_t)(intptr_t)path);
    kinoko_trace_i32("loadse:size", (int32_t)size);
    if (size == 0 || size > 16u * 1024u * 1024u) {
        kinoko_reader_close(reader);
        return;
    }
    blob = (unsigned char *)malloc(size + 1u);
    if (blob != NULL && kinoko_reader_read_exact(reader_slot, blob, size)) {
        /* This is the same rolling transform used by sub_414930 before the
           CSV stream is handed to the original line parser. */
        for (index = 0; index < size; ++index) {
            blob[index] ^= key;
            key = (unsigned char)(key + step);
            step = (unsigned char)(step - 0x6bu);
        }
        blob[size] = 0;
        kinoko_trace("loadse:blob-read");

        /* se.cv1 is a small CSV table.  Its quoted filename is passed to
           CWaveBuffer::Load one row at a time, which creates the actual
           secondary buffer used later by PlaySE. */
        {
            char *cursor = (char *)blob;
            char *end = cursor + size;
            while (cursor < end) {
                char *line_end = (char *)memchr(cursor, '\n',
                                                (size_t)(end - cursor));
                char *field;
                char *path_start;
                char *path_end;
                char saved;
                char *number_end;
                long id;

                if (line_end == NULL)
                    line_end = end;
                *line_end = 0;
                field = cursor;
                while (*field == ' ' || *field == '\t' || *field == '\r')
                    ++field;
                if (*field != 0 && *field != '#') {
                    id = strtol(field, &number_end, 10);
                    if (number_end != field && id >= 0 && id <= 0x7fffffffL) {
                        field = number_end;
                        while (*field == ' ' || *field == '\t')
                            ++field;
                        if (*field == ',') {
                            ++field;
                            while (*field == ' ' || *field == '\t')
                                ++field;
                            if (*field == '"')
                                ++field;
                            path_start = field;
                            path_end = path_start;
                            while (*path_end != 0 && *path_end != '"' &&
                                   *path_end != '\r')
                                ++path_end;
                            saved = *path_end;
                            *path_end = 0;
                            if (*path_start != 0 &&
                                kinoko_se_load_entry((int)id, path_start))
                                ++loaded;
                            *path_end = saved;
                        }
                    }
                }
                if (line_end == end)
                    break;
                cursor = line_end + 1;
            }
        }
        kinoko_trace_i32("loadse:entries-loaded", loaded);
    } else {
        kinoko_trace("loadse:blob-read-failed");
    }
    free(blob);
    kinoko_reader_close(reader);
}

int32_t kinoko_audio_load_sound_table(const char* path) {
    /* LoadSE builds the same ID -> CDSBuffer table that PlaySE consumes. */
    kinoko_loadse_blob(path);
    return 0;
}

namespace {
// Constructed last and destroyed first: workers cannot race member destruction.
struct AudioRuntimeShutdown {
    ~AudioRuntimeShutdown() {
        kinoko_audio_shutdown_resources();
    }
} audio_runtime_shutdown;
}
