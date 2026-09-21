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
Address allocate(size_t bytes) {
    void* p = std::calloc(1, bytes);
    if (!p) throw std::bad_alloc();
    return address(p);
}
struct Vector { Address begin, end, capacity; };
struct Device { Address vtable; unsigned char value[164]; };
static_assert(sizeof(Device) == 168);

void destroy_devices(Address begin, Address end) {
    for (; begin != end; begin += sizeof(Device)) {
        auto* device = pointer<Device>(begin);
        retdec_call_thiscall1(device, pointer<void>(*pointer<Address>(device->vtable)), 0);
    }
}
void assign_devices(Vector& out, const Vector& in) {
    const auto bytes = in.end - in.begin;
    const auto old_bytes = out.end - out.begin;
    if (bytes > out.capacity - out.begin) {
        if (bytes / sizeof(Device) > 0x1861861) throw std::length_error("vector<T> too long");
        destroy_devices(out.begin, out.end);
        std::free(pointer<void>(out.begin));
        out = {};
        out.begin = allocate(bytes);
        out.end = out.begin;
        out.capacity = out.begin + bytes;
    }
    auto* source = pointer<Device>(in.begin);
    auto* target = pointer<Device>(out.begin);
    const auto existing = (out.end - out.begin) / sizeof(Device);
    for (size_t i = 0; i < bytes / sizeof(Device); ++i) {
        // 46BF00 assigns the two value records, preserving an existing vtable;
        // 46BFF0 constructs a sliced CInputManager for a new vector element.
        if (i >= existing) target[i].vtable = address(g35);
        std::copy_n(source[i].value, sizeof(Device::value), target[i].value);
    }
    if (bytes < old_bytes) destroy_devices(out.begin + bytes, out.end);
    out.end = out.begin + bytes;
}
void assign_bytes(Vector& out, const Vector& in) {
    const auto bytes = in.end - in.begin;
    if (bytes > out.capacity - out.begin) {
        std::free(pointer<void>(out.begin));
        out = {};
        out.begin = allocate(bytes);
        out.capacity = out.begin + bytes;
    }
    if (bytes) std::copy_n(pointer<unsigned char>(in.begin), bytes, pointer<unsigned char>(out.begin));
    out.end = out.begin + bytes;
}

}

extern "C" int32_t __fastcall kinoko_method_delete_input_device(int32_t receiver, void*, unsigned char flags) {
    *pointer<Address>(receiver) = address(g35);
    if (flags & 1) std::free(pointer<void>(receiver));
    return receiver;
}

// SqPlus copy-function ABI is explicit (destination, source), not __thiscall.
// 46ED80 delegates to Input::operator= (46EBD0) and returns the destination.
extern "C" int32_t function_46ed80(int32_t destination, int32_t source) {
    if (destination == source) return destination;
    auto* out = pointer<unsigned char>(destination);
    const auto* in = pointer<unsigned char>(source);
    function_4a95c0_this(destination, source);
    std::copy_n(in + 16, 164, out + 16);
    assign_devices(*reinterpret_cast<Vector*>(out + 180), *reinterpret_cast<const Vector*>(in + 180));
    std::copy_n(in + 200, 164, out + 200);
    kinoko_input_cluster_assign(destination + 196, source + 196);
    out[388] = in[388];
    std::copy_n(in + 392, 1024, out + 392);
    assign_bytes(*reinterpret_cast<Vector*>(out + 1416), *reinterpret_cast<const Vector*>(in + 1416));
    std::copy_n(in + 1432, 3, out + 1432);
    std::copy_n(in + 1436, 76, out + 1436);
    return destination;
}
