#include "kinoko/input_devices.h"
#include "kinoko/legacy_memory.hpp"
#include <vector>
#include "kinoko/input_keys.h"
#include "kinoko/input_cluster.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/legacy_abi.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>

extern "C" int32_t g35[2];
namespace {
using Address = uint32_t;
template<class T> T* pointer(Address p) { return reinterpret_cast<T*>(static_cast<uintptr_t>(p)); }
Address address(const void* p) { return static_cast<Address>(reinterpret_cast<uintptr_t>(p)); }
struct Device {
    Address vtable;
    unsigned char value[164];
    Device() : vtable(address(g35)), value{} {}
    Device(const Device& source) : Device() { *this=source; }
    Device& operator=(const Device& source) {
        std::copy_n(source.value,sizeof(value),value);return *this;
    }
    ~Device() {
        retdec_call_thiscall1(this, pointer<void>(*pointer<Address>(vtable)), 0);
    }
};
static_assert(sizeof(Device)==168);
using Devices=std::vector<Device>;
Devices*& devices(int32_t manager) { return kinoko::legacy::field<Devices*>(manager+180); }

}

extern "C" int32_t __fastcall kinoko_method_delete_input_device(int32_t receiver, void*, unsigned char flags) {
    *pointer<Address>(receiver) = address(g35);
    if (flags & 1) std::free(pointer<void>(receiver));
    return receiver;
}

extern "C" void kinoko_input_devices_construct(int32_t manager) { devices(manager)=new Devices; }
extern "C" void kinoko_input_devices_destroy(int32_t manager) { delete devices(manager);devices(manager)=nullptr; }
extern "C" void kinoko_input_devices_resize(int32_t manager,uint32_t count) { devices(manager)->resize(count); }
extern "C" void kinoko_input_devices_assign(int32_t destination,int32_t source) {
    if(destination!=source) *devices(destination)=*devices(source);
}
extern "C" int32_t kinoko_input_devices_begin(int32_t manager) {
    return devices(manager)?address(devices(manager)->data()):0;
}
extern "C" int32_t kinoko_input_devices_end(int32_t manager) {
    return kinoko_input_devices_begin(manager)+(devices(manager)?static_cast<int32_t>(devices(manager)->size()*sizeof(Device)):0);
}

// SqPlus copy-function ABI is explicit (destination, source), not __thiscall.
// 46ED80 delegates to Input::operator= (46EBD0) and returns the destination.
extern "C" int32_t function_46ed80(int32_t destination, int32_t source) {
    if (destination == source) return destination;
    auto* out = pointer<unsigned char>(destination);
    const auto* in = pointer<unsigned char>(source);
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(intptr_t)(destination), (const void *)(intptr_t)(source)));
    std::copy_n(in + 16, 164, out + 16);
    kinoko_input_devices_assign(destination, source);
    std::copy_n(in + 200, 164, out + 200);
    kinoko_input_cluster_assign(destination + 196, source + 196);
    out[388] = in[388];
    kinoko_input_keys_assign(destination + 392, source + 392);
    std::copy_n(in + 1436, 76, out + 1436);
    return destination;
}
