#pragma once
#include "kinoko/input_device.h"
#include "kinoko/input_cluster.h"
#include "kinoko/input_keys.h"
typedef struct KinokoInputDeviceStorage KinokoInputDeviceStorage;
typedef struct KinokoInputPublishedState {
    int32_t x, y;
    int32_t buttons[6];
    uint8_t released[4];
    int32_t digits[10];
} KinokoInputPublishedState;
typedef struct KinokoInputManager {
    uint8_t script_object[12]; /* existing externally owned SqPlus object ABI */
    KinokoInputDevice keyboard;
    KinokoInputDeviceStorage *devices;
    uint8_t reserved184[12];
    KinokoInputCluster cluster;
    KinokoKeyTracker keys;
    KinokoInputPublishedState published;
} KinokoInputManager;
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_input_manager_construct_devices(KinokoInputManager*, uint32_t controllers);
int32_t kinoko_input_manager_update(KinokoInputManager*);
int32_t kinoko_input_save_config(KinokoInputManager*, const char* path);
int32_t kinoko_input_load_config(KinokoInputManager*, const char* path);
int32_t kinoko_input_set_assignment(KinokoInputManager*, int32_t device, int32_t field, int32_t value);
int32_t kinoko_input_wait_assignment(KinokoInputManager*, int32_t device, int32_t field);
int32_t kinoko_input_get_assignment(KinokoInputManager*, int32_t device, int32_t field);
KinokoInputManager *kinoko_input_manager_assign(KinokoInputManager*, const KinokoInputManager*);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoInputPublishedState)==76 && sizeof(KinokoInputManager)==1512);
static_assert(offsetof(KinokoInputManager,keyboard)==12);
static_assert(offsetof(KinokoInputManager,devices)==180);
static_assert(offsetof(KinokoInputManager,cluster)==196);
static_assert(offsetof(KinokoInputManager,keys)==392);
static_assert(offsetof(KinokoInputManager,published)==1436);
static_assert(offsetof(KinokoInputPublishedState,released)==32);
static_assert(offsetof(KinokoInputPublishedState,digits)==36);
#endif
