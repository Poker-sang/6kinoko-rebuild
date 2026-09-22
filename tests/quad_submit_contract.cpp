#define CINTERFACE
#include <d3d9.h>
#include "kinoko/quad_render.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cstdio>
#include <vector>
using namespace kinoko::render;
static std::vector<int> calls;
static QuadRecord *expected;
static bool valid;
extern "C" {
int32_t g678=0,g702=0,g703=0,g707=0,g709=0;
int32_t retdec_set_texture_stage(int32_t stage,int32_t texture) { calls.push_back(1);valid=valid && stage==0 && texture==17;return E_FAIL; }
void retdec_trace_i32(const char *,int32_t) {}
void retdec_trace_hresult(const char *,long) {}
}
static HRESULT STDMETHODCALLTYPE format(IDirect3DDevice9 *,DWORD fvf) {
    calls.push_back(2);valid=valid && fvf==324;return E_FAIL;
}
static HRESULT STDMETHODCALLTYPE draw(IDirect3DDevice9 *,D3DPRIMITIVETYPE type,UINT count,const void *vertices,UINT stride) {
    calls.push_back(3);valid=valid && type==D3DPT_TRIANGLESTRIP && count==2 && stride==28 && vertices==expected->vertices.data();return 37;
}
static HRESULT STDMETHODCALLTYPE state(IDirect3DDevice9 *,D3DRENDERSTATETYPE type,DWORD value) { calls.push_back(static_cast<int>(type));calls.push_back(static_cast<int>(value));return 0; }
static HRESULT STDMETHODCALLTYPE texture_state(IDirect3DDevice9 *,DWORD stage,D3DTEXTURESTAGESTATETYPE type,DWORD value) {
    valid=valid && stage==0 && type==D3DTSS_ALPHAOP && value==D3DTOP_MODULATE;calls.push_back(4);return 0;
}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"quad line %d\n",__LINE__);return 1; } } while(0)
int main() {
    IDirect3DDevice9Vtbl methods{};methods.SetFVF=format;methods.DrawPrimitiveUP=draw;
    methods.SetRenderState=state;methods.SetTextureStageState=texture_state;
    IDirect3DDevice9 device{&methods};g678=g702=kinoko::legacy::address(&device);
    QuadRecord quad{};quad.texture=17;quad.positions[0]={1,2,3};
    quad.vertices[0].u=0.25f;quad.vertices[0].v=0.75f;quad.vertices[0].color=0x12785634;
    expected=&quad;valid=true;
    CHECK(kinoko_quad_submit(reinterpret_cast<KinokoQuad *>(&quad),4,5)==37);
    CHECK(valid && (calls==std::vector<int>{1,2,3}));
    CHECK(quad.vertices[0].x==4.5f && quad.vertices[0].y==6.5f && quad.vertices[0].z==3.5f);
    CHECK(quad.vertices[0].rhw==1 && quad.vertices[0].u==0.25f && quad.vertices[0].color==0x12785634);
    CHECK(quad.positions[0].x==1);
    calls.clear();kinoko_render_set_blend(1);
    CHECK((calls==std::vector<int>{171,1,19,5,20,6}));
    calls.clear();kinoko_render_set_blend(1);CHECK(calls.empty());
    kinoko_render_set_blend(3);CHECK((calls==std::vector<int>{171,3,20,2}));
    calls.clear();kinoko_render_set_blend(1);CHECK((calls==std::vector<int>{171,1,20,6}));
    calls.clear();kinoko_render_set_alpha(1,0);CHECK((calls==std::vector<int>{27,1,4}));
    calls.clear();kinoko_render_set_alpha(1,0);CHECK((calls==std::vector<int>{4}));
    calls.clear();kinoko_render_set_depth(0,0);CHECK((calls==std::vector<int>{7,0,14,0}));
    return 0;
}
