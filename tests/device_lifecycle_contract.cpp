// Compiled with the build; execution is reserved for the user.
#define CINTERFACE
#include "kinoko/renderer.h"
#include "kinoko/render_target.h"
#include "kinoko/texture_store.h"
#include <vector>
#include <cstdio>
extern "C" {
KinokoGraphics kinoko_graphics{};
KinokoRenderer kinoko_renderer{};
CRITICAL_SECTION g676;
int32_t g746,g747,g748,g749,g750,g751,g752,g753;
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY]{};
int32_t kinoko_texture_register(void*,uint32_t,uint32_t) { return 0; }
void retdec_trace(const char*) {}
void retdec_trace_hresult(const char*,long) {}
HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,D3DFORMAT,D3DPOOL,IDirect3DTexture9**) { return E_FAIL; }
}
static std::vector<int> calls;
static HRESULT reset_result=S_OK;
static IDirect3DSwapChain9 chain{};
static IDirect3DSurface9 surface{};
static HRESULT STDMETHODCALLTYPE reset(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*) { calls.push_back(3);return reset_result; }
static ULONG STDMETHODCALLTYPE release_chain(IDirect3DSwapChain9*) { calls.push_back(2);return 0; }
static HRESULT STDMETHODCALLTYPE get_chain(IDirect3DDevice9*,UINT,IDirect3DSwapChain9** out) { calls.push_back(4);*out=&chain;return S_OK; }
static int32_t __fastcall before(KinokoRenderer*,void*) { calls.push_back(1);return 0; }
static int32_t __fastcall after(KinokoRenderer*,void*) { calls.push_back(5);return 0; }
static ULONG STDMETHODCALLTYPE release_surface(IDirect3DSurface9*) { calls.push_back(8);return 17; }
static HRESULT STDMETHODCALLTYPE get_surface(IDirect3DTexture9*,UINT,IDirect3DSurface9** out) { calls.push_back(6);*out=&surface;return S_OK; }
static HRESULT STDMETHODCALLTYPE bind(IDirect3DDevice9*,DWORD,IDirect3DSurface9*) { calls.push_back(7);return E_FAIL; }
static HRESULT STDMETHODCALLTYPE render_surface(IDirect3DDevice9*,DWORD,IDirect3DSurface9** out) { calls.push_back(9);*out=&surface;return S_OK; }
static HRESULT STDMETHODCALLTYPE depth_surface(IDirect3DDevice9*,IDirect3DSurface9** out) { calls.push_back(10);*out=&surface;return S_OK; }
static HRESULT STDMETHODCALLTYPE texture(IDirect3DDevice9*,DWORD stage,IDirect3DBaseTexture9* value) { calls.push_back(value ? -1 : 100+stage);return S_OK; }
static HRESULT STDMETHODCALLTYPE state(IDirect3DDevice9*,D3DRENDERSTATETYPE type,DWORD value) { calls.push_back(1000+type);calls.push_back(value);return S_OK; }
static HRESULT STDMETHODCALLTYPE sampler(IDirect3DDevice9*,DWORD,D3DSAMPLERSTATETYPE type,DWORD value) { calls.push_back(2000+type);calls.push_back(value);return S_OK; }
static HRESULT STDMETHODCALLTYPE stage(IDirect3DDevice9*,DWORD,D3DTEXTURESTAGESTATETYPE type,DWORD value) { calls.push_back(3000+type);calls.push_back(value);return S_OK; }
static HRESULT STDMETHODCALLTYPE clear(IDirect3DDevice9*,DWORD,const D3DRECT*,DWORD flags,D3DCOLOR,float,DWORD) { calls.push_back(4000+flags);return 23; }
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"device line %d\n",__LINE__);return 1; } } while(0)
int main() {
    InitializeCriticalSection(&g676);
    IDirect3DDevice9Vtbl methods{};
    methods.Reset=reset;methods.GetSwapChain=get_chain;methods.SetRenderTarget=bind;
    methods.GetRenderTarget=render_surface;methods.GetDepthStencilSurface=depth_surface;
    methods.SetTexture=texture;methods.SetRenderState=state;methods.SetSamplerState=sampler;
    methods.SetTextureStageState=stage;methods.Clear=clear;
    IDirect3DDevice9 device{&methods};
    IDirect3D9 factory{};
    IDirect3DSwapChain9Vtbl chain_methods{};chain_methods.Release=release_chain;chain.lpVtbl=&chain_methods;
    IDirect3DSurface9Vtbl surface_methods{};surface_methods.Release=release_surface;surface.lpVtbl=&surface_methods;
    IDirect3DTexture9Vtbl texture_methods{};texture_methods.GetSurfaceLevel=get_surface;
    IDirect3DTexture9 target{&texture_methods};
    struct Callbacks { decltype(&before) first,second; } callbacks{before,after};
    kinoko_renderer.methods=&callbacks;
    auto *listener=reinterpret_cast<KinokoDeviceListener*>(&kinoko_renderer);
    kinoko_graphics.factory=&factory;kinoko_graphics.device=&device;kinoko_graphics.swap_chain=&chain;
    kinoko_graphics.present.Windowed=TRUE;kinoko_graphics.display.Format=D3DFMT_R5G6B5;
    CHECK(kinoko_add_device_listener(listener)==1);
    CHECK(kinoko_add_device_listener(listener)==0);
    CHECK(kinoko_graphics_reset()==1);
    CHECK((calls==std::vector<int>{1,2,3,4,5}));
    CHECK(kinoko_graphics.present.BackBufferFormat==D3DFMT_R5G6B5);
    calls.clear();reset_result=E_FAIL;
    CHECK(kinoko_graphics_reset()==0);
    CHECK((calls==std::vector<int>{1,2,3}) && kinoko_graphics.swap_chain==nullptr);
    calls.clear();kinoko_graphics.cooperative_status=D3DERR_DEVICELOST;
    CHECK(kinoko_graphics_reset()==0 && calls.empty());
    kinoko_remove_device_listener(listener);
    kinoko_renderer.device=&device;kinoko_renderer.backbuffer=&surface;
    kinoko_texture_slots[1].texture=&target;
    CHECK(kinoko_set_render_target(1)==17);
    CHECK((calls==std::vector<int>{6,7,8}));
    calls.clear();CHECK(kinoko_set_render_target(0)==E_FAIL);
    CHECK((calls==std::vector<int>{7}));
    kinoko_renderer.state={1,2,3,99,0x100,4,0x101,8,127,88};
    kinoko_renderer.present_pending=1;
    calls.clear();CHECK(kinoko_renderer_after_reset(&kinoko_renderer,nullptr)==23);
    CHECK((calls==std::vector<int>{100,101,102,103,104,105,106,107,9,10,
        1027,1,1015,1,3004,4,1025,8,1024,127,1007,0,1014,1,1023,4,
        2005,2,2006,2,2007,2,1171,1,1019,5,1020,6,1022,3,1058,255,4004}));
    CHECK(kinoko_renderer.state.unknown24==0 && kinoko_renderer.state.unknown48==0);
    CHECK(kinoko_renderer.state.filter==2 && kinoko_renderer.state.depth_flags==0x100);
    CHECK(kinoko_renderer.present_pending==0);
    calls.clear();CHECK(kinoko_renderer_before_reset(&kinoko_renderer,nullptr)==17);
    CHECK((calls==std::vector<int>{8,8}) && kinoko_renderer.backbuffer==&surface);
    DeleteCriticalSection(&g676);
    return 0;
}
