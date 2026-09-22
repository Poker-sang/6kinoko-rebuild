#include "kinoko/input_cluster.h"
#include <deque>
#include <cstdlib>
#include <cstring>
#include <cmath>

extern "C" int32_t g35[2]; // remaining native methods-table boundary
struct KinokoInputClusterStorage { std::deque<KinokoInputDevice*> devices; };
namespace {
// Original CDQ/XOR/SUB, including signed comparison of INT_MIN's bits.
int32_t magnitude(int32_t value) {
    return value<0?static_cast<int32_t>(0u-uint32_t(value)):value;
}
void merge_device(KinokoInputCluster& cluster,const KinokoInputDevice& device) {
    auto& output=cluster.device.state;const auto& source=device.state;
    for (int index=0;index<14;++index) {
        auto& current=output.counts[index];const auto incoming=source.counts[index];
        const bool replace=index<2?magnitude(incoming)>magnitude(current):incoming>current;
        if (replace) {
            current=incoming;output.released[index]=0;
            cluster.active_device=static_cast<uint8_t>(device.assignment.id);
        } else if (current==0 && source.released[index]) output.released[index]=1;
    }
    for (int axis=0;axis<6;++axis)
        if (std::fabs(source.axes[axis])>std::fabs(output.axes[axis])) output.axes[axis]=source.axes[axis];
}
}
extern "C" void kinoko_input_cluster_construct(KinokoInputCluster* cluster) {
    cluster->devices=new KinokoInputClusterStorage;
}
extern "C" void kinoko_input_cluster_clear(KinokoInputCluster* cluster) { cluster->devices->devices.clear(); }
extern "C" void kinoko_input_cluster_append(KinokoInputCluster* cluster,KinokoInputDevice* device) {
    cluster->devices->devices.push_back(device);
}
extern "C" void kinoko_input_cluster_assign(KinokoInputCluster* destination,const KinokoInputCluster* source) {
    if (destination!=source) destination->devices->devices=source->devices->devices;
}
extern "C" uint32_t kinoko_input_cluster_size(const KinokoInputCluster* cluster) {
    return static_cast<uint32_t>(cluster->devices->devices.size());
}
extern "C" KinokoInputDevice* kinoko_input_cluster_at(const KinokoInputCluster* cluster,uint32_t index) {
    return cluster->devices->devices.at(index);
}
extern "C" KinokoInputCluster* __fastcall kinoko_input_cluster_delete(KinokoInputCluster* cluster,void*,unsigned char flags) {
    delete cluster->devices;cluster->devices=nullptr;
    // 46A7E0 -> 4074B0 restores the base identity after destroying the deque.
    cluster->device.methods=reinterpret_cast<const KinokoInputDeviceMethods*>(g35);
    if (flags&1) std::free(cluster);
    return cluster;
}
// 4077C0: borrowed inputs are visited in registration order. The deque owns
// only pointers, including after assignment (no speculative rebinding).
extern "C" int32_t __fastcall kinoko_input_cluster_update(KinokoInputCluster* cluster,void*) {
    auto& output=cluster->device.state;
    std::memset(&output,0,sizeof(output));
    const auto count=kinoko_input_cluster_size(cluster);
    if (!count) return static_cast<int32_t>(reinterpret_cast<intptr_t>(&output));
    for (const auto* device:cluster->devices->devices) merge_device(*cluster,*device);
    return static_cast<int32_t>(count);
}
