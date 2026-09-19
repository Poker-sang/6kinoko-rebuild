// The stage host still compiles as C. This test-only translation unit builds
// the actual audio implementation once so fixtures can access its private
// owners without exporting globals or adding injection paths to the game.
#include "../src/reconstructed/audio_runtime.cpp"
#include "stage_audio_contract.h"
#include "kinoko/game_math.h"
#include "directsound_fixture.hpp"
#include <array>
#include <cstdio>
#include <float.h>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "stage audio line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

namespace {
class Device final : public IDirectSound8 {
public:
    AudioTestBuffer buffer;
    ULONG references = 1;
    bool fail_create = false;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** result) override {
        if (result) *result = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override { return --references; }
    HRESULT STDMETHODCALLTYPE CreateSoundBuffer(LPCDSBUFFERDESC description,
        LPDIRECTSOUNDBUFFER* result, LPUNKNOWN) override {
        *result = nullptr;
        if (fail_create) return E_FAIL;
        if (!description || description->dwSize != sizeof(DSBUFFERDESC)) return E_INVALIDARG;
        buffer.bytes.resize(description->dwBufferBytes);
        buffer.AddRef();
        *result = &buffer;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetCaps(LPDSCAPS) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER,
        LPDIRECTSOUNDBUFFER*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE Compact() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetSpeakerConfig(LPDWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetSpeakerConfig(DWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Initialize(LPCGUID) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE VerifyCertification(LPDWORD) override { return E_NOTIMPL; }
};

// Failure paths must restore the real device and close every decoder before
// stack-based test buffers disappear. A fixture device supplies one extra ref.
class DeviceFixture final {
    kinoko::ComOwner<IDirectSound8> previous_;
public:
    explicit DeviceFixture(IDirectSound8& device)
        : previous_(std::move(g_audio_device.device)) {
        device.AddRef();
        g_audio_device.device.reset(&device);
    }
    ~DeviceFixture() {
        retdec_bgm_release_all_tracks_locked();
        g_audio_device.device = std::move(previous_);
    }
    DeviceFixture(const DeviceFixture&) = delete;
    DeviceFixture& operator=(const DeviceFixture&) = delete;
};
struct SoundCleanupGuard {
    ~SoundCleanupGuard() { retdec_se_pool_release(); }
};
struct Module {
    HMODULE handle = LoadLibraryA("dsound.dll");
    ~Module() { if (handle) FreeLibrary(handle); }
};
}

extern "C" int kinoko_test_sound_cleanup(int32_t (*clear_all)(void)) {
    AudioTestBuffer buffers[3];
    SoundCleanupGuard cleanup;
    g_retdec_se_entry_count = 2;
    g_retdec_se_entries[0].buffer.reset(&buffers[0]);
    g_retdec_se_entries[1].buffer.reset(&buffers[1]);
    g_retdec_se_pool.stream_slots[0].buffer.reset(&buffers[2]);
    g_retdec_se_pool.initialized = 1;
    CHECK(clear_all() == 1);
    CHECK(!g_retdec_se_entry_count && !g_retdec_se_pool.initialized);
    CHECK(clear_all() == 1);
    for (const auto& buffer : buffers) CHECK(buffer.releases == 1 && buffer.refs == 0);
    CHECK(buffers[0].stops == 1 && buffers[1].stops == 1);
    return 0;
}

extern "C" int kinoko_test_bgm_preserves_game_math(void) {
    // Only DirectSound is replaced. DAT reads, the vendored Vorbis decoder,
    // SFL loop markers and the production initial ring fill remain real.
    Device device;
    DeviceFixture fixture(device);
    unsigned current = 0, x87 = 0, sse = 0;
    std::array<unsigned char, RETDEC_BGM_CHUNK_BYTES> reference{};
    _controlfp_s(&current, _RC_NEAR, _MCW_RC);
    CHECK(retdec_bgm_prepare_track(1, "data/bgm/st1.ogg", 1, 1.0f));
    std::memcpy(reference.data(), device.buffer.bytes.data(), reference.size());
    retdec_bgm_release_track_locked();
    CHECK(device.buffer.refs == 1);
    kinoko_enter_game_math();
    for (int failure = 0; failure <= 2; ++failure) {
        device.fail_create = failure == 1;
        device.buffer.fail_lock = failure == 2;
        CHECK(retdec_bgm_prepare_track(1, "data/bgm/st1.ogg", 1, 1.0f) == (failure == 0));
        CHECK(__control87_2(0, 0, &x87, &sse));
        CHECK((x87 & _MCW_RC) == _RC_UP && (sse & _MCW_RC) == _RC_UP);
        if (!failure) CHECK(std::memcmp(reference.data(), device.buffer.bytes.data(), reference.size()) == 0);
        retdec_bgm_release_track_locked();
        CHECK(device.buffer.refs == 1);
    }
    device.fail_create = device.buffer.fail_lock = false;
    CHECK(!retdec_bgm_prepare_track(1, "data/script/constant.cv4", 1, 1.0f));
    CHECK(__control87_2(0, 0, &x87, &sse));
    CHECK((x87 & _MCW_RC) == _RC_UP && (sse & _MCW_RC) == _RC_UP);
    std::puts("PASS: real BGM decoding preserves PCM and game rounding on success/decoder/device/fill failure");
    return 0;
}

extern "C" int kinoko_test_sound_module(void) {
    Module module;
    CHECK(module.handle);
    char path[MAX_PATH]{};
    CHECK(GetModuleFileNameA(module.handle, path, MAX_PATH));
    std::printf("module=%s base=%p\n", path, module.handle);
    using Create = HRESULT (WINAPI*)(LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN);
    const auto create = reinterpret_cast<Create>(GetProcAddress(module.handle, "DirectSoundCreate8"));
    CHECK(create);
    kinoko::ComOwner<IDirectSound8> device;
    CHECK(SUCCEEDED(create(nullptr, device.put(), nullptr)) && device);
    CHECK(SUCCEEDED(device->SetCooperativeLevel(GetDesktopWindow(), DSSCL_NORMAL)));
    WAVEFORMATEX format{WAVE_FORMAT_PCM, 1, 22050, 44100, 2, 16, 0};
    DSBUFFERDESC description{};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    description.dwBufferBytes = 4096;
    description.lpwfxFormat = &format;
    kinoko::ComOwner<IDirectSoundBuffer> buffer;
    CHECK(SUCCEEDED(device->CreateSoundBuffer(&description, buffer.put(), nullptr)) && buffer);
    std::printf("buffer=%p\n", buffer.get());
    return 0;
}
