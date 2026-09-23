#include "kinoko/input_devices.h"
#include "kinoko/squirrel_host_compat.h"
#include <vector>
#include <cstdlib>

namespace {
// A native vector element owns its virtual device lifetime. Assignment copies
// payload only; construction installs base methods, as the original copy ctor.
struct Device {
    KinokoInputDevice record{};
    Device() { record.methods=&kinoko_input_device_methods; }
    Device(const Device& source):Device() { *this=source; }
    Device& operator=(const Device& source) {
        record.assignment=source.record.assignment;record.state=source.record.state;return *this;
    }
    ~Device() { record.methods->destroy(&record,0); }
};
static_assert(sizeof(Device)==sizeof(KinokoInputDevice) && offsetof(Device,record)==0);
}
struct KinokoInputDeviceStorage { std::vector<Device> devices; };
extern "C" KinokoInputDevice* __fastcall kinoko_input_device_delete(KinokoInputDevice* receiver,void*,unsigned char flags) {
    receiver->methods=&kinoko_input_device_methods;
    if (flags&1) std::free(receiver);
    return receiver;
}
extern "C" const KinokoInputDeviceMethods kinoko_input_device_methods{
    reinterpret_cast<decltype(KinokoInputDeviceMethods::destroy)>(kinoko_input_device_delete),
    reinterpret_cast<decltype(KinokoInputDeviceMethods::update)>(kinoko_input_device_update)};
extern "C" void kinoko_input_devices_construct(KinokoInputManager* manager) { manager->devices=new KinokoInputDeviceStorage; }
extern "C" void kinoko_input_devices_destroy(KinokoInputManager* manager) { delete manager->devices;manager->devices=nullptr; }
extern "C" void kinoko_input_devices_resize(KinokoInputManager* manager,uint32_t count) { manager->devices->devices.resize(count); }
extern "C" void kinoko_input_devices_assign(KinokoInputManager* destination,const KinokoInputManager* source) {
    if (destination!=source) destination->devices->devices=source->devices->devices;
}
extern "C" uint32_t kinoko_input_devices_size(const KinokoInputManager* manager) {
    return manager->devices?static_cast<uint32_t>(manager->devices->devices.size()):0;
}
extern "C" KinokoInputDevice* kinoko_input_devices_at(KinokoInputManager* manager,uint32_t index) {
    return &manager->devices->devices.at(index).record;
}
extern "C" KinokoInputDevice* kinoko_input_devices_begin(KinokoInputManager* manager) {
    return manager->devices?reinterpret_cast<KinokoInputDevice*>(manager->devices->devices.data()):nullptr;
}
extern "C" KinokoInputDevice* kinoko_input_devices_end(KinokoInputManager* manager) {
    auto* begin=kinoko_input_devices_begin(manager);
    return begin?reinterpret_cast<KinokoInputDevice*>(reinterpret_cast<unsigned char*>(begin)+
        kinoko_input_devices_size(manager)*sizeof(Device)):nullptr;
}
// 46ED80 -> 46EBD0: copy the SqPlus external reference first, then payloads
// and owners in original order. Cluster pointers remain borrowed from source.
extern "C" KinokoInputManager* kinoko_input_manager_assign(KinokoInputManager* destination,const KinokoInputManager* source) {
    if (destination==source) return destination;
    kinoko_sqplus_object_assign(destination->script_object,source->script_object);
    destination->keyboard.assignment=source->keyboard.assignment;
    destination->keyboard.state=source->keyboard.state;
    kinoko_input_devices_assign(destination,source);
    destination->cluster.device.assignment=source->cluster.device.assignment;
    destination->cluster.device.state=source->cluster.device.state;
    kinoko_input_cluster_assign(&destination->cluster,&source->cluster);
    destination->cluster.active_device=source->cluster.active_device;
    kinoko_input_keys_assign(&destination->keys,&source->keys);
    destination->published=source->published;
    return destination;
}
