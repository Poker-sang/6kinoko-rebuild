#include "kinoko/input_devices.h"
#include <cstdio>
#include <cstring>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "input frame line %d\n", __LINE__); return 1; } } while (0)
static int32_t order[8];
static unsigned calls;
extern "C" {
unsigned char g_retdec_keyboard_state[256]{};
void* kinoko_sqplus_object_assign(void* destination, const void* source) {
    std::memcpy(destination, source, 12); return destination;
}
int32_t __fastcall kinoko_input_device_update(KinokoInputDevice* device, void*) {
    order[calls++] = device->assignment.id;
    return 0;
}
}
static int32_t __fastcall cluster_update(KinokoInputDevice* device, void*) {
    order[calls++] = 999;
    for (int i = 0; i < 14; ++i) device->state.counts[i] = 70 + i;
    for (int i = 0; i < 4; ++i) device->state.released[i + 2] = static_cast<uint8_t>(i & 1);
    // A key changed in the virtual cluster callback must reach this frame.
    g_retdec_keyboard_state[10] = 0x80;
    return 0;
}
int main() {
    KinokoInputManager manager{};
    manager.script_object[0] = 0x42;
    kinoko_input_manager_construct_devices(&manager, 2);
    CHECK(manager.script_object[0] == 0x42);
    CHECK(kinoko_input_devices_size(&manager) == 2);
    CHECK(kinoko_input_cluster_size(&manager.cluster) == 3);
    CHECK(kinoko_input_cluster_at(&manager.cluster, 0) == kinoko_input_devices_at(&manager, 0));
    CHECK(kinoko_input_cluster_at(&manager.cluster, 2) == &manager.keyboard);
    CHECK(manager.keyboard.assignment.id == 255 && manager.keyboard.assignment.up == 200);
    CHECK(manager.keyboard.assignment.buttons[3] == 45 && manager.keyboard.assignment.buttons[6] == 255);
    CHECK(manager.keyboard.assignment.buttons[7] == 0 && kinoko_input_devices_at(&manager, 1)->assignment.buttons[11] == 11);
    CHECK(kinoko_input_keys_size(&manager.keys) == 22);
    CHECK(kinoko_input_keys_at(&manager.keys, 0) == 59 && kinoko_input_keys_at(&manager.keys, 21) == 11);
    const KinokoInputDeviceMethods cluster_methods{
        kinoko_input_cluster_methods.destroy,
        reinterpret_cast<decltype(KinokoInputDeviceMethods::update)>(cluster_update)};
    manager.cluster.device.methods = &cluster_methods;
    g_retdec_keyboard_state[11] = g_retdec_keyboard_state[2] = 0x80;
    CHECK(kinoko_input_manager_update(&manager) == 1);
    CHECK(calls == 4 && order[0] == 0 && order[1] == 1 && order[2] == 255 && order[3] == 999);
    CHECK(manager.published.x == 70 && manager.published.y == 71);
    for (int i = 0; i < 6; ++i) CHECK(manager.published.buttons[i] == 72 + i);
    for (int i = 0; i < 4; ++i) CHECK(manager.published.released[i] == (i & 1));
    CHECK(manager.published.digits[0] == 1 && manager.published.digits[1] == 1 && manager.published.digits[9] == 1);
    for (int i = 2; i < 9; ++i) CHECK(manager.published.digits[i] == 0);
    CHECK(kinoko_input_manager_update(nullptr) == 0 && calls == 4);
    kinoko_input_cluster_delete(&manager.cluster, nullptr, 0);
    kinoko_input_keys_destroy(&manager.keys);
    kinoko_input_devices_destroy(&manager);
    KinokoInputManager keyboard_only{};
    kinoko_input_manager_construct_devices(&keyboard_only, 0);
    CHECK(kinoko_input_cluster_size(&keyboard_only.cluster) == 1);
    CHECK(kinoko_input_cluster_at(&keyboard_only.cluster, 0) == &keyboard_only.keyboard);
    kinoko_input_cluster_delete(&keyboard_only.cluster, nullptr, 0);
    kinoko_input_keys_destroy(&keyboard_only.keys);
    kinoko_input_devices_destroy(&keyboard_only);
    return 0;
}
