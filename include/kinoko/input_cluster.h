#pragma once
#include "kinoko/input_device.h"
typedef struct KinokoInputClusterStorage KinokoInputClusterStorage;
typedef struct KinokoInputCluster {
    KinokoInputDevice device;
    uint32_t reserved168;
    KinokoInputClusterStorage *devices;
    uint8_t reserved176[16];
    uint8_t active_device, reserved193[3];
} KinokoInputCluster;
#ifdef __cplusplus
extern "C" {
#endif
extern const KinokoInputDeviceMethods kinoko_input_cluster_methods;
void kinoko_input_cluster_construct(KinokoInputCluster *cluster);
void kinoko_input_cluster_clear(KinokoInputCluster *cluster);
void kinoko_input_cluster_append(KinokoInputCluster *cluster, KinokoInputDevice *device);
void kinoko_input_cluster_assign(KinokoInputCluster *destination, const KinokoInputCluster *source);
uint32_t kinoko_input_cluster_size(const KinokoInputCluster *cluster);
KinokoInputDevice *kinoko_input_cluster_at(const KinokoInputCluster *cluster, uint32_t index);
KinokoInputCluster *__fastcall kinoko_input_cluster_delete(KinokoInputCluster *cluster, void *unused, unsigned char flags);
int32_t __fastcall kinoko_input_cluster_update(KinokoInputCluster *cluster, void *unused);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoInputCluster)==196);
static_assert(offsetof(KinokoInputCluster,devices)==172);
static_assert(offsetof(KinokoInputCluster,active_device)==192);
#endif
