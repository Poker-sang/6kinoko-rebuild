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

extern "C" int32_t function_46b7c0(int32_t this_ptr, int32_t lpFileName) {
    retdec_trace_i32("46b7c0:this", this_ptr);
    retdec_trace_i32("46b7c0:path", lpFileName);
    HANDLE file_handle = CreateFileA((LPCSTR)(intptr_t)lpFileName,
                                      GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        return 0;

    DWORD transferred = 0;
    WriteFile(file_handle, (LPCVOID)(intptr_t)(this_ptr + 0x10), 0x44,
              &transferred, NULL);
    int32_t begin = (int32_t)(intptr_t)kinoko_input_devices_begin((KinokoInputManager *)(intptr_t)(this_ptr));
    int32_t end = (int32_t)(intptr_t)kinoko_input_devices_end((KinokoInputManager *)(intptr_t)(this_ptr));
    if (begin != end)
        WriteFile(file_handle, (LPCVOID)(intptr_t)(begin + 4), 0x44,
                  &transferred, NULL);
    CloseHandle(file_handle);
    retdec_trace_squirrel_name("input:config-saved", lpFileName);
    return 0;
}

extern "C" int32_t function_46b880(int32_t this_ptr, int32_t lpFileName) {
    retdec_trace_i32("46b880:this", this_ptr);
    retdec_trace_i32("46b880:path", lpFileName);
    retdec_trace_squirrel_name("input:config-load", lpFileName);
    HANDLE file_handle = CreateFileA((LPCSTR)(intptr_t)lpFileName,
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        return 0;

    unsigned char record[0x44];
    DWORD transferred = 0;
    if (ReadFile(file_handle, record, sizeof(record), &transferred, NULL) &&
        transferred == sizeof(record)) {
        memcpy((void *)(intptr_t)(this_ptr + 0x10), record, sizeof(record));
        retdec_trace("input:config-keyboard-loaded");
        if ((int32_t)(int8_t)record[0] >= kinoko_input_snapshot.controller_count) {
            memset((void *)(intptr_t)(this_ptr + 0x10), 0, sizeof(record));
            *(unsigned char *)(intptr_t)(this_ptr + 0x10) = 0xfe;
        }

        // The original reads the second record once, then broadcasts that
        // same 0x44-byte value to every registered device record.
        if (ReadFile(file_handle, record, sizeof(record), &transferred,
                     NULL) && transferred == sizeof(record)) {
            int32_t begin = (int32_t)(intptr_t)kinoko_input_devices_begin((KinokoInputManager *)(intptr_t)(this_ptr));
            int32_t end = (int32_t)(intptr_t)kinoko_input_devices_end((KinokoInputManager *)(intptr_t)(this_ptr));
            if ((int32_t)(int8_t)record[0] >= kinoko_input_snapshot.controller_count) {
                memset(record, 0, sizeof(record));
                record[0] = 0xfe;
            }
            while (begin != end) {
                memcpy((void *)(intptr_t)(begin + 4), record,
                       sizeof(record));
                begin += 0xa8;
            }
        }
    }
    CloseHandle(file_handle);
    retdec_trace("46b880:done");
    return 0;
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

extern "C" int32_t function_46bbe0(int32_t this_ptr, int32_t device,
                        int32_t field, int32_t value) {
    if (this_ptr == 0 || field < 0 || field >= 12)
        return 0;

    int32_t begin = (int32_t)(intptr_t)kinoko_input_devices_begin((KinokoInputManager *)(intptr_t)(this_ptr));
    int32_t end = (int32_t)(intptr_t)kinoko_input_devices_end((KinokoInputManager *)(intptr_t)(this_ptr));
    int32_t count = (begin != 0 && end >= begin)
                        ? (end - begin) / 0xa8
                        : 0;
    int32_t *destination = 0;
    if (device == -1) {
        destination = (int32_t *)(intptr_t)(this_ptr + 0x10);
    } else if (device >= 0 && device < count) {
        destination = (int32_t *)(intptr_t)(begin + device * 0xa8 + 4);
    } else {
        return 0;
    }

    int32_t record[17];
    memcpy(record, destination, sizeof(record));
    record[field + 5] = value;
    if ((int32_t)(int8_t)(record[0] & 0xff) >= kinoko_input_snapshot.controller_count) {
        memset(record, 0, sizeof(record));
        record[0] = 0xfe;
    }
    memcpy(destination, record, sizeof(record));
    return 0;
}

extern "C" int32_t function_46bc90(int32_t this_ptr, int32_t device, int32_t field) {
    int32_t record[17];
    int32_t begin, end;
    if (this_ptr == 0 || field < 0 || field >= 12)
        return 0;
    if (device == -1) {
        for (int32_t scan = 0; scan < 256; ++scan) {
            /* The original excludes Kanji, Caps Lock and Kana. */
            if (scan == 148 || scan == 58 || scan == 112 || !kinoko_input_key_down(scan))
                continue;
            memcpy(record, (const void *)(intptr_t)(this_ptr + 16), sizeof(record));
            record[field + 1] = scan;
            memcpy((void *)(intptr_t)(this_ptr + 16), record, sizeof(record));
            retdec_trace_i32("input:assign-keyboard-field", field);
            retdec_trace_i32("input:assign-keyboard-scan", scan);
            return 1;
        }
        return 0;
    }
    begin = (int32_t)(intptr_t)kinoko_input_devices_begin((KinokoInputManager *)(intptr_t)(this_ptr));
    end = (int32_t)(intptr_t)kinoko_input_devices_end((KinokoInputManager *)(intptr_t)(this_ptr));
    if (device >= 0 && device < (end - begin) / 168) {
        for (int32_t index = 0; index < (end - begin) / 168; ++index) {
            const auto* state = kinoko_input_controller_state(index);
            if (state == 0) continue;
            for (int32_t button = 0; button < 32; ++button) {
                if (state->buttons[button] == 0) continue;
                memcpy(record, (const void *)(intptr_t)(begin + 4), sizeof(record));
                record[field + 5] = button;
                for (int32_t target = begin; target < end; target += 168)
                    memcpy((void *)(intptr_t)(target + 4), record, sizeof(record));
                return 1;
            }
        }
    }
    return 0;
}

extern "C" int32_t function_46be40(int32_t this_ptr, int32_t device, int32_t field) {
    if (this_ptr == 0 || field < 0 || field >= 12)
        return -1;

    int32_t begin = (int32_t)(intptr_t)kinoko_input_devices_begin((KinokoInputManager *)(intptr_t)(this_ptr));
    int32_t end = (int32_t)(intptr_t)kinoko_input_devices_end((KinokoInputManager *)(intptr_t)(this_ptr));
    int32_t count = (begin != 0 && end >= begin)
                        ? (end - begin) / 0xa8
                        : 0;
    int32_t *record = 0;
    if (device == -1) {
        record = (int32_t *)(intptr_t)(this_ptr + 0x10);
    } else if (device >= 0 && device < count) {
        record = (int32_t *)(intptr_t)(begin + device * 0xa8 + 4);
    } else {
        return -1;
    }
    /* 46BE83: keyboard indexes include the four direction assignments. */
    return record[field + (device == -1 ? 1 : 5)];
}

