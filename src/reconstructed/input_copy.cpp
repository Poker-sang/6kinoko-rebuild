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
struct Deque { Address proxy, map; uint32_t map_size, first, size; };
struct Device { Address vtable; unsigned char value[164]; };
static_assert(sizeof(Device) == 168 && sizeof(Deque) == 20);

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
Address& deque_slot(const Deque& q, uint32_t offset) {
    const auto block = (offset / 4) % q.map_size;
    return pointer<Address>(pointer<Address>(q.map)[block])[offset % 4];
}
void clear(Deque& q) {
    auto* map = pointer<Address>(q.map);
    for (auto i = q.map_size; i; --i) std::free(pointer<void>(map[i - 1]));
    std::free(map);
    q.map = q.map_size = q.first = q.size = 0;
}
void grow(Deque& q) {
    // 46C220 grows by max(old_size / 2, 8) and keeps the first offset.
    const auto increment = (std::max)(q.map_size / 2, 8u);
    if (q.map_size > 0x0fffffff - increment) throw std::length_error("deque<T> too long");
    const auto count = q.map_size + increment;
    const auto storage = allocate(count * sizeof(Address));
    auto* target = pointer<Address>(storage);
    auto* source = pointer<Address>(q.map);
    const auto start = q.first / 4;
    for (uint32_t i = 0; i < q.map_size; ++i) {
        const auto old_slot = (start + i) % q.map_size;
        target[(start + i) % count] = source[old_slot];
    }
    std::free(source);
    q.map = storage;
    q.map_size = count;
}
void append(Deque& q, Address value) {
    const auto end = q.first + q.size;
    if (end % 4 == 0 && q.map_size <= (q.size + 4) / 4) grow(q);
    const auto block = (end / 4) % q.map_size;
    auto& storage = pointer<Address>(q.map)[block];
    if (!storage) storage = allocate(4 * sizeof(Address));
    pointer<Address>(storage)[end % 4] = value;
    ++q.size;
}
void assign_deque(Deque& out, const Deque& in) {
    if (!in.size) { clear(out); return; }
    const auto common = (std::min)(out.size, in.size);
    for (uint32_t i = 0; i < common; ++i)
        deque_slot(out, out.first + i) = deque_slot(in, in.first + i);
    for (uint32_t i = common; i < in.size; ++i) append(out, deque_slot(in, in.first + i));
    // Erasing trailing pointer elements preserves allocated blocks and proxy.
    out.size = in.size;
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
    assign_deque(*reinterpret_cast<Deque*>(out + 364), *reinterpret_cast<const Deque*>(in + 364));
    out[388] = in[388];
    std::copy_n(in + 392, 1024, out + 392);
    assign_bytes(*reinterpret_cast<Vector*>(out + 1416), *reinterpret_cast<const Vector*>(in + 1416));
    std::copy_n(in + 1432, 3, out + 1432);
    std::copy_n(in + 1436, 76, out + 1436);
    return destination;
}
