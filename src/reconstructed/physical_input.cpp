#include "kinoko/direct_input.h"
#include "kinoko/input_keys.h"
#include "kinoko/legacy_memory.hpp"

extern "C" {
extern unsigned char g_retdec_keyboard_state[256];
}

namespace {
using namespace kinoko::legacy;
struct Device {
    int32_t vtable, id;
    int32_t up, down, left, right;
    int32_t buttons[12];
    int32_t counts[14];
    uint8_t released[14], padding[2];
    float axes[6];
};
static_assert(sizeof(Device) == 168);
static_assert(offsetof(Device, counts) == 72);
static_assert(offsetof(Device, released) == 128);
static_assert(offsetof(Device, axes) == 144);
bool key(int32_t scan) { return (g_retdec_keyboard_state[uint8_t(scan)] & 0x80) != 0; }
// INC/DEC in the original wrap, including at signed integer limits.
int32_t advance(int32_t count, int32_t delta) {
    return static_cast<int32_t>(uint32_t(count) + uint32_t(delta));
}
void direction(Device& device, int axis, int sign) {
    auto& count = device.counts[axis];
    device.released[axis] = sign == 0 && count != 0;
    if (!sign) count = 0;
    else {
        if ((sign < 0 && count > 0) || (sign > 0 && count < 0)) count = 0;
        count = advance(count, sign);
    }
}
void button(Device& device, int index, bool pressed) {
    auto& count = device.counts[index + 2];
    device.released[index + 2] = !pressed && count > 0;
    count = pressed ? advance(count, 1) : 0;
}
}

// 407500: physical input update, with the original 168-byte object layout.
extern "C" int32_t __fastcall function_407500(int32_t self) {
    auto& device = *pointer<Device>(self);
    const auto id = static_cast<int8_t>(device.id);
    if (id >= 0) {
        const auto* state = kinoko_input_controller_state(id);
        if (!state) return id;
        const auto* axes = state->axes;
        for (int axis = 0; axis < 2; ++axis)
            direction(device, axis, axes[axis] < -500 ? -1 : axes[axis] > 500 ? 1 : 0);
        for (int index = 0; index < 12; ++index)
            if (device.buttons[index] >= 0)
                button(device, index, state->buttons[device.buttons[index]] != 0);
        for (int axis = 0; axis < 6; ++axis)
            device.axes[axis] = static_cast<float>(static_cast<double>(axes[axis]) / 1000.0);
    } else if (id == -1) {
        const int32_t negative[] = {device.left, device.up};
        const int32_t positive[] = {device.right, device.down};
        for (int axis = 0; axis < 2; ++axis) {
            if (negative[axis] < 0 || positive[axis] < 0) continue;
            const int sign = key(negative[axis]) ? -1 : key(positive[axis]) ? 1 : 0;
            direction(device, axis, sign);
            device.axes[axis] = static_cast<float>(sign);
        }
        for (int index = 0; index < 12; ++index)
            if (device.buttons[index] >= 0) button(device, index, key(device.buttons[index]));
        for (int axis = 2; axis < 6; ++axis) device.axes[axis] = 0;
    } else {
        std::memset(pointer(self + 72), 0, 96);
        return self + 72;
    }
    return self + 128;
}

