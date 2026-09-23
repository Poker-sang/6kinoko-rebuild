#include "kinoko/input_devices.h"

// Called once on zeroed native storage; the host owns the SqPlus base and
// global initialization guard. The cluster borrows stable vector elements.
extern "C" void kinoko_input_manager_construct_devices(KinokoInputManager* manager, uint32_t controllers) {
    manager->keyboard.methods = &kinoko_input_device_methods;
    manager->cluster.device.methods = &kinoko_input_cluster_methods;
    manager->keyboard.assignment = {255, 200, 208, 203, 205,
                                   {44, 30, 46, 45, 255, 255, 255, 0, 0, 0, 0, 0}};
    kinoko_input_devices_construct(manager);
    kinoko_input_devices_resize(manager, controllers);
    kinoko_input_cluster_construct(&manager->cluster);
    for (uint32_t i = 0; i < controllers; ++i) {
        auto* device = kinoko_input_devices_at(manager, i);
        device->assignment.id = static_cast<uint8_t>(i);
        for (int32_t button = 0; button < 12; ++button) device->assignment.buttons[button] = button;
        kinoko_input_cluster_append(&manager->cluster, device);
    }
    kinoko_input_cluster_append(&manager->cluster, &manager->keyboard);
    kinoko_input_keys_construct(&manager->keys);
    constexpr uint8_t scans[] = {59,60,61,62,63,64,65,66,67,68,87,88,2,3,4,5,6,7,8,9,10,11};
    for (auto scan : scans) kinoko_input_keys_add(&manager->keys, scan);
}
namespace {
void update(KinokoInputDevice& device) {
    if (device.methods && device.methods->update) device.methods->update(&device);
}
}
// 46B9A0: controllers, keyboard, virtual cluster update, then key tracker.
extern "C" int32_t kinoko_input_manager_update(KinokoInputManager* manager) {
    if (!manager) return 0;
    for (uint32_t i = 0; i < kinoko_input_devices_size(manager); ++i)
        update(*kinoko_input_devices_at(manager, i));
    update(manager->keyboard);
    update(manager->cluster.device);
    kinoko_input_keys_update(&manager->keys);
    const auto& state = manager->cluster.device.state;
    auto& published = manager->published;
    published.buttons[0] = state.counts[2];
    published.x = state.counts[0];
    published.y = state.counts[1];
    published.buttons[3] = state.counts[5];
    published.buttons[1] = state.counts[3];
    published.buttons[2] = state.counts[4];
    published.buttons[4] = state.counts[6];
    published.released[0] = state.released[2];
    published.buttons[5] = state.counts[7];
    for (int i = 1; i < 4; ++i) published.released[i] = state.released[i + 2];
    published.digits[0] = kinoko_input_key_pressed(&manager->keys, 11, 0, 0, 0);
    for (int i = 1; i < 10; ++i)
        published.digits[i] = kinoko_input_key_pressed(&manager->keys, i + 1, 0, 0, 0);
    return published.digits[9];
}
