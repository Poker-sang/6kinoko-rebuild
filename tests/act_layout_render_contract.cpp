// Compiled only. User controls test/game execution.
#define CINTERFACE
#include "kinoko/act_layout_render.hpp"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/renderer.h"
#include "kinoko/texture_store.h"
#include "kinoko/quad_render.h"
#include "kinoko/string_layout.h"
#include "kinoko/act_layer_access.h"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace kinoko::act;
using kinoko::legacy::address;
static StringGlyphRecord glyph{};
static std::vector<int> calls;
static int queries=0,world_calls=0,submitted=0,appended=0;
static bool query_ok=true;
static DWORD state_values[256]{};
extern "C" {
KinokoGraphics kinoko_graphics{};
KinokoRenderer kinoko_renderer{};
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY]{};
int32_t kinoko_texture_bind_stage(int32_t,int32_t texture) { calls.push_back(500+texture);return E_FAIL; }
int32_t kinoko_quad_submit(KinokoQuad*,float,float) { ++submitted;calls.push_back(900);return E_FAIL; }
void kinoko_trace_i32(const char*,int32_t) {}
KinokoActLayer *__fastcall kinoko_act_layer_world_position(KinokoActLayer *layer,void*,float *x,float *y,float *z) { *x=3;*y=4;*z=5;return layer; }
uint32_t kinoko_string_queue_size(KinokoStringLayout*) { return 1; }
int32_t kinoko_string_queue_at(KinokoStringLayout*,uint32_t) { return address(&glyph); }
int32_t kinoko_string_add_character(int32_t,const char*) { ++appended;return 1; }
int32_t kinoko_string_rebuild_queue(KinokoStringLayout*) { return 0; }
}
static uint8_t __fastcall query(KinokoActResource *resource,void*,const void*,KinokoActResource **out) {
    ++queries;*out=query_ok ? resource : nullptr;return query_ok;
}
static void __fastcall position(KinokoActLayer*,void*,float *x,float *y,float *z) { ++world_calls;*x=3;*y=4;*z=5; }
static HRESULT STDMETHODCALLTYPE get_state(IDirect3DDevice9*,D3DRENDERSTATETYPE type,DWORD *value) {
    calls.push_back(-static_cast<int>(type));*value=state_values[type];return S_OK;
}
static HRESULT STDMETHODCALLTYPE set_state(IDirect3DDevice9*,D3DRENDERSTATETYPE type,DWORD value) {
    calls.push_back(type);state_values[type]=value;return S_OK;
}
static HRESULT STDMETHODCALLTYPE sampler(IDirect3DDevice9*,DWORD,D3DSAMPLERSTATETYPE type,DWORD) { calls.push_back(200+type);return S_OK; }
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"layout line %d\n",__LINE__);return 1; } } while(0)
int main() {
    IDirect3DDevice9Vtbl device_methods{};device_methods.GetRenderState=get_state;
    device_methods.SetRenderState=set_state;device_methods.SetSamplerState=sampler;
    IDirect3DDevice9 device{&device_methods};kinoko_graphics.device=&device;kinoko_renderer.device=&device;
    struct ResourceMethods { void *prefix[2];decltype(&query) convert; } resource_methods{{},query};
    TextureResourcePrefix resource{};resource.vtable=&resource_methods;
    resource.texture=1;resource.width=64;resource.height=64;resource.source_width=16;resource.source_height=20;
    kinoko_texture_slots[1]={nullptr,64,64};
    struct LayerMethods { void *prefix[7];decltype(&position) position_method; } layer_methods{{},position};
    kinoko::map::LayerRecord layer{};layer.methods=reinterpret_cast<const unsigned char*>(&layer_methods);
    layer.resource=reinterpret_cast<KinokoActResource*>(&resource);layer.visible=1;
    Layout2DRecord object{};object.scale={1,1,1};object.alpha=0.5f;object.blend=1;
    object.red=256;object.green=-1;object.blue=257;
    auto *layout=reinterpret_cast<KinokoActLayout*>(&object);
    CHECK(bind_layout_2d(layout,reinterpret_cast<KinokoActLayer*>(&layer))==0);
    CHECK(object.texture==1 && object.rotation_pivot.x==8 && object.scale_pivot.y==10);
    CHECK(kinoko::legacy::load<void*>(reinterpret_cast<unsigned char*>(&layer)+4)==&object.rotation.x);
    CHECK(update_layout_2d(layout)==0);
    CHECK(object.quad.vertices[0].color==0x7f00ff01u);
    CHECK(object.quad.positions[0].x==3 && object.quad.positions[0].y==4);
    object.scale.x=0;CHECK(update_layout_2d(layout)==0);
    CHECK(object.quad.positions[0].x==11 && object.quad.positions[1].x==11);
    resource.source_width=0;resource.source_height=-4;
    object.scale={1,1,1};CHECK(update_layout_2d(layout)==0);
    CHECK(object.quad.source_u_extent==0 && object.quad.base_positions[3].y==-4);
    CHECK(object.rotation_pivot.x==8); // bind initializes pivot only once
    state_values[19]=9;state_values[20]=8;state_values[171]=7;state_values[27]=6;
    const auto updated=world_calls;
    calls.clear();CHECK(draw_layout_2d(layout,7,8)==0);
    CHECK(world_calls==updated && submitted==1); // draw must NOT run update
    CHECK(state_values[19]==9 && state_values[20]==8 && state_values[171]==7 && state_values[27]==6);
    CHECK((calls==std::vector<int>{-19,-20,-171,-27,27,171,19,20,501,900,500,19,20,171,27}));
    // Own-layer visibility only: an invisible parent does not cause a native fallback walk.
    kinoko::map::LayerRecord parent{};layer.parent=reinterpret_cast<KinokoActLayer*>(&parent);
    CHECK(draw_layout_2d(layout,0,0)==0 && submitted==2);
    query_ok=false;calls.clear();CHECK(draw_layout_2d(layout,0,0)==E_FAIL && calls.empty());query_ok=true;
    layer.visible=0;const auto old_queries=queries;
    CHECK(update_layout_2d(layout)==0 && queries==old_queries);
    layer.visible=1;
    kinoko_renderer.state.blend=4;state_values[171]=D3DBLENDOP_REVSUBTRACT;
    set_layout_blend(1);CHECK(state_values[171]==D3DBLENDOP_REVSUBTRACT); // 42AC20 key 32 omits operation write
    StringLayoutRecord text{};text.layer=reinterpret_cast<KinokoActLayer*>(&layer);
    text.alpha=1;text.base_red=256;text.base_green=-1;text.base_blue=257;
    text.scale_x=2;text.scale_y=0.5f;text.alignment=1;text.maximum_width=5;text.blend=1;
    kinoko::legacy::StringView pending(&text.pending),displayed(&text.text);
    pending.assign("A",1);
    glyph.x=10;glyph.y=6;glyph.height=20;
    glyph.quad.base_positions={{{0,0,0},{8,0,0},{0,20,0},{8,20,0}}};
    layer.visible=0;CHECK(kinoko_method_update_string_layout((KinokoStringLayout*)(uintptr_t)(address(&text)), nullptr)==0);
    CHECK(appended==1 && pending.length()==0 && displayed.length()==1);
    layer.visible=1;CHECK(kinoko_method_update_string_layout((KinokoStringLayout*)(uintptr_t)(address(&text)), nullptr)==0);
    CHECK(glyph.quad.positions[0].x==19 && glyph.quad.positions[0].y==12);
    CHECK(glyph.quad.vertices[0].color==0xff00ff01u);
    state_values[19]=9;state_values[20]=8;state_values[171]=7;state_values[27]=6;
    calls.clear();const auto before=world_calls;
    CHECK(kinoko_method_draw_string_layout((KinokoStringLayout*)(uintptr_t)(address(&text)), nullptr, 0, 0)==0);
    CHECK(world_calls==before && kinoko_renderer.state.filter==2);
    CHECK(state_values[19]==9 && state_values[20]==8 && state_values[27]==6);
    pending.destroy();displayed.destroy();
    return 0;
}
