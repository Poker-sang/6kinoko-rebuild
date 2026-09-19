#include "kinoko/act_frame.h"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/legacy_memory.hpp"
#include <algorithm>

extern "C" void retdec_trace_i32(const char*, int32_t);

namespace {
using namespace kinoko::act;
using namespace kinoko::legacy;
using kinoko::native::RecordView;
using VectorView = RecordView<VectorStorage>;
constexpr uint32_t stride = sizeof(BlitSprite);
}

extern "C" int32_t retdec_act_bitblt_this(int32_t self, int32_t x, int32_t y,
    int32_t width, int32_t height, int32_t texture_resource, int32_t sx, int32_t sy,
    int32_t blend, float alpha) {
    if (!self || !texture_resource) return E_FAIL;
    const RecordView<TextureResourcePrefix> texture(pointer(texture_resource));
    const auto* symbols = kinoko_act_host_symbols();
    const auto type = texture.get(&TextureResourcePrefix::vtable);
    if (type != static_cast<Address>(address(symbols->texture_resource_vtable)) &&
        type != static_cast<Address>(address(symbols->render_target_vtable))) return E_FAIL;
    const auto vector = RecordView<RuntimeRecord>(pointer(self)).view(&RuntimeRecord::draw_commands);
    auto storage = vector.load();
    const size_t count = storage.begin ? (storage.end - storage.begin) / sizeof(BlitCommand) : 0;
    const size_t capacity = storage.begin ? (storage.capacity - storage.begin) / sizeof(BlitCommand) : 0;
    if (count == capacity) {
        const size_t next = capacity ? capacity + capacity / 2 + 1 : 1;
        const auto replacement = std::realloc(pointer(storage.begin), next * sizeof(BlitCommand));
        if (!replacement) return E_OUTOFMEMORY;
        storage.begin = address(replacement);
        storage.capacity = storage.begin + next * sizeof(BlitCommand);
    }
    const BlitCommand command{blend, alpha < 0 ? 0 : alpha > 1 ? 1 : alpha,
        static_cast<float>(x), static_cast<float>(y), sx, sy, width, height,
        texture.get(&TextureResourcePrefix::texture)};
    store(pointer(storage.begin + count * sizeof(BlitCommand)), command);
    storage.end = storage.begin + (count + 1) * sizeof(BlitCommand);
    store(vector.data(), storage);
    static volatile LONG trace_count;
    if (InterlockedIncrement(&trace_count) <= 12) {
        retdec_trace_i32("act:bitblt-texture", command.texture);
        retdec_trace_i32("act:bitblt-x", x);
        retdec_trace_i32("act:bitblt-y", y);
    }
    return 0;
}

extern "C" int32_t function_452c20(int32_t address_value, uint32_t requested) {
    if (!address_value) return 0;
    const VectorView vector(pointer(address_value));
    auto storage = vector.load();
    const auto begin = static_cast<int32_t>(storage.begin);
    const auto finish = static_cast<int32_t>(storage.end);
    const auto capacity_end = static_cast<int32_t>(storage.capacity);
    const uint32_t size = begin && finish >= begin ? (finish - begin) / stride : 0;
    const uint32_t capacity = begin && capacity_end >= begin ? (capacity_end - begin) / stride : 0;
    if (requested <= capacity) {
        vector.set(&VectorStorage::end, storage.begin + requested * stride);
        return capacity;
    }
    if (requested > 0x1642c85u || requested > SIZE_MAX / stride) return 0;
    const size_t bytes = requested * stride;
    Allocation<unsigned char> replacement(static_cast<unsigned char*>(std::malloc(bytes)));
    if (!replacement) return 0;
    std::memset(replacement.get(), 0, bytes);
    if (begin && size && size <= requested) std::memcpy(replacement.get(), pointer(begin), size * stride);
    for (uint32_t index = size; index < requested; ++index)
        RecordView<BlitSprite>(replacement.get() + index * stride).view(&BlitSprite::sprite)
            .set(&KinokoSprite::vtable, const_cast<void*>(kinoko_act_host_symbols()->sprite_vtable));
    storage.begin = address(replacement.release());
    storage.end = storage.capacity = storage.begin + bytes;
    store(vector.data(), storage);
    std::free(pointer(begin));
    retdec_trace_i32("452c20:vector", address_value);
    retdec_trace_i32("452c20:requested", requested);
    retdec_trace_i32("452c20:replacement", storage.begin);
    return capacity;
}

// Original 455230 is element assignment, not raw memcpy: retain each
// destination CSprite vtable while copying command, texture, vertices and pose.
extern "C" int32_t function_455230(int32_t first, int32_t last, int32_t output) {
    while (first != last) {
        const RecordView<BlitSprite> destination(pointer(output));
        auto value = load<BlitSprite>(pointer(first));
        value.sprite.vtable = destination.view(&BlitSprite::sprite).get(&KinokoSprite::vtable);
        store(destination.data(), value);
        first += stride;
        output += stride;
    }
    return output;
}

extern "C" int32_t retdec_act_clear_layout_vector(int32_t address_value) {
    if (!address_value) return 0;
    const VectorView vector(pointer(address_value));
    const auto storage = vector.load();
    // The old erase(end,end,begin) call is a no-op. Destruction resets IColor
    // identity; textures are borrowed and vector capacity remains allocated.
    for (auto item = storage.begin; item != storage.end; item += stride)
        RecordView<BlitSprite>(pointer(item)).view(&BlitSprite::sprite)
            .set(&KinokoSprite::vtable, const_cast<void*>(kinoko_act_host_symbols()->color_vtable));
    vector.set(&VectorStorage::end, storage.begin);
    return storage.begin;
}
