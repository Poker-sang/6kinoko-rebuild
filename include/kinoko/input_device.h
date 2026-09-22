#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct KinokoInputDeviceMethods KinokoInputDeviceMethods;
/* Original 0x44-byte assignment record, shared by keyboard/controller files.
   Only the low signed byte of id selects the source. Negative mappings skip
   an update; keyboard scans are interpreted through their low byte. */
typedef struct KinokoInputAssignment {
    int32_t id;
    int32_t up, down, left, right;
    int32_t buttons[12];
} KinokoInputAssignment;
typedef struct KinokoInputState {
    int32_t counts[14];
    uint8_t released[14], reserved70[2];
    float axes[6];
} KinokoInputState;
typedef struct KinokoInputDevice {
    const KinokoInputDeviceMethods *methods;
    KinokoInputAssignment assignment;
    KinokoInputState state;
} KinokoInputDevice;
#ifdef __cplusplus
extern "C" {
#endif
/* True ECX receiver; EDX padding adapts the original thiscall virtual slot.
   Return retains original mixed EAX (device id or end pointer), not ownership. */
int32_t __fastcall kinoko_input_device_update(KinokoInputDevice *device, void *unused);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoInputAssignment)==68 && sizeof(KinokoInputState)==96);
static_assert(sizeof(KinokoInputDevice)==168);
static_assert(offsetof(KinokoInputDevice,assignment)==4);
static_assert(offsetof(KinokoInputDevice,state)==72);
static_assert(offsetof(KinokoInputState,released)==56);
static_assert(offsetof(KinokoInputState,axes)==72);
#endif
