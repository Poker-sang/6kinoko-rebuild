#include "kinoko/graphics_device.h"
#include "kinoko/string_layout.h"
#include "kinoko/act_frame.h"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/act_layer_access.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/texture_store.h"
#include "kinoko/render_target.h"
#include "kinoko/windows_owner.hpp"
#include <d3d9.h>

extern "C" {
void retdec_trace_i32(const char*, int32_t);
void retdec_trace_squirrel_name(const char*, int32_t);
const char* retdec_std_string_data(int32_t);
int32_t retdec_set_texture_stage(int32_t, int32_t);
}

namespace {
using namespace kinoko::act;
using namespace kinoko::legacy;
using kinoko::native::RecordView;
using RuntimeView = RecordView<RuntimeRecord>;

struct DrawLayoutPrefix {
    Address vtable;
    std::array<uint8_t, 304> unknown4;
    int32_t texture;
};
static_assert(offsetof(DrawLayoutPrefix, texture) == 308);
int32_t float_bits(float value) { return load<int32_t>(&value); }
Address method(int32_t object, unsigned index) {
    const auto table = load<Address>(pointer(object));
    return table ? load<Address>(pointer<unsigned char>(table) + index * sizeof(Address)) : 0;
}
class DrawTarget final {
    bool selected_;
public:
    DrawTarget(Address target, IDirect3DDevice9* device) : selected_(target != 0) {
        if (!selected_) return;
        const RecordView<TextureResourcePrefix> resource(pointer(target));
        kinoko_set_render_target(resource.get(&TextureResourcePrefix::texture));
        // 452636/452659 clear the selected target to opaque black before
        // checking stage/ACT visibility. The original ignores these HRESULTs.
        if (device) device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xff000000u, 1.0f, 0);
    }
    ~DrawTarget() { if (selected_) kinoko_set_render_target(0); }
    DrawTarget(const DrawTarget&) = delete;
    DrawTarget& operator=(const DrawTarget&) = delete;
};
// Original 452670..452720 / 4529FB..452A84 save these states around the
// whole ACT pass, including layout virtual calls, not only BitBlt sprites.
class DrawStates final {
    IDirect3DDevice9* device_;
    DWORD u_ = D3DTADDRESS_WRAP, v_ = D3DTADDRESS_WRAP;
    DWORD values_[4]{};
    static constexpr D3DRENDERSTATETYPE states_[4] = {
        D3DRS_SRCBLEND, D3DRS_DESTBLEND, D3DRS_BLENDOP, D3DRS_ALPHABLENDENABLE};
public:
    explicit DrawStates(IDirect3DDevice9* device) : device_(device) {
        if (!device_) return;
        device_->GetSamplerState(0, D3DSAMP_ADDRESSU, &u_);
        device_->GetSamplerState(0, D3DSAMP_ADDRESSV, &v_);
        device_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        for (int i = 0; i < 4; ++i) device_->GetRenderState(states_[i], &values_[i]);
        device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    }
    ~DrawStates() {
        if (!device_) return;
        for (int i = 0; i < 4; ++i) device_->SetRenderState(states_[i], values_[i]);
        device_->SetSamplerState(0, D3DSAMP_ADDRESSU, u_);
        device_->SetSamplerState(0, D3DSAMP_ADDRESSV, v_);
    }
    DrawStates(const DrawStates&) = delete;
    DrawStates& operator=(const DrawStates&) = delete;
};
void set_blend(IDirect3DDevice9* device, int32_t blend) {
    DWORD src = D3DBLEND_ONE, dest = D3DBLEND_ZERO, op = D3DBLENDOP_ADD;
    switch (blend) {
    case 1: src = D3DBLEND_SRCALPHA; dest = D3DBLEND_INVSRCALPHA; break;
    case 2: src = D3DBLEND_SRCALPHA; dest = D3DBLEND_ONE; break;
    case 3: src = D3DBLEND_SRCALPHA; dest = D3DBLEND_ONE; op = D3DBLENDOP_REVSUBTRACT; break;
    case 4: src = D3DBLEND_ZERO; dest = D3DBLEND_SRCCOLOR; break;
    case 5: src = D3DBLEND_DESTCOLOR; dest = D3DBLEND_ONE; break;
    }
    device->SetRenderState(D3DRS_SRCBLEND, src);
    device->SetRenderState(D3DRS_DESTBLEND, dest);
    device->SetRenderState(D3DRS_BLENDOP, op);
}
int32_t prepare_sprite(int32_t item, const BlitCommand& command) {
    if (command.texture <= 0 || static_cast<uint32_t>(command.texture) >= KINOKO_TEXTURE_CAPACITY) return E_FAIL;
    const auto& texture = kinoko_texture_slots[command.texture];
    if (!texture.width || !texture.height) return E_FAIL;
    KinokoSprite sprite{};
    sprite.vtable = const_cast<void*>(kinoko_act_host_symbols()->sprite_vtable);
    kinoko_sprite_set_rect(&sprite, nullptr, command.texture, command.source_x,
        command.source_y, command.width, command.height);
    const uint32_t color = (static_cast<uint32_t>(command.alpha * 255.0f) << 24) | 0xffffffu;
    for (auto& vertex : sprite.vertices) vertex.color = color;
    const RecordView<BlitSprite> target(pointer(item));
    target.set(&BlitSprite::command, command);
    target.set(&BlitSprite::sprite, sprite);
    return 0;
}
void trace_draw(int32_t self, const RuntimeView& resource, LONG actor_index, LONG trace_index) {
    const auto act = resource.get(&RuntimeRecord::active_document);
    const DocumentView document(act);
    if (actor_index <= 64) {
        retdec_trace_i32("4525d0:actor-index", actor_index);
        retdec_trace_i32("4525d0:flag-68", load<int32_t>(resource.bytes(&RuntimeRecord::hidden)));
        retdec_trace_i32("4525d0:flag-8", load<int32_t>(resource.bytes(&RuntimeRecord::stage_active)));
        retdec_trace_i32("4525d0:field-10", address(resource.get(&RuntimeRecord::active_holder)));
        retdec_trace_i32("4525d0:act", address(act));
        if (act) {
            retdec_trace_squirrel_name("4525d0:actor-name", address(retdec_std_string_data(address(document.bytes(&DocumentRecord::name)))));
            retdec_trace_i32("4525d0:act-60", load<int32_t>(document.bytes(&DocumentRecord::visible)));
            retdec_trace_i32("4525d0:act-begin", address(document.get(&DocumentRecord::layers).begin));
            retdec_trace_i32("4525d0:act-end", address(document.get(&DocumentRecord::layers).end));
        }
    }
    if (trace_index <= 8) {
        retdec_trace("4525d0:live-entry");
        retdec_trace_i32("4525d0:live-resource", self);
        retdec_trace_i32("4525d0:live-act", address(act));
        if (act) retdec_trace_squirrel_name("4525d0:live-act-name", address(retdec_std_string_data(address(document.bytes(&DocumentRecord::name)))));
        retdec_trace_squirrel_name("4525d0:live-resource-name", address(retdec_std_string_data(address(resource.bytes(&RuntimeRecord::name)))));
    }
}
}

extern "C" int32_t kinoko_act_prepare_draw(int32_t self) {
    if (!self) return E_FAIL;
    const RuntimeView resource(pointer(self));
    if (resource.get(&RuntimeRecord::hidden)) return 0;
    kinoko::windows::CriticalLock lock(reinterpret_cast<CRITICAL_SECTION*>(resource.bytes(&RuntimeRecord::lock)));
    const auto act = resource.get(&RuntimeRecord::active_document);
    if (!resource.get(&RuntimeRecord::stage_active) || !act) return 0;
    const DocumentView document(act);
    if (!document.get(&DocumentRecord::visible)) return 0;
    int32_t result = 0;
    const auto layers = document.get(&DocumentRecord::layers);
    for (int32_t i = layer_distance(layers) - 1; i >= 0; --i) {
        const auto layout = address(kinoko_act_layer_layout(pointer<KinokoActRuntime>(self), i));
        if (layout) {
            const auto update = method(layout, 7);
            if (update && retdec_call_thiscall0_result(pointer(layout), pointer(update)) < 0) result = E_FAIL;
        }
    }
    const auto commands = kinoko_act_command_span(self);
    const auto count = static_cast<int32_t>(commands.end - commands.begin) / 36;
    function_452c20(address(resource.bytes(&RuntimeRecord::draw_sprites)), count);
    const auto sprites = kinoko_act_sprite_span(self);
    if (static_cast<int32_t>(sprites.end - sprites.begin) / 184 != count) return E_OUTOFMEMORY;
    for (int32_t i = 0; i < count; ++i)
        if (prepare_sprite(sprites.begin + i * sizeof(BlitSprite),
                load<BlitCommand>(pointer(commands.begin + i * sizeof(BlitCommand)))) < 0) result = E_FAIL;
    return result;
}

extern "C" int32_t kinoko_act_draw(int32_t self, float x, float y) {
    auto* device = kinoko_graphics.device;
    if (!self) return E_FAIL;
    const RuntimeView resource(pointer(self));
    if (resource.get(&RuntimeRecord::hidden)) return 0;
    static volatile LONG actor_trace_count, trace_count;
    const auto actor_index = InterlockedIncrement(&actor_trace_count);
    const auto trace_index = InterlockedIncrement(&trace_count);
    trace_draw(self, resource, actor_index, trace_index);
    kinoko::windows::CriticalLock lock(reinterpret_cast<CRITICAL_SECTION*>(resource.bytes(&RuntimeRecord::lock)));
    DrawTarget target(resource.get(&RuntimeRecord::render_target), device);
    if (!resource.get(&RuntimeRecord::stage_active)) return 0;
    const auto act = resource.get(&RuntimeRecord::active_document);
    if (!act) return E_FAIL;
    const DocumentView document(act);
    if (!document.get(&DocumentRecord::visible)) return 0;
    const auto layers = document.get(&DocumentRecord::layers);
    if (!ordered_layers(layers)) return 0;
    const float draw_x = x + document.get(&DocumentRecord::offset_x);
    const float draw_y = y + document.get(&DocumentRecord::offset_y);
    DrawStates states(device);
    int32_t result = 0;
    for (int32_t i = layer_distance(layers) - 1; i >= 0; --i) {
        const auto layout = address(kinoko_act_layer_layout(pointer<KinokoActRuntime>(self), i));
        if (!layout) continue;
        const auto draw = method(layout, 8);
        if (!draw) { result = E_FAIL; continue; }
        const auto status = retdec_call_thiscall2_result(pointer(layout), pointer(draw), float_bits(draw_x), float_bits(draw_y));
        if (trace_index <= 8) {
            retdec_trace_i32("4525d0:live-x", float_bits(draw_x));
            retdec_trace_i32("4525d0:live-y", float_bits(draw_y));
            retdec_trace_i32("4525d0:live-layout", layout);
            retdec_trace_i32("4525d0:live-texture", load<Address>(pointer(layout))==address(g350)?0:RecordView<DrawLayoutPrefix>(pointer(layout)).get(&DrawLayoutPrefix::texture));
            retdec_trace_i32("4525d0:live-draw-result", status);
        }
        if (status < 0) result = status;
    }
    if (document.get(&DocumentRecord::visible)) {
        auto* blit_device = kinoko_graphics.device;
        if (blit_device) {
            for (auto item = kinoko_act_sprite_span(self).begin;
                 item != kinoko_act_sprite_span(self).end; item += sizeof(BlitSprite)) {
                const RecordView<BlitSprite> entry(pointer(item));
                const auto command = entry.get(&BlitSprite::command);
                const auto sprite = address(entry.bytes(&BlitSprite::sprite));
                set_blend(blit_device, command.blend);
                const auto draw = method(sprite, 7);
                if (draw && retdec_call_thiscall2_result(pointer(sprite), pointer(draw),
                    float_bits(draw_x + command.x), float_bits(draw_y + command.y)) < 0) result = E_FAIL;
            }
            retdec_set_texture_stage(0, 0);
        }
    }
    return result;
}
