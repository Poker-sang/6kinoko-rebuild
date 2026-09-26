#include "kinoko/direct_input.h"
#include "kinoko/input_device.h"
#include <cstring>

extern "C" {
extern unsigned char kinoko_keyboard_state[256];
}

namespace {
bool key(int32_t scan) { return (kinoko_keyboard_state[uint8_t(scan)] & 0x80) != 0; }
// INC/DEC in the original wrap, including at signed integer limits.
int32_t advance(int32_t count, int32_t delta) {
    return static_cast<int32_t>(uint32_t(count) + uint32_t(delta));
}
void direction(KinokoInputState& state, int axis, int sign) {
    auto& count = state.counts[axis];
    state.released[axis] = sign == 0 && count != 0;
    if (!sign) count = 0;
    else {
        if ((sign < 0 && count > 0) || (sign > 0 && count < 0)) count = 0;
        count = advance(count, sign);
    }
}
void button(KinokoInputState& state, int index, bool pressed) {
    auto& count = state.counts[index + 2];
    state.released[index + 2] = !pressed && count > 0;
    count = pressed ? advance(count, 1) : 0;
}
}

// 407500: physical input update, with the original 168-byte object layout.
extern "C" int32_t __fastcall kinoko_input_device_update(KinokoInputDevice* self, void*) {
    auto& assignment = self->assignment;
    auto& output = self->state;
    const auto id = static_cast<int8_t>(assignment.id);
    if (id >= 0) {
        const auto* state = kinoko_input_controller_state(id);
        if (!state) return id;
        const auto* axes = state->axes;
        for (int axis = 0; axis < 2; ++axis)
            direction(output, axis, axes[axis] < -500 ? -1 : axes[axis] > 500 ? 1 : 0);
        for (int index = 0; index < 12; ++index)
            if (assignment.buttons[index] >= 0)
                button(output, index, state->buttons[assignment.buttons[index]] != 0);
        for (int axis = 0; axis < 6; ++axis)
            output.axes[axis] = static_cast<float>(static_cast<double>(axes[axis]) / 1000.0);
    } else if (id == -1) {
        const int32_t negative[] = {assignment.left, assignment.up};
        const int32_t positive[] = {assignment.right, assignment.down};
        for (int axis = 0; axis < 2; ++axis) {
            if (negative[axis] < 0 || positive[axis] < 0) continue;
            const int sign = key(negative[axis]) ? -1 : key(positive[axis]) ? 1 : 0;
            direction(output, axis, sign);
            output.axes[axis] = static_cast<float>(sign);
        }
        for (int index = 0; index < 12; ++index)
            if (assignment.buttons[index] >= 0) button(output, index, key(assignment.buttons[index]));
        for (int axis = 2; axis < 6; ++axis) output.axes[axis] = 0;
    } else {
        std::memset(&output, 0, sizeof(output));
        return static_cast<int32_t>(reinterpret_cast<intptr_t>(&output));
    }
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(output.released));
}
