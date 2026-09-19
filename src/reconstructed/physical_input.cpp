#include "kinoko/legacy_memory.hpp"

extern "C" {
extern unsigned char g_retdec_keyboard_state[256];
extern int32_t g782;
extern char* g783;
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
        if (id >= g782) return id;
        const auto state_address = address(g783) + 80 * id;
        if (!state_address) return id;
        const auto* axes = pointer<int32_t>(state_address);
        for (int axis = 0; axis < 2; ++axis)
            direction(device, axis, axes[axis] < -500 ? -1 : axes[axis] > 500 ? 1 : 0);
        for (int index = 0; index < 12; ++index)
            if (device.buttons[index] >= 0)
                button(device, index, pointer<uint8_t>(state_address)[48 + device.buttons[index]] != 0);
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

extern "C" int32_t function_408320(int32_t self) {
    for (int32_t cursor = field<int32_t>(self, 1024); cursor != field<int32_t>(self, 1028); ++cursor) {
        const auto scan = field<uint8_t>(cursor);
        auto& count = field<int32_t>(self, 4u * scan);
        count = key(scan) ? advance(count, 1) : 0;
    }
    field<uint8_t>(self, 1040) = key(0x2a) || key(0x36);
    field<uint8_t>(self, 1041) = key(0x38) || key(0xb8);
    return field<uint8_t>(self, 1042) = key(0x1d) || key(0x9d);
}

extern "C" int32_t function_4083e0(int32_t self, int32_t scan, int32_t shift, int32_t alt, int32_t ctrl) {
    return field<int32_t>(self, 4u * uint8_t(scan)) == 1
        && (!uint8_t(shift) || field<uint8_t>(self, 1040))
        && (!uint8_t(alt) || field<uint8_t>(self, 1041))
        && (!uint8_t(ctrl) || field<uint8_t>(self, 1042));
}
