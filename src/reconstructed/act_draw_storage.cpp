#include "kinoko/act_frame.h"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/legacy_memory.hpp"
#include <algorithm>
#include <vector>
#include <new>

extern "C" void retdec_trace_i32(const char*, int32_t);

namespace {
using namespace kinoko::act;
using namespace kinoko::legacy;
using kinoko::native::RecordView;
using Commands=std::vector<BlitCommand>;
struct Sprite {
    BlitSprite value{};
    Sprite() { value.sprite.vtable=const_cast<void*>(kinoko_act_host_symbols()->sprite_vtable); }
    ~Sprite() { value.sprite.vtable=const_cast<void*>(kinoko_act_host_symbols()->color_vtable); }
};
static_assert(sizeof(Sprite)==sizeof(BlitSprite));
using Sprites=std::vector<Sprite>;
template<class T> T*& owner(int32_t slot) { return field<T*>(slot); }
template<class T> T& ensure(int32_t slot) { auto& p=owner<T>(slot);if(!p) p=new T;return *p; }
template<class T> KinokoDrawSpan span(T* values) {
    if(!values) return {};
    const auto begin=static_cast<uint32_t>(address(values->data()));
    return {begin,begin+static_cast<uint32_t>(values->size()*sizeof(typename T::value_type)),
        begin+static_cast<uint32_t>(values->capacity()*sizeof(typename T::value_type))};
}
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
    const BlitCommand command{blend, alpha < 0 ? 0 : alpha > 1 ? 1 : alpha,
        static_cast<float>(x), static_cast<float>(y), sx, sy, width, height,
        texture.get(&TextureResourcePrefix::texture)};
    try { ensure<Commands>(self+44).push_back(command); }
    catch(const std::bad_alloc&) { return E_OUTOFMEMORY; }
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
    if(requested>0x1642c85u) return 0;
    try {
        auto& values=ensure<Sprites>(address_value);
        const auto capacity=static_cast<int32_t>(values.capacity());
        values.resize(requested);return capacity;
    } catch(const std::bad_alloc&) { return 0; }
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
    auto* values=owner<Sprites>(address_value);
    if(!values) return 0;
    const auto begin=address(values->data());values->clear();return begin;
}

extern "C" KinokoDrawSpan kinoko_act_command_span(int32_t resource) { return span(owner<Commands>(resource+44)); }
extern "C" KinokoDrawSpan kinoko_act_sprite_span(int32_t resource) { return span(owner<Sprites>(resource+60)); }
extern "C" void kinoko_act_commands_clear(int32_t resource) { if(owner<Commands>(resource+44)) owner<Commands>(resource+44)->clear(); }
extern "C" void kinoko_act_draw_storage_destroy(int32_t resource) {
    delete owner<Sprites>(resource+60);owner<Sprites>(resource+60)=nullptr;
    delete owner<Commands>(resource+44);owner<Commands>(resource+44)=nullptr;
}
