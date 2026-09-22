#include "kinoko/texture_store.h"
#include "kinoko/graphics_device.h"
#include <array>

extern "C" void retdec_trace_i32(const char*,int32_t);
namespace {
// 4059C0 initializes eight consecutive borrowed handles at original 51AEE8.
// They are cache keys only; ownership stays with the store and D3D device.
std::array<int32_t,KINOKO_TEXTURE_STAGE_COUNT> stage_handles{};
}
extern "C" void kinoko_initialize_texture_cache(void) { stage_handles.fill(0); }
extern "C" void kinoko_texture_forget_bindings(int32_t handle) {
    for (auto& cached:stage_handles) if (cached==handle) cached=0;
}
extern "C" int32_t kinoko_texture_bind_stage(int32_t stage,int32_t handle) {
    auto* device=kinoko_graphics.device;
    // Existing invalid-device/handle guards remain reconstruction boundaries.
    // Eight stages correspond to the recovered cache, not an unbounded offset.
    if (stage<0 || stage>=KINOKO_TEXTURE_STAGE_COUNT || !device ||
        !*reinterpret_cast<void***>(device)) return E_FAIL;
    auto& cached=stage_handles[stage];
    if (!handle) {
        device->SetTexture(static_cast<DWORD>(stage),nullptr);
        cached=0;
        return 0;
    }
    if (handle<0 || handle>=KINOKO_TEXTURE_CAPACITY || !kinoko_texture_slots[handle].texture) {
        retdec_trace_i32("texture:unresolved-handle",handle);
        return E_FAIL;
    }
    if (handle==cached) return handle;
    const auto result=device->SetTexture(static_cast<DWORD>(stage),kinoko_texture_slots[handle].texture);
    cached=handle; // 405E94 writes unconditionally, including a failed HRESULT.
    return result;
}
