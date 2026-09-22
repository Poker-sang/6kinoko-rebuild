#pragma once
#include <windows.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* DIJOYSTATE binary layout, shared without exposing COM to the C host. */
typedef struct KinokoControllerState {
    int32_t axes[6], sliders[2];
    uint32_t pov[4];
    uint8_t buttons[32];
} KinokoControllerState;
typedef struct KinokoMouseState {
    int32_t x, y, wheel;
    uint8_t buttons[8];
} KinokoMouseState;
/* Borrowed view of the owning input service's frame cache. */
typedef struct KinokoInputSnapshot {
    KinokoControllerState *controllers;
    int32_t controller_count;
    KinokoMouseState mouse;
} KinokoInputSnapshot;
extern KinokoInputSnapshot kinoko_input_snapshot;
int32_t kinoko_input_initialize(HWND window, HINSTANCE instance);
int32_t kinoko_input_shutdown(void);
int32_t kinoko_input_open_keyboard(void);
int32_t kinoko_input_open_controllers(void);
int32_t kinoko_input_open_mouse(void);
int32_t kinoko_input_poll(void);
int32_t kinoko_input_key_down(int32_t scan);
const KinokoControllerState *kinoko_input_controller_state(int32_t index);
HWND kinoko_input_window(void);
#ifdef __cplusplus
}
#endif
