#include <cstdint>
#include <cstring>
#include <cmath>

namespace {
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

// 4077C0: InputCluster::Update. The original MSVC deque stores four pointers
// per block. Its iterator expansion accounted for most of the decompiled body.
extern "C" int32_t __fastcall function_4077c0(int32_t self) {
    std::memset(reinterpret_cast<void*>(static_cast<uintptr_t>(self + 72)), 0, 96);
    const auto count = read<uint32_t>(self + 184);
    if (!count) return self + 72;
    const auto map = read<int32_t>(self + 172);
    const auto map_size = read<uint32_t>(self + 176);
    const auto first = read<uint32_t>(self + 180);
    for (uint32_t index = 0; index < count; ++index) {
        const auto slot = first + index;
        auto block_index = slot / 4;
        if (block_index >= map_size) block_index -= map_size;
        const auto block = read<int32_t>(map + 4 * block_index);
        merge_device(self, read<int32_t>(block + 4 * (slot & 3)));
    }
    return static_cast<int32_t>(count);
}
