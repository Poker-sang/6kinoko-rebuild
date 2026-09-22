#include "kinoko/input_keys.h"
#include "kinoko/direct_input.h"
#include "kinoko/input_devices.h"
#include "kinoko/input_cluster.h"
#include <windows.h>
#include <cstdint>
#include <cstring>

extern "C" {
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_name(const char*, int32_t);
}
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

