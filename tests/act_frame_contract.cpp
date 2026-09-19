// Exercise the actual update body with a deterministic WinMM clock. Container
// callbacks and D3D are controlled boundaries, not replacements for the loop.
#define CINTERFACE
#include <windows.h>
#include <mmsystem.h>
#include <d3d9.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>
#include "kinoko/act_draw_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/texture_store.h"
static DWORD frame_clock;
static DWORD WINAPI test_timeGetTime() { return frame_clock; }
#define timeGetTime test_timeGetTime
#include "../src/reconstructed/act_frame_update.cpp"
#undef timeGetTime

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #condition); return 1; } } while (0)
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::act::BlitCommand;
using kinoko::act::BlitSprite;

namespace test {
struct Fixture {
    // Independent native ABI fixtures; offsets do not use production schemas.
    int32_t runtime[48]{}, source[60]{}, active[60]{};
    int32_t a[62]{}, b[62]{}, c[62]{}, layers[3]{};
    int32_t source_holder, active_holder;
    int32_t heads[3][3]{}, nodes[3][3]{}, keys[3][2]{}, layouts[3][80]{};
    Fixture() : source_holder(address(source)), active_holder(address(active)) {
        runtime[0] = address(&source_holder); runtime[2] = 1;
        runtime[3] = address(active); runtime[4] = address(&active_holder);
        source[34] = 0x1000001; active[24] = 1;
        source[52] = active[52] = address(layers);
        layers[0] = address(a); layers[1] = address(b); layers[2] = address(c);
        count(2);
        int32_t* items[] = {a, b, c};
        for (int i = 0; i < 3; ++i) {
            items[i][36] = 100 + i; items[i][37] = 200 + i; items[i][38] = 300 + i;
            items[i][60] = 0x1000001;
            items[i][45] = address(heads[i]); items[i][46] = 1;
            heads[i][0] = address(nodes[i]); nodes[i][0] = address(heads[i]);
            nodes[i][2] = address(keys[i]); keys[i][1] = address(layouts[i]);
        }
        InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(runtime + 5));
    }
    void count(int count) { source[53] = active[53] = address(layers + count); }
    ~Fixture() {
        std::free(pointer(runtime[11])); std::free(pointer(runtime[15]));
        DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(runtime + 5));
    }
};
static Fixture* current;
enum Mutation { none, append, shrink, deactivate, replace_source, fail };
static Mutation mutation;
static std::vector<int32_t> callbacks, draws;
static int32_t replacement_source[60], replacement_holder;
static bool allocation_fails;
static int32_t draw_result;
static DWORD render_states[256], sampler_states[3];
static DWORD observed_blends[6][3];
static int sprite_draws, texture_unbinds;
static bool draw_state_valid;
static float observed_x, observed_y;
static int32_t sprite_vtable[10], layout_vtable[9];
static char texture_identity, target_identity, color_identity;
static KinokoActHostSymbols symbols{};
static IDirect3DDevice9Vtbl device_vtable{};
static IDirect3DDevice9 device{&device_vtable};
static HRESULT WINAPI get_render(IDirect3DDevice9*, D3DRENDERSTATETYPE state, DWORD* out) {
    *out = render_states[state]; return S_OK;
}
static HRESULT WINAPI set_render(IDirect3DDevice9*, D3DRENDERSTATETYPE state, DWORD value) {
    render_states[state] = value; return S_OK;
}
static HRESULT WINAPI get_sampler(IDirect3DDevice9*, DWORD, D3DSAMPLERSTATETYPE state, DWORD* out) {
    *out = sampler_states[state]; return S_OK;
}
static HRESULT WINAPI set_sampler(IDirect3DDevice9*, DWORD, D3DSAMPLERSTATETYPE state, DWORD value) {
    sampler_states[state] = value; return S_OK;
}
static int32_t __fastcall draw_layout(void* object, void*, int32_t x, int32_t y) {
    draws.push_back(address(object));
    observed_x = kinoko::legacy::load<float>(&x); observed_y = kinoko::legacy::load<float>(&y);
    draw_state_valid &= sampler_states[1] == D3DTADDRESS_CLAMP && sampler_states[2] == D3DTADDRESS_CLAMP;
    draw_state_valid &= render_states[D3DRS_ALPHABLENDENABLE] == TRUE;
    // A layout can mutate blend state. The original outer pass must restore it.
    render_states[D3DRS_SRCBLEND] = 99;
    return draw_result;
}
static int32_t __fastcall prepare_layout(void* object, void*) {
    draws.push_back(address(object)); return draw_result;
}
static int32_t __fastcall draw_sprite(void*, void*, int32_t x, int32_t y) {
    if (sprite_draws < 6) {
        observed_blends[sprite_draws][0] = render_states[D3DRS_SRCBLEND];
        observed_blends[sprite_draws][1] = render_states[D3DRS_DESTBLEND];
        observed_blends[sprite_draws][2] = render_states[D3DRS_BLENDOP];
    }
    ++sprite_draws;
    observed_x = kinoko::legacy::load<float>(&x); observed_y = kinoko::legacy::load<float>(&y);
    return draw_result;
}
static void setup_device() {
    device_vtable.GetRenderState = get_render; device_vtable.SetRenderState = set_render;
    device_vtable.GetSamplerState = get_sampler; device_vtable.SetSamplerState = set_sampler;
    render_states[D3DRS_SRCBLEND] = 41; render_states[D3DRS_DESTBLEND] = 42;
    render_states[D3DRS_BLENDOP] = 43; render_states[D3DRS_ALPHABLENDENABLE] = FALSE;
    sampler_states[1] = D3DTADDRESS_WRAP; sampler_states[2] = D3DTADDRESS_MIRROR;
    draw_state_valid = true;
}
static bool restored() {
    return render_states[D3DRS_SRCBLEND] == 41 && render_states[D3DRS_DESTBLEND] == 42 &&
        render_states[D3DRS_BLENDOP] == 43 && render_states[D3DRS_ALPHABLENDENABLE] == FALSE &&
        sampler_states[1] == D3DTADDRESS_WRAP && sampler_states[2] == D3DTADDRESS_MIRROR;
}
}

extern "C" {
int32_t g678;
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY]{};
const KinokoActHostSymbols* kinoko_act_host_symbols() { return &test::symbols; }
int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size) {
    return test::allocation_fails ? 0 : address(std::malloc(size));
}
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
const char* retdec_std_string_data(int32_t) { return "fixture"; }
int32_t retdec_set_texture_stage(int32_t stage, int32_t handle) {
    if (!stage && !handle) ++test::texture_unbinds; return 0;
}
int32_t function_415810_this(int32_t callback) {
    using namespace test;
    callbacks.push_back(callback);
    if (callback == address(current->a + 57)) {
        if (mutation == append) current->count(3);
        if (mutation == shrink) current->count(1);
        if (mutation == deactivate) current->runtime[2] = 0;
        if (mutation == replace_source) current->runtime[0] = address(&replacement_holder);
        if (mutation == fail) return -1;
    }
    return 0;
}
}

static int test_update() {
    using namespace test;
    Fixture fixture; current = &fixture;
    fixture.a[60] = fixture.b[60] = fixture.c[60] = 1;
    // Original JNB comparison, rather than a new wrap-aware scheduling rule.
    const DWORD deadlines[][3] = {
        {100,100,0}, {100,99,1}, {0x7fffffffu,0x80000000u,0},
        {0x80000000u,0x7fffffffu,1}, {0xffffffffu,0,1}, {0,0xffffffffu,0}};
    for (const auto& sample : deadlines) {
        frame_clock = sample[0]; fixture.runtime[25] = sample[1]; callbacks.clear();
        CHECK(function_451640(address(fixture.runtime)) == 0);
        CHECK(callbacks.size() == (sample[2] ? 2u : 0u));
    }
    frame_clock = 100; fixture.runtime[25] = 0;
    for (const auto mode : {append, shrink, deactivate, replace_source, fail}) {
        fixture.count(2); fixture.runtime[2] = 1; fixture.runtime[0] = address(&fixture.source_holder);
        std::memcpy(replacement_source, fixture.source, sizeof(replacement_source));
        replacement_source[53] = replacement_source[52] + 4;
        replacement_holder = address(replacement_source);
        callbacks.clear(); mutation = mode;
        CHECK(function_451640(address(fixture.runtime)) == (mode == fail ? E_FAIL : 0));
        CHECK(callbacks.size() == (mode == append ? 3u : 1u));
        CHECK(fixture.a[42] == 100 && fixture.a[43] == 200 && fixture.a[44] == 300);
        // LeaveCriticalSection ran on every failure/early-return path.
        CHECK(reinterpret_cast<CRITICAL_SECTION*>(fixture.runtime + 5)->RecursionCount == 0);
    }
    mutation = none; fixture.count(2); fixture.runtime[2] = 1;
    fixture.runtime[0] = address(&fixture.source_holder);
    allocation_fails = true; callbacks.clear();
    CHECK(function_451640(address(fixture.runtime)) == 0 && callbacks.empty());
    allocation_fails = false;
    fixture.runtime[26] = 1;
    CHECK(function_451640(address(fixture.runtime)) == 0 && callbacks.empty());
    fixture.runtime[26] = 0; fixture.runtime[2] = 0;
    CHECK(function_451640(address(fixture.runtime)) == E_FAIL);
    return 0;
}

static int test_storage_and_draw() {
    using namespace test;
    Fixture fixture; current = &fixture; fixture.count(2);
    for (auto& layout : fixture.layouts) layout[0] = address(layout_vtable);
    kinoko::legacy::store(fixture.active + 22, 10.0f);
    kinoko::legacy::store(fixture.active + 23, 20.0f);
    int32_t texture_resource[18]{};
    texture_resource[0] = address(&texture_identity); texture_resource[17] = 1;
    kinoko_texture_slots[1].width = 128; kinoko_texture_slots[1].height = 64;
    for (int blend = 0; blend < 6; ++blend)
        CHECK(retdec_act_bitblt_this(address(fixture.runtime), 3, 4, 32, 16,
            address(texture_resource), 8, 4, blend, blend == 0 ? -1.0f : blend == 5 ? 2.0f : 0.5f) == 0);
    const auto* commands = pointer<BlitCommand>(fixture.runtime[11]);
    CHECK(commands[0].alpha == 0 && commands[5].alpha == 1 && commands[2].alpha == 0.5f);
    draws.clear(); draw_result = 0;
    CHECK(function_4522f0(address(fixture.runtime)) == 0);
    CHECK(draws.size() == 2 && draws[0] == address(fixture.layouts[1]) && draws[1] == address(fixture.layouts[0]));
    auto* sprites = pointer<BlitSprite>(fixture.runtime[15]);
    CHECK(fixture.runtime[16] - fixture.runtime[15] == 6 * 184);
    CHECK(sprites[2].sprite.vertices[0].color == 0x7fffffffu);
    CHECK(sprites[2].sprite.vertices[0].u == 0.0625f && sprites[2].sprite.vertices[3].v == 0.3125f);
    CHECK(sprites[2].sprite.scale_x == 1 && sprites[2].sprite.width == 32);
    CHECK(sprites[0].sprite.vertices[0].color == 0xffffffu && sprites[5].sprite.vertices[0].color == 0xffffffffu);
    const auto allocation = fixture.runtime[15];
    CHECK(function_452c20(address(fixture.runtime + 15), 3) == 6);
    CHECK(fixture.runtime[15] == allocation && fixture.runtime[16] - allocation == 3 * 184);
    CHECK(function_452c20(address(fixture.runtime + 15), 6) == 6);
    const auto saved = kinoko::legacy::load<std::array<int32_t,3>>(fixture.runtime + 15);
    CHECK(function_452c20(address(fixture.runtime + 15), UINT32_MAX) == 0);
    CHECK(std::memcmp(&saved, fixture.runtime + 15, sizeof(saved)) == 0);
    BlitSprite copies[2]{};
    copies[0].sprite.vtable = &color_identity; copies[1].sprite.vtable = &target_identity;
    CHECK(function_455230(address(sprites), address(sprites + 2), address(copies)) == address(copies + 2));
    CHECK(copies[0].sprite.vtable == &color_identity && copies[1].sprite.vtable == &target_identity);
    CHECK(copies[1].sprite.texture == 1 && copies[1].sprite.vertices[0].color == sprites[1].sprite.vertices[0].color);
    CHECK(function_455230(address(sprites), address(sprites), address(copies)) == address(copies));
    g678 = address(&device); setup_device(); draws.clear(); sprite_draws = texture_unbinds = 0;
    CHECK(function_4525d0(address(fixture.runtime), 100, 200) == 0);
    CHECK(draw_state_valid && restored() && draws.size() == 2 && draws[0] == address(fixture.layouts[1]));
    CHECK(sprite_draws == 6 && texture_unbinds == 1 && observed_x == 113 && observed_y == 224);
    const DWORD blends[6][3] = {{D3DBLEND_ONE,D3DBLEND_ZERO,D3DBLENDOP_ADD},
        {D3DBLEND_SRCALPHA,D3DBLEND_INVSRCALPHA,D3DBLENDOP_ADD},
        {D3DBLEND_SRCALPHA,D3DBLEND_ONE,D3DBLENDOP_ADD},
        {D3DBLEND_SRCALPHA,D3DBLEND_ONE,D3DBLENDOP_REVSUBTRACT},
        {D3DBLEND_ZERO,D3DBLEND_SRCCOLOR,D3DBLENDOP_ADD},
        {D3DBLEND_DESTCOLOR,D3DBLEND_ONE,D3DBLENDOP_ADD}};
    CHECK(std::memcmp(blends, observed_blends, sizeof(blends)) == 0);
    draw_result = E_FAIL; setup_device();
    CHECK(function_4525d0(address(fixture.runtime), 0, 0) == E_FAIL && restored());
    CHECK(reinterpret_cast<CRITICAL_SECTION*>(fixture.runtime + 5)->RecursionCount == 0);
    fixture.active[24] = 0; draws.clear(); sprite_draws = 0;
    CHECK(function_4522f0(address(fixture.runtime)) == 0);
    CHECK(function_4525d0(address(fixture.runtime), 0, 0) == 0 && draws.empty() && sprite_draws == 0);
    fixture.active[24] = 1; draw_result = 0;
    CHECK(retdec_act_clear_layout_vector(address(fixture.runtime + 15)) == allocation);
    CHECK(fixture.runtime[16] == allocation && fixture.runtime[17] == saved[2]);
    CHECK(sprites[0].sprite.vtable == &color_identity && sprites[5].sprite.vtable == &color_identity);
    CHECK(kinoko_texture_slots[1].width == 128); // clear never owns/releases the texture
    g678 = 0;
    return 0;
}

int main() {
    test::symbols.texture_resource_vtable = &test::texture_identity;
    test::symbols.render_target_vtable = &test::target_identity;
    test::symbols.color_vtable = &test::color_identity;
    test::symbols.sprite_vtable = test::sprite_vtable;
    test::sprite_vtable[7] = address(test::draw_sprite);
    test::layout_vtable[7] = address(test::prepare_layout);
    test::layout_vtable[8] = address(test::draw_layout);
    CHECK(test_update() == 0);
    CHECK(test_storage_and_draw() == 0);
    puts("PASS: original ACT update ordering, unsigned clock, mutable layers, BitBlt and D3D state restoration");
    return 0;
}
