#include "kinoko/input_devices.h"
#include <cstring>
#include <cstdio>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"copy line %d\n",__LINE__);return 1; } } while(0)
static unsigned script_assigns, virtual_destroys;
extern "C" {
unsigned char kinoko_keyboard_state[256]{};
void* kinoko_sqplus_object_assign(void* destination,const void* source) {
    ++script_assigns;std::memcpy(destination,source,12);return destination;
}
int32_t __fastcall kinoko_input_device_update(KinokoInputDevice*,void*) { return 0; }
}
static KinokoInputDevice* __fastcall destroy(KinokoInputDevice* device,void*,unsigned char flags) {
    ++virtual_destroys;return kinoko_input_device_delete(device,nullptr,flags);
}
static void construct(KinokoInputManager& manager) {
    kinoko_input_devices_construct(&manager);kinoko_input_cluster_construct(&manager.cluster);
    kinoko_input_keys_construct(&manager.keys);
}
static void release(KinokoInputManager& manager) {
    kinoko_input_cluster_delete(&manager.cluster,nullptr,0);
    kinoko_input_keys_destroy(&manager.keys);kinoko_input_devices_destroy(&manager);
}
int main() {
    KinokoInputManager source{},target{};construct(source);construct(target);
    kinoko_input_devices_resize(&source,2);kinoko_input_devices_resize(&target,2);
    source.keyboard.assignment.id=255;source.keyboard.state.counts[2]=7;
    source.cluster.device.state.counts[13]=8;source.cluster.active_device=31;
    source.published.buttons[5]=9;source.published.digits[9]=10;
    source.script_object[4]=11;
    auto* source_device=kinoko_input_devices_at(&source,0);
    source_device->assignment.buttons[0]=17;
    kinoko_input_cluster_append(&source.cluster,source_device);
    kinoko_input_keys_add(&source.keys,44);
    target.reserved184[0]=0x31;target.cluster.reserved168=0x1234;target.keys.reserved1028[0]=0x32;
    const KinokoInputDeviceMethods custom{
        reinterpret_cast<decltype(KinokoInputDeviceMethods::destroy)>(destroy),nullptr};
    target.keyboard.methods=&custom;target.cluster.device.methods=&custom;
    kinoko_input_devices_at(&target,0)->methods=&custom;
    auto* owner=target.devices;auto* buffer=kinoko_input_devices_begin(&target);
    CHECK(kinoko_input_manager_assign(&target,&source)==&target && script_assigns==1);
    CHECK(target.devices==owner && kinoko_input_devices_begin(&target)==buffer);
    CHECK(target.keyboard.methods==&custom && target.cluster.device.methods==&custom);
    CHECK(kinoko_input_devices_at(&target,0)->methods==&custom);
    CHECK(target.keyboard.state.counts[2]==7 && target.cluster.device.state.counts[13]==8);
    CHECK(target.cluster.active_device==31 && target.published.buttons[5]==9 && target.published.digits[9]==10);
    CHECK(target.reserved184[0]==0x31 && target.cluster.reserved168==0x1234 && target.keys.reserved1028[0]==0x32);
    CHECK(kinoko_input_cluster_at(&target.cluster,0)==source_device && target.cluster.devices!=source.cluster.devices);
    source_device->assignment.buttons[0]=29;
    CHECK(kinoko_input_devices_at(&target,0)->assignment.buttons[0]==17);
    kinoko_input_keys_clear(&source.keys);CHECK(kinoko_input_keys_size(&target.keys)==1);
    CHECK(kinoko_input_manager_assign(&target,&target)==&target && script_assigns==1);
    kinoko_input_devices_resize(&target,0);CHECK(virtual_destroys==1);
    CHECK(kinoko_input_devices_begin(&target)==kinoko_input_devices_end(&target));
    release(target);release(source);
    CHECK(!target.devices && !source.devices);
    return 0;
}
