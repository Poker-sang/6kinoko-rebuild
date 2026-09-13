#include "kinoko/texture_store.h"

#include <array>
#include <string>
#include <windows.h>
#include <d3d9.h>

extern "C" {
extern int32_t g678;
int32_t function_40e630(int32_t unused, const char *path, int32_t texture_out,
                      uint32_t *width, uint32_t *height);
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY] = {};
}

namespace {
struct Ownership {
    std::string name;
    uint32_t references = 0;
};
std::array<Ownership, KINOKO_TEXTURE_CAPACITY> owners;

bool valid(int32_t handle) {
    return handle > 0 && handle < KINOKO_TEXTURE_CAPACITY &&
           owners[handle].references != 0;
}

// Original 406370 canonicalizes the resource name before the reference lookup.
std::string resource_key(const char *path) {
    std::string key(path);
    for (char &character : key)
        if (character == '\\') character = '/';
    CharLowerBuffA(key.data(), static_cast<DWORD>(key.size()));
    return key;
}
}

extern "C" int32_t kinoko_texture_register(
    void *texture, uint32_t width, uint32_t height) {
    if (!texture) return 0;
    for (int32_t handle = 1; handle < KINOKO_TEXTURE_CAPACITY; ++handle) {
        if (owners[handle].references) continue;
        owners[handle].references = 1;
        kinoko_texture_slots[handle] = {texture, width, height};
        return handle;
    }
    return 0;
}

extern "C" int32_t kinoko_texture_acquire(const char *path) {
    if (!path || !*path) return 0;
    try {
        auto key = resource_key(path);
        for (int32_t handle = 1; handle < KINOKO_TEXTURE_CAPACITY; ++handle) {
            auto &owner = owners[handle];
            if (owner.references && owner.name == key) {
                ++owner.references;
                return handle;
            }
        }
        int32_t texture_value = 0;
        uint32_t width = 0, height = 0;
        if (function_40e630(0, path, reinterpret_cast<int32_t>(&texture_value),
                           &width, &height) < 0 || !texture_value)
            return 0;
        auto *texture = reinterpret_cast<IDirect3DBaseTexture9 *>(texture_value);
        const auto handle = kinoko_texture_register(texture, width, height);
        if (!handle) {
            texture->Release();
            return 0;
        }
        owners[handle].name.swap(key);
        return handle;
    } catch (...) {
        // Do not let C++ allocation exceptions cross the reconstructed C ABI.
        return 0;
    }
}

extern "C" int32_t kinoko_texture_release(int32_t handle) {
    if (!valid(handle)) return 0;
    auto &owner = owners[handle];
    if (--owner.references) return 1;

    auto &slot = kinoko_texture_slots[handle];
    auto *texture = static_cast<IDirect3DBaseTexture9 *>(slot.texture);
    auto *device = reinterpret_cast<IDirect3DDevice9 *>(g678);
    // 405D60 unbinds a final reference from all eight texture stages first.
    if (device) {
        for (DWORD stage = 0; stage < 8; ++stage) {
            IDirect3DBaseTexture9 *bound = nullptr;
            if (SUCCEEDED(device->GetTexture(stage, &bound)) && bound) {
                if (bound == texture) device->SetTexture(stage, nullptr);
                bound->Release();
            }
        }
    }
    texture->Release();
    slot = {};
    owner.name.clear();
    return 1;
}

extern "C" int32_t kinoko_texture_retain(int32_t handle) {
    if (!valid(handle)) return 0;
    ++owners[handle].references;
    return 1;
}
