#pragma once
// Native SDK interfaces keep the test double ABI identical to production.
#include <windows.h>
#include <dsound.h>
#include <algorithm>
#include <vector>

class AudioTestBuffer final : public IDirectSoundBuffer {
public:
    ULONG refs = 1;
    int releases = 0, stops = 0, plays = 0, locks = 0, unlocks = 0;
    DWORD status = 0, position = 0;
    LONG volume = 0;
    bool fail_lock = false;
    std::vector<unsigned char> bytes = std::vector<unsigned char>(16);
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { ++releases; return --refs; }
    HRESULT STDMETHODCALLTYPE GetCaps(LPDSBCAPS) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetCurrentPosition(LPDWORD play, LPDWORD write) override {
        if (play) *play = position; if (write) *write = position; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetFormat(LPWAVEFORMATEX, DWORD, LPDWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetVolume(LPLONG value) override { *value = volume; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetPan(LPLONG) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetFrequency(LPDWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetStatus(LPDWORD value) override { *value = status; return S_OK; }
    HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTSOUND, LPCDSBUFFERDESC) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Lock(DWORD offset, DWORD size, LPVOID* first, LPDWORD first_size,
        LPVOID* second, LPDWORD second_size, DWORD) override {
        ++locks;
        if (fail_lock) return E_FAIL;
        if (offset >= bytes.size() || size > bytes.size()) return E_INVALIDARG;
        *first_size = (std::min)(size, static_cast<DWORD>(bytes.size()) - offset);
        *second_size = size - *first_size;
        *first = bytes.data() + offset;
        *second = *second_size ? bytes.data() : nullptr;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Play(DWORD, DWORD, DWORD) override { ++plays; status = DSBSTATUS_PLAYING; return S_OK; }
    HRESULT STDMETHODCALLTYPE SetCurrentPosition(DWORD value) override { position = value; return S_OK; }
    HRESULT STDMETHODCALLTYPE SetFormat(LPCWAVEFORMATEX) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetVolume(LONG value) override { volume = value; return S_OK; }
    HRESULT STDMETHODCALLTYPE SetPan(LONG) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetFrequency(DWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Stop() override { ++stops; status = 0; return S_OK; }
    HRESULT STDMETHODCALLTYPE Unlock(LPVOID, DWORD, LPVOID, DWORD) override { ++unlocks; return S_OK; }
    HRESULT STDMETHODCALLTYPE Restore() override { return S_OK; }
};
