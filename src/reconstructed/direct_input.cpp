#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
extern "C" {
extern char *g768, *g783, *g784;
extern void *g769, *g770, *g771;
extern int32_t g772,g773,g774,g782,g786,g787;
extern unsigned char g_retdec_keyboard_state[256];
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_hresult(const char*, long);
void retdec_poll_fallback_keyboard(void);
}
extern "C" int32_t function_408930(HWND hwnd, HINSTANCE instance) {
    void *direct_input = NULL;
    HRESULT hr;

    (void)instance;
    if (g769 != NULL) {
        return 1;
    }
    g768 = (char *)hwnd;
    retdec_trace("408930:pre-cocreate");
    hr = (HRESULT)CoCreateInstance(
        CLSID_DirectInput8, NULL, CLSCTX_INPROC_SERVER,
        IID_IDirectInput8A, &direct_input);
    retdec_trace(FAILED(hr) ? "408930:cocreate-failed" :
                 "408930:post-cocreate");
    if (FAILED(hr) || direct_input == NULL) {
        g769 = NULL;
        MessageBoxA(hwnd, "DirectInput8Create failed", "DInput-Error", MB_OK);
        return 0;
    }
    g769 = direct_input;
    return 1;
}

extern "C" int32_t function_4089c0(void) {
    IDirectInputDevice8A *mouse = (IDirectInputDevice8A *)g771;
    IDirectInputDevice8A *keyboard = (IDirectInputDevice8A *)g770;
    IDirectInput8A *direct_input = (IDirectInput8A *)g769;

    if (mouse != NULL) {
        mouse->Unacquire();
        mouse->Release();
        g771 = NULL;
    }
    if (keyboard != NULL) {
        keyboard->Unacquire();
        keyboard->Release();
        g770 = NULL;
    }
    if (g772 != 0) {
        free((void *)(intptr_t)g772);
        g772 = 0;
    }
    g773 = 0;
    g774 = 0;
    g782 = 0;
    g783 = NULL;
    g784 = NULL;
    g786 = 0;
    g787 = 0;
    if (direct_input != NULL) {
        direct_input->Release();
        g769 = NULL;
    }
    return 1;
}

extern "C" int32_t function_408b30(void) {
    IDirectInput8A *direct_input = (IDirectInput8A *)g769;
    IDirectInputDevice8A *keyboard = NULL;
    HRESULT hr;

    if (g770 != NULL) {
        return 1;
    }
    if (direct_input == NULL || g768 == NULL) {
        return 0;
    }
    retdec_trace("408b30:pre-create-device");
    hr = direct_input->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    retdec_trace_hresult("408b30:create-device-hr", hr);
    retdec_trace(FAILED(hr) ? "408b30:create-device-failed" :
                 "408b30:create-device-ok");
    if (FAILED(hr) || keyboard == NULL) {
        return 0;
    }
    retdec_trace("408b30:pre-data-format");
    hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
    retdec_trace(FAILED(hr) ? "408b30:data-format-failed" :
                 "408b30:data-format-ok");
    if (SUCCEEDED(hr)) {
        retdec_trace("408b30:pre-cooperative-level");
        hr = keyboard->SetCooperativeLevel((HWND)g768, 22);
        retdec_trace(FAILED(hr) ? "408b30:cooperative-level-failed" :
                     "408b30:cooperative-level-ok");
    }
    if (SUCCEEDED(hr)) {
        retdec_trace("408b30:pre-acquire");
        hr = keyboard->Acquire();
        retdec_trace(FAILED(hr) ? "408b30:acquire-failed" :
                     "408b30:acquire-ok");
    }
    if (FAILED(hr)) {
        keyboard->Release();
        return 0;
    }
    g770 = keyboard;
    return 1;
}

extern "C" int32_t function_408c80(void) {
    IDirectInputDevice8A *keyboard = (IDirectInputDevice8A *)g770;
    IDirectInputDevice8A *mouse = (IDirectInputDevice8A *)g771;
    HRESULT hr;
    unsigned char keyboard_state[256];
    DIMOUSESTATE2 mouse_state;
    static volatile LONG poll_trace_count;
    LONG poll_index = InterlockedIncrement(&poll_trace_count);

    if (poll_index <= 5)
        retdec_trace_i32("input:poll-keyboard", (int32_t)(intptr_t)keyboard);

    if (keyboard != NULL) {
        hr = keyboard->GetDeviceState((DWORD)sizeof(keyboard_state), keyboard_state);
        if (FAILED(hr)) {
            keyboard->Acquire();
            ZeroMemory(keyboard_state, sizeof(keyboard_state));
        }
        /* The original consumers read this fixed 256-byte state area. */
        memcpy(g_retdec_keyboard_state, keyboard_state,
               sizeof(g_retdec_keyboard_state));
    } else {
        retdec_poll_fallback_keyboard();
    }
    if (mouse != NULL) {
        hr = mouse->GetDeviceState((DWORD)sizeof(mouse_state), &mouse_state);
        if (FAILED(hr)) {
            mouse->Acquire();
            ZeroMemory(&mouse_state, sizeof(mouse_state));
        }
    }
    return 1;
}

extern "C" int32_t function_408d00(void) {
    IDirectInput8A *direct_input = (IDirectInput8A *)g769;
    IDirectInputDevice8A *mouse = NULL;
    DIPROPDWORD buffer_property;
    HRESULT hr;

    if (g771 != NULL) {
        return 1;
    }
    if (direct_input == NULL || g768 == NULL) {
        return 0;
    }
    hr = direct_input->CreateDevice(GUID_SysMouse, &mouse, NULL);
    if (FAILED(hr) || mouse == NULL) {
        return 0;
    }
    hr = mouse->SetDataFormat(&c_dfDIMouse2);
    if (SUCCEEDED(hr)) {
        hr = mouse->SetCooperativeLevel((HWND)g768, 6);
    }
    ZeroMemory(&buffer_property, sizeof(buffer_property));
    buffer_property.diph.dwSize = sizeof(buffer_property);
    buffer_property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    buffer_property.diph.dwObj = 0;
    buffer_property.diph.dwHow = DIPH_DEVICE;
    buffer_property.dwData = 1;
    if (SUCCEEDED(hr)) {
        hr = mouse->SetProperty(DIPROP_BUFFERSIZE, &buffer_property.diph);
    }
    if (SUCCEEDED(hr)) {
        hr = mouse->Acquire();
    }
    if (FAILED(hr)) {
        mouse->Release();
        return 0;
    }
    g771 = mouse;
    return 1;
}

extern "C" int32_t function_408e60(int32_t a1) {
    return g_retdec_keyboard_state[(unsigned char)a1] >> 7;
}

extern "C" int32_t function_408e80(int32_t a1) {
    return a1 >= 0 && a1 < g782 && g783 != NULL
        ? (int32_t)(intptr_t)(g783 + 80 * a1) : 0;
}
