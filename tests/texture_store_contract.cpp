#define CINTERFACE
#include "kinoko/graphics_device.h"
#include <windows.h>
#include <d3d9.h>
#include "kinoko/texture_store.h"
#include "kinoko/texture_image.h"
#include <cstdio>
#include <cstring>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #condition); return 1; } } while (0)

struct Texture {
    IDirect3DBaseTexture9Vtbl *vtable;
    unsigned references;
};
static unsigned loads, releases, unbinds, binds;
static IDirect3DBaseTexture9 *bound_textures[8];
static IDirect3DBaseTexture9 *&bound_texture = bound_textures[0];
static ULONG WINAPI release_texture(IDirect3DBaseTexture9 *raw) {
    auto *texture = reinterpret_cast<Texture *>(raw);
    const auto references = --texture->references;
    if (!references) { ++releases; delete texture; }
    return references;
}
static HRESULT WINAPI get_texture(IDirect3DDevice9 *, DWORD stage,
                                  IDirect3DBaseTexture9 **out) {
    *out = bound_textures[stage];
    if (*out) ++reinterpret_cast<Texture *>(*out)->references;
    return S_OK;
}
static HRESULT WINAPI set_texture(IDirect3DDevice9 *, DWORD stage,
                                  IDirect3DBaseTexture9 *texture) {
    ++binds;
    if (texture == bound_textures[stage]) return S_OK;
    if (bound_textures[stage]) {
        if (!texture) ++unbinds;
        release_texture(bound_textures[stage]);
    }
    bound_textures[stage] = texture;
    if (texture) ++reinterpret_cast<Texture *>(texture)->references;
    return S_OK;
}
static IDirect3DBaseTexture9Vtbl texture_vtable = {};
extern "C" {
KinokoGraphics kinoko_graphics{};
void kinoko_trace_i32(const char*,int32_t) {}
HRESULT kinoko_texture_load_image(const char *path, IDirect3DTexture9 **out,
                      uint32_t *width, uint32_t *height) {
    if (std::strstr(path, "missing")) return E_FAIL;
    auto *texture = new Texture{&texture_vtable, 1};
    *out = reinterpret_cast<IDirect3DTexture9 *>(texture);
    *width = 256; *height = 128; ++loads;
    return S_OK;
}
}

int main() {
    texture_vtable.Release = release_texture;
    IDirect3DDevice9Vtbl device_vtable = {};
    device_vtable.GetTexture = get_texture;
    device_vtable.SetTexture = set_texture;
    IDirect3DDevice9 device = {&device_vtable};
    kinoko_graphics.device = &device;
    for (unsigned cycle = 0; cycle < 5000; ++cycle) {
        const auto first = kinoko_texture_acquire("data/map/terrain.cv2");
        CHECK(first != 0);
        const auto second = kinoko_texture_acquire("DATA\\MAP\\TERRAIN.CV2");
        CHECK(second == first && loads == cycle + 1);
        CHECK(kinoko_texture_slots[first].width == 256);
        CHECK(kinoko_texture_slots[first].height == 128);
        bound_texture = static_cast<IDirect3DBaseTexture9 *>(kinoko_texture_slots[first].texture);
        ++reinterpret_cast<Texture *>(bound_texture)->references;
        CHECK(kinoko_texture_release(first) == 1);
        CHECK(releases == cycle && unbinds == cycle);
        CHECK(kinoko_texture_slots[second].texture == bound_texture);
        CHECK(kinoko_texture_release(second) == 1);
        CHECK(!bound_texture && releases == cycle + 1 && unbinds == cycle + 1);
        CHECK(!kinoko_texture_slots[first].texture);
        CHECK(kinoko_texture_release(first) == 0);
    }
    CHECK(kinoko_texture_acquire("data/missing.cv2") == 0);
    CHECK(kinoko_texture_acquire("data/missing.cv2") == 0);
    CHECK(kinoko_texture_release(0) == 0 && kinoko_texture_release(-1) == 0);
    CHECK(kinoko_texture_release(KINOKO_TEXTURE_CAPACITY) == 0);
    // The same texture can be bound more than once; an unrelated stage must
    // keep both its device reference and its store reference.
    const auto shared = kinoko_texture_acquire("shared.cv2");
    const auto other = kinoko_texture_acquire("other.cv2");
    CHECK(shared && other && shared != other);
    auto *shared_raw = static_cast<IDirect3DBaseTexture9 *>(kinoko_texture_slots[shared].texture);
    auto *other_raw = static_cast<IDirect3DBaseTexture9 *>(kinoko_texture_slots[other].texture);
    bound_textures[1] = bound_textures[7] = shared_raw;
    reinterpret_cast<Texture *>(shared_raw)->references += 2;
    bound_textures[3] = other_raw;
    ++reinterpret_cast<Texture *>(other_raw)->references;
    const auto before_release = releases, before_unbind = unbinds;
    CHECK(kinoko_texture_release(shared) == 1);
    CHECK(releases == before_release + 1 && unbinds == before_unbind + 2);
    CHECK(!bound_textures[1] && !bound_textures[7] && bound_textures[3] == other_raw);
    CHECK(reinterpret_cast<Texture *>(other_raw)->references == 2);
    CHECK(kinoko_texture_release(other) == 1);
    CHECK(releases == before_release + 2 && unbinds == before_unbind + 3);

    // Exercise the real cache with final release and handle-slot reuse. A new
    // texture at the same handle must not inherit the retired binding key.
    kinoko_initialize_texture_cache();
    const auto cached = kinoko_texture_acquire("cached.cv2");
    auto before_binds = binds;
    CHECK(kinoko_texture_bind_stage(0,cached) == S_OK);
    CHECK(kinoko_texture_bind_stage(7,cached) == S_OK);
    CHECK(kinoko_texture_bind_stage(0,cached) == cached && binds==before_binds+2);
    CHECK(kinoko_texture_release(cached)==1 && !bound_textures[0] && !bound_textures[7]);
    const auto reused = kinoko_texture_acquire("cached.cv2");
    CHECK(reused==cached);
    before_binds=binds;
    CHECK(kinoko_texture_bind_stage(0,reused)==S_OK && binds==before_binds+1);
    CHECK(kinoko_texture_release(reused)==1 && !bound_textures[0]);

    // A full store must release a successfully loaded, but unregistered,
    // texture exactly once. Existing slots must stay owned and unchanged.
    int32_t handles[KINOKO_TEXTURE_CAPACITY - 1]{};
    for (auto &handle : handles) {
        auto *raw = new Texture{&texture_vtable, 1};
        handle = kinoko_texture_register(reinterpret_cast<IDirect3DBaseTexture9*>(raw), 1, 1);
        CHECK(handle != 0);
    }
    const auto full_releases = releases, full_loads = loads;
    CHECK(kinoko_texture_acquire("full.cv2") == 0);
    CHECK(releases == full_releases + 1 && loads == full_loads + 1);
    for (auto handle : handles) CHECK(kinoko_texture_release(handle) == 1);
    CHECK(releases == full_releases + KINOKO_TEXTURE_CAPACITY);
    std::puts("PASS: texture ownership, shared names, all-stage unbind, unrelated borrows, capacity failure and reuse");
}
