#define DIRECTINPUT_VERSION 0x0800
#include "kinoko/direct_input.h"
#include <dinput.h>
#include <vector>
#include <memory>
#include <cstring>
#include <cstddef>
extern "C" {
extern unsigned char g_retdec_keyboard_state[256];
void retdec_poll_fallback_keyboard(void);
KinokoInputSnapshot kinoko_input_snapshot{};
}
namespace {
struct ReleaseDevice {
    void operator()(IDirectInputDevice8A* device) const {
        if (device) { device->Unacquire(); device->Release(); }
    }
};
using Device = std::unique_ptr<IDirectInputDevice8A, ReleaseDevice>;
struct ReleaseInput { void operator()(IDirectInput8A* input) const { if(input) input->Release(); } };
struct InputService {
    HWND window{};
    std::unique_ptr<IDirectInput8A, ReleaseInput> input;
    Device keyboard, mouse;
    std::vector<Device> controllers;
    std::vector<KinokoControllerState> states;
    std::vector<DWORD> button_counts;
};
InputService service;
static_assert(sizeof(KinokoControllerState) == sizeof(DIJOYSTATE));
static_assert(offsetof(KinokoControllerState, buttons) == offsetof(DIJOYSTATE, rgbButtons));
static_assert(sizeof(KinokoMouseState) == sizeof(DIMOUSESTATE2));
BOOL CALLBACK configure_axis(const DIDEVICEOBJECTINSTANCEA* object, void* context) {
    DIPROPRANGE range{};
    range.diph = {sizeof(range), sizeof(DIPROPHEADER), object->dwType, DIPH_BYID};
    range.lMin = -1000; range.lMax = 1000;
    return SUCCEEDED(static_cast<IDirectInputDevice8A*>(context)->SetProperty(DIPROP_RANGE, &range.diph));
}
BOOL CALLBACK enumerate_controller(const DIDEVICEINSTANCEA* instance, void*) {
    IDirectInputDevice8A* raw{};
    if (FAILED(service.input->CreateDevice(instance->guidInstance, &raw, nullptr))) return DIENUM_STOP;
    Device device(raw);
    // 408EB0 deliberately ignores these setup HRESULTs; axis enumeration stops
    // on the first failed range property. Acquisition occurs in the poll path.
    device->SetDataFormat(&c_dfDIJoystick);
    device->SetCooperativeLevel(service.window, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    DIDEVCAPS caps{}; caps.dwSize = sizeof(caps);
    device->GetCapabilities(&caps);
    service.button_counts.push_back(caps.dwButtons); // 408FE3 reads DIDEVCAPS + 16.
    device->EnumObjects(configure_axis, device.get(), DIDFT_AXIS);
    service.controllers.push_back(std::move(device));
    return DIENUM_CONTINUE;
}
int32_t open_device(Device& destination, REFGUID guid, const DIDATAFORMAT& format, DWORD flags, bool mouse) {
    if (destination) return 1;
    if (!service.input) return 0;
    IDirectInputDevice8A* raw{};
    if (FAILED(service.input->CreateDevice(guid, &raw, nullptr))) return 0;
    Device device(raw);
    if (FAILED(device->SetDataFormat(&format)) || FAILED(device->SetCooperativeLevel(service.window, flags))) return 0;
    if (mouse) {
        DIPROPDWORD property{};
        property.diph = {sizeof(property), sizeof(DIPROPHEADER), 0, DIPH_DEVICE};
        property.dwData = DIPROPAXISMODE_REL;
        // 408E1B uses property ID 2 (AXISMODE), not ID 1 (BUFFERSIZE).
        if (FAILED(device->SetProperty(DIPROP_AXISMODE, &property.diph))) return 0;
    }
    // Original ignores Acquire failure (e.g. window not foreground yet).
    device->Acquire();
    destination = std::move(device);
    return 1;
}
}
extern "C" HWND kinoko_input_window(void) { return service.window; }
extern "C" int32_t kinoko_input_initialize(HWND window, HINSTANCE instance) {
    if (service.input) return 1;
    service.window = window;
    IDirectInput8A* input{};
    HRESULT hr = CoCreateInstance(CLSID_DirectInput8, nullptr, CLSCTX_ALL, IID_IDirectInput8A, reinterpret_cast<void**>(&input));
    if (SUCCEEDED(hr) && input) {
        service.input.reset(input);
        hr = input->Initialize(instance, DIRECTINPUT_VERSION); // 408998, previously omitted.
        if (SUCCEEDED(hr)) return 1;
        service.input.reset();
    }
    MessageBoxA(window, "DirectInput initialization failed", "DInput-Error", MB_OK);
    return 0;
}
extern "C" int32_t kinoko_input_shutdown(void) {
    service.mouse.reset(); service.keyboard.reset();
    service.controllers.clear(); service.states.clear(); service.button_counts.clear();
    kinoko_input_snapshot = {};
    service.input.reset(); service.window = nullptr;
    return 1;
}
extern "C" int32_t kinoko_input_open_keyboard(void) {
    return open_device(service.keyboard, GUID_SysKeyboard, c_dfDIKeyboard, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY, false);
}
extern "C" int32_t kinoko_input_open_mouse(void) {
    return open_device(service.mouse, GUID_SysMouse, c_dfDIMouse2, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE, true);
}
extern "C" int32_t kinoko_input_open_controllers(void) {
    if (!service.input) return 1;
    if (FAILED(service.input->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerate_controller, nullptr, DIEDFL_ATTACHEDONLY))) return 0;
    // The original publishes the low byte of the vector length (408C50).
    const auto count = static_cast<uint8_t>(service.controllers.size());
    service.states.resize(count);
    kinoko_input_snapshot.controllers = service.states.data();
    kinoko_input_snapshot.controller_count = count;
    return 1;
}
extern "C" int32_t kinoko_input_poll(void) {
    // 408C80 then 40DCDC: controllers, keyboard, mouse, in that order.
    for (size_t i = 0; i < service.states.size(); ++i) {
        auto* device = service.controllers[i].get();
        if (FAILED(device->Poll())) device->Acquire();
        // Original retains the preceding cache if GetDeviceState fails.
        device->GetDeviceState(sizeof(KinokoControllerState), &service.states[i]);
    }
    if (service.keyboard) {
        if (FAILED(service.keyboard->GetDeviceState(256, g_retdec_keyboard_state))) {
            service.keyboard->Acquire();
            std::memset(g_retdec_keyboard_state, 0, 256);
        }
    } else {
        // Established reconstruction fallback: foreground-only scan mapping.
        retdec_poll_fallback_keyboard();
    }
    if (service.mouse && FAILED(service.mouse->GetDeviceState(sizeof(KinokoMouseState), &kinoko_input_snapshot.mouse)))
        service.mouse->Acquire();
    return 1;
}
extern "C" int32_t kinoko_input_key_down(int32_t scan) { return g_retdec_keyboard_state[uint8_t(scan)] >> 7; }
extern "C" const KinokoControllerState* kinoko_input_controller_state(int32_t index) {
    return index >= 0 && index < kinoko_input_snapshot.controller_count && kinoko_input_snapshot.controllers
        ? &kinoko_input_snapshot.controllers[index] : nullptr;
}
