#define CINTERFACE
#include <windows.h>
#include <d3d9.h>
#include "kinoko/texture_store.h"
#include <cstdio>
#include <cstring>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #condition); return 1; } } while (0)

struct Texture {
    IDirect3DBaseTexture9Vtbl *vtable;
    unsigned references;
};
static unsigned loads, releases, unbinds;
static IDirect3DBaseTexture9 *bound_texture;
static ULONG WINAPI release_texture(IDirect3DBaseTexture9 *raw) {
    auto *texture = reinterpret_cast<Texture *>(raw);
    const auto references = --texture->references;
    if (!references) { ++releases; delete texture; }
    return references;
}
static HRESULT WINAPI get_texture(IDirect3DDevice9 *, DWORD stage,
                                  IDirect3DBaseTexture9 **out) {
    *out = stage == 0 ? bound_texture : nullptr;
    if (*out) ++reinterpret_cast<Texture *>(*out)->references;
    return S_OK;
}
static HRESULT WINAPI set_texture(IDirect3DDevice9 *, DWORD stage,
                                  IDirect3DBaseTexture9 *texture) {
    if (stage == 0 && !texture && bound_texture) {
        ++unbinds;
        release_texture(bound_texture);
        bound_texture = nullptr;
    }
    return S_OK;
}
static IDirect3DBaseTexture9Vtbl texture_vtable = {};
extern "C" {
int32_t g678 = 0;
int32_t function_40e630(int32_t, const char *path, int32_t out,
                      uint32_t *width, uint32_t *height) {
    if (std::strstr(path, "missing")) return E_FAIL;
    auto *texture = new Texture{&texture_vtable, 1};
    *reinterpret_cast<Texture **>(out) = texture;
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
    g678 = reinterpret_cast<int32_t>(&device);
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
    std::puts("PASS: 5000 shared texture cycles; one load, retained owner, final unbind/release, slot reuse, failed loads");
}
