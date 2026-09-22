#include "kinoko/input_keys.h"
#include "kinoko/direct_input.h"
#include "kinoko/input_devices.h"
#include "kinoko/input_cluster.h"
#include <windows.h>
#include <cstdint>
#include <cstring>
#include "kinoko/legacy_abi.h"

extern "C" {
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_name(const char*, int32_t);
}
namespace {
// Original Input record: 4-byte vtable, 68-byte assignment, 96-byte state.
constexpr int device_stride = 168;
int32_t& word(int32_t base, int offset) {
    return *reinterpret_cast<int32_t*>(static_cast<intptr_t>(base) + offset);
}
uint8_t& byte(int32_t base, int offset) {
    return *reinterpret_cast<uint8_t*>(static_cast<intptr_t>(base) + offset);
}
} // namespace

namespace {
// 4074C0 validates the signed low byte, not the full integer identifier.
void apply_assignment(KinokoInputDevice& device, KinokoInputAssignment record) {
    if (static_cast<int8_t>(record.id & 255) >= kinoko_input_snapshot.controller_count) {
        record = {};
        record.id = 254;
    }
    device.assignment = record;
}
int32_t& keyboard_field(KinokoInputAssignment& record, int32_t field) {
    switch (field) {
    case 0: return record.up;
    case 1: return record.down;
    case 2: return record.left;
    case 3: return record.right;
    default: return record.buttons[field - 4];
    }
}
KinokoInputDevice* assignment_target(KinokoInputManager* manager, int32_t device) {
    if (device == -1) return &manager->keyboard;
    // 46BC3C / 46BE6C: device only bounds-checks; both use the FIRST record.
    if (device >= 0 && static_cast<uint32_t>(device) < kinoko_input_devices_size(manager))
        return kinoko_input_devices_at(manager, 0);
    return nullptr;
}
class ConfigFile {
public:
    explicit ConfigFile(HANDLE handle): handle_(handle) {}
    ~ConfigFile() { if (valid()) CloseHandle(handle_); }
    ConfigFile(const ConfigFile&) = delete;
    ConfigFile& operator=(const ConfigFile&) = delete;
    bool valid() const { return handle_ != INVALID_HANDLE_VALUE; }
    bool read(KinokoInputAssignment& record) {
        DWORD count = 0;
        return ReadFile(handle_, &record, sizeof(record), &count, nullptr) && count == sizeof(record);
    }
    void write(const KinokoInputAssignment& record) {
        DWORD count = 0;
        WriteFile(handle_, &record, sizeof(record), &count, nullptr);
    }
private:
    HANDLE handle_;
};
int32_t diagnostic_address(const void* pointer) {
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(pointer));
}
}
extern "C" int32_t kinoko_input_save_config(KinokoInputManager* manager, const char* path) {
    retdec_trace_i32("46b7c0:this", diagnostic_address(manager));
    retdec_trace_i32("46b7c0:path", diagnostic_address(path));
    {
        ConfigFile file(CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!file.valid()) return 0;
        file.write(manager->keyboard.assignment);
        if (kinoko_input_devices_size(manager)) file.write(kinoko_input_devices_at(manager, 0)->assignment);
    }
    retdec_trace_squirrel_name("input:config-saved", diagnostic_address(path));
    return 0;
}
extern "C" int32_t kinoko_input_load_config(KinokoInputManager* manager, const char* path) {
    retdec_trace_i32("46b880:this", diagnostic_address(manager));
    retdec_trace_i32("46b880:path", diagnostic_address(path));
    retdec_trace_squirrel_name("input:config-load", diagnostic_address(path));
    {
        ConfigFile file(CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!file.valid()) return 0;
        KinokoInputAssignment record;
        if (file.read(record)) {
            apply_assignment(manager->keyboard, record);
            retdec_trace("input:config-keyboard-loaded");
            // One saved controller record is broadcast to all registered devices.
            if (file.read(record))
                for (uint32_t i = 0; i < kinoko_input_devices_size(manager); ++i)
                    apply_assignment(*kinoko_input_devices_at(manager, i), record);
        }
    }
    retdec_trace("46b880:done");
    return 0;
}
extern "C" int32_t kinoko_input_set_assignment(KinokoInputManager* manager, int32_t device,
                                               int32_t field, int32_t value) {
    if (!manager || field < 0 || field >= 12) return 0;
    auto* target = assignment_target(manager, device);
    if (!target) return 0;
    auto record = target->assignment;
    record.buttons[field] = value; // SetAssign uses buttons even for keyboard.
    apply_assignment(*target, record);
    return 0;
}
extern "C" int32_t kinoko_input_wait_assignment(KinokoInputManager* manager, int32_t device, int32_t field) {
    if (!manager || field < 0 || field >= 12) return 0;
    if (device == -1) {
        for (int32_t scan = 0; scan < 256; ++scan) {
            if (scan == 148 || scan == 58 || scan == 112 || !kinoko_input_key_down(scan)) continue;
            auto record = manager->keyboard.assignment;
            keyboard_field(record, field) = scan;
            apply_assignment(manager->keyboard, record);
            retdec_trace_i32("input:assign-keyboard-field", field);
            retdec_trace_i32("input:assign-keyboard-scan", scan);
            return 1;
        }
        return 0;
    }
    const auto count = kinoko_input_devices_size(manager);
    if (device < 0 || static_cast<uint32_t>(device) >= count) return 0;
    for (uint32_t i = 0; i < count; ++i) {
        const auto* state = kinoko_input_controller_state(i);
        if (!state) continue;
        for (int32_t button = 0; button < 32; ++button) {
            if (!state->buttons[button]) continue;
            auto record = kinoko_input_devices_at(manager, 0)->assignment;
            record.buttons[field] = button;
            for (uint32_t target = 0; target < count; ++target)
                apply_assignment(*kinoko_input_devices_at(manager, target), record);
            return 1;
        }
    }
    return 0;
}
extern "C" int32_t kinoko_input_get_assignment(KinokoInputManager* manager, int32_t device, int32_t field) {
    if (!manager || field < 0 || field >= 12) return -1;
    auto* target = assignment_target(manager, device);
    if (!target) return -1;
    return device == -1 ? keyboard_field(target->assignment, field) : target->assignment.buttons[field];
}

extern "C" int32_t function_46b9a0(int32_t self) {
    if (!self) return 0;
    const int32_t begin = (int32_t)(intptr_t)kinoko_input_devices_begin((KinokoInputManager *)(intptr_t)(self)), end = (int32_t)(intptr_t)kinoko_input_devices_end((KinokoInputManager *)(intptr_t)(self));
    const int count = begin && end >= begin && (end-begin)%device_stride == 0
        ? (end-begin)/device_stride : 0;
    auto update_device = [](int32_t device) {
        const auto* vtable = reinterpret_cast<const int32_t*>(word(device, 0));
        if (vtable && vtable[1]) retdec_call_thiscall0(
            reinterpret_cast<void*>(device), reinterpret_cast<void*>(vtable[1]));
    };
    for (int i = 0; i < count; ++i) update_device(begin + i*device_stride);
    update_device(self + 12);
    kinoko_input_cluster_update((KinokoInputCluster *)(intptr_t)(self + 196), NULL);
    kinoko_input_keys_update((KinokoKeyTracker *)(intptr_t)(self + 392));
    // Publish the same directional/button counters and release edges.
    constexpr int copies[][2] = {{1444,276},{1436,268},{1440,272},{1456,288},
        {1448,280},{1452,284},{1460,292},{1464,296}};
    for (const auto& offsets : copies) word(self, offsets[0]) = word(self, offsets[1]);
    for (int i = 0; i < 4; ++i) byte(self, 1468+i) = byte(self, 326+i);
    word(self, 1472) = kinoko_input_key_pressed((KinokoKeyTracker *)(intptr_t)(self+392), 11, 0, 0, 0);
    for (int i = 1; i < 10; ++i)
        word(self, 1472+i*4) = kinoko_input_key_pressed((KinokoKeyTracker *)(intptr_t)(self+392), i+1, 0, 0, 0);
    return word(self, 1508);
}

