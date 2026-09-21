#include "kinoko/input_cluster.h"
#include "kinoko/legacy_memory.hpp"
#include <deque>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>

extern "C" int32_t g35[2];
namespace {
using Devices = std::deque<int32_t>;
Devices*& storage(int32_t cluster) { return kinoko::legacy::field<Devices*>(cluster + 172); }
Devices& devices(int32_t cluster) { return *storage(cluster); }
template<class T> T read(int32_t address) {
    T value;
    std::memcpy(&value, reinterpret_cast<const void*>(static_cast<uintptr_t>(address)), sizeof(value));
    return value;
}
template<class T> void write(int32_t address, T value) {
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(address)), &value, sizeof(value));
}
// Original CDQ/XOR/SUB, including the signed comparison of the resulting bits.
int32_t magnitude(int32_t value) {
    return value < 0 ? static_cast<int32_t>(0u - static_cast<uint32_t>(value)) : value;
}
void merge_device(int32_t output, int32_t source) {
    for (int field = 0; field < 14; ++field) {
        const auto destination = output + 72 + field * 4;
        const auto current = read<int32_t>(destination);
        const auto incoming = read<int32_t>(source + 72 + field * 4);
        const bool replace = field < 2 ? magnitude(incoming) > magnitude(current) : incoming > current;
        if (replace) {
            write(destination, incoming);
            write<uint8_t>(output + 128 + field, 0);
            write<uint8_t>(output + 192, read<uint8_t>(source + 4));
        } else if (current == 0 && read<uint8_t>(source + 128 + field)) {
            write<uint8_t>(output + 128 + field, 1);
        }
    }
    for (int axis = 0; axis < 6; ++axis) {
        const auto destination = output + 144 + axis * 4;
        const float incoming = read<float>(source + 144 + axis * 4);
        if (std::fabs(incoming) > std::fabs(read<float>(destination))) write(destination, incoming);
    }
}
} // namespace

// Opaque native storage replaces all VC8 deque blocks/map/iterator arithmetic.
extern "C" void kinoko_input_cluster_construct(int32_t cluster) {
    storage(cluster) = new Devices;
}
extern "C" void kinoko_input_cluster_clear(int32_t cluster) { devices(cluster).clear(); }
extern "C" void kinoko_input_cluster_append(int32_t cluster, int32_t device) {
    devices(cluster).push_back(device);
}
extern "C" void kinoko_input_cluster_assign(int32_t destination, int32_t source) {
    if (destination != source) devices(destination) = devices(source);
}
extern "C" uint32_t kinoko_input_cluster_size(int32_t cluster) {
    return static_cast<uint32_t>(devices(cluster).size());
}
extern "C" int32_t kinoko_input_cluster_at(int32_t cluster, uint32_t index) {
    return devices(cluster).at(index);
}
extern "C" int32_t __fastcall kinoko_input_cluster_delete(int32_t cluster, void*, unsigned char flags) {
    delete storage(cluster);
    storage(cluster) = nullptr;
    // 46A7E0 -> 4074B0 resets the base identity after destroying the deque.
    write<int32_t>(cluster, kinoko::legacy::address(g35));
    if (flags & 1) std::free(kinoko::legacy::pointer<void>(cluster));
    return cluster;
}
// 4077C0: original merge rules, in registration order. Values are borrowed
// device pointers, including after Input::operator= (no implicit rebinding).
extern "C" int32_t __fastcall function_4077c0(int32_t self) {
    std::memset(reinterpret_cast<void*>(static_cast<uintptr_t>(self + 72)), 0, 96);
    const auto count = kinoko_input_cluster_size(self);
    if (!count) return self + 72;
    for (auto device : devices(self)) merge_device(self, device);
    return static_cast<int32_t>(count);
}
