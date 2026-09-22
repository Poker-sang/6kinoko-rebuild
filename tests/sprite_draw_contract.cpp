#include "kinoko/sprite.h"
#include "kinoko/graphics_device.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
extern "C" {
KinokoGraphics kinoko_graphics{};
float function_404130(long double a) { return static_cast<float>(std::cos(a)); }
float function_4040d0(long double a) { return static_cast<float>(std::sin(a)); }
}
static int step, bound;
static DWORD fvf;
static const void *submitted;
static bool valid;
extern "C" int32_t kinoko_texture_bind_stage(int32_t stage, int32_t texture) {
    valid &= step++ == 0 && stage == 0;
    bound = texture;
    return E_FAIL; // Original still sets FVF and draws after a failed bind.
}
static HRESULT WINAPI set_fvf(IDirect3DDevice9 *, DWORD value) {
    valid &= step++ == 1;
    fvf = value;
    return E_FAIL;
}
static HRESULT WINAPI draw_up(IDirect3DDevice9 *, D3DPRIMITIVETYPE type,
    UINT count, const void *vertices, UINT stride) {
    valid &= step++ == 2 && type == D3DPT_TRIANGLESTRIP && count == 2 && stride == 28;
    submitted = vertices;
    return S_FALSE;
}
int main() {
    void *vtable[119]{};
    vtable[89] = reinterpret_cast<void *>(set_fvf);
    vtable[83] = reinterpret_cast<void *>(draw_up);
    void **device = vtable;
    kinoko_graphics.device = reinterpret_cast<IDirect3DDevice9 *>(&device);
    KinokoSprite sprite{};
    sprite.texture = 17;
    sprite.pivot_x = 99; sprite.scale_x = 7; sprite.angle = 45;
    for (auto &v : sprite.vertices) { v.z = .25f; v.rhw = 1; v.color = 0x12345678; v.u = .125f; v.v = .75f; }
    using Bounds = int32_t (__thiscall *)(KinokoSprite *, float, float, float, float);
    valid = true;
    CHECK(reinterpret_cast<Bounds>(kinoko_sprite_draw_bounds)(&sprite, 10, 20, 5, -3) == S_FALSE);
    CHECK(valid && step == 3 && bound == 17 && fvf == 324 && submitted == sprite.vertices);
    CHECK(sprite.vertices[0].x == 9.5f && sprite.vertices[0].y == 19.5f);
    CHECK(sprite.vertices[1].x == 4.5f && sprite.vertices[1].y == 19.5f);
    CHECK(sprite.vertices[2].x == 9.5f && sprite.vertices[2].y == -3.5f);
    CHECK(sprite.vertices[3].x == 4.5f && sprite.vertices[3].y == -3.5f);
    for (auto &v : sprite.vertices) CHECK(v.z == .25f && v.rhw == 1 && v.color == 0x12345678 && v.u == .125f && v.v == .75f);
    using Draw = int32_t (__thiscall *)(KinokoSprite *, float, float);
    const Draw methods[] = {reinterpret_cast<Draw>(kinoko_sprite_draw_404770), reinterpret_cast<Draw>(kinoko_sprite_draw_4049c0), reinterpret_cast<Draw>(kinoko_sprite_draw_404bc0)};
    sprite.angle = 0;
    for (int i = 0; i < 3; ++i) {
        step = 0;
        CHECK(methods[i](&sprite, 0, 0) == S_FALSE);
        CHECK(valid && step == 3 && fvf == (i == 2 ? 16706u : 324u));
    }
    std::puts("PASS: sprite thiscall bounds, retained attributes and original draw failure ordering");
}
