#define CINTERFACE
#include "kinoko/texture_store.h"
#include "kinoko/graphics_device.h"
#include <vector>
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
struct Bind { DWORD stage; IDirect3DBaseTexture9* texture; };
static std::vector<Bind> calls;
static HRESULT status=S_OK;
static unsigned diagnostics;
extern "C" {
KinokoGraphics kinoko_graphics{};
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY]{};
void retdec_trace_i32(const char*,int32_t) { ++diagnostics; }
}
static HRESULT WINAPI bind(IDirect3DDevice9*,DWORD stage,IDirect3DBaseTexture9* texture) {
    calls.push_back({stage,texture}); return status;
}
int main() {
    IDirect3DDevice9Vtbl methods{}; methods.SetTexture=bind;
    IDirect3DDevice9 device{&methods};kinoko_graphics.device=&device;
    IDirect3DBaseTexture9 first{}, second{};
    kinoko_texture_slots[1]={&first,32,16};kinoko_texture_slots[2]={&second,64,64};
    kinoko_initialize_texture_cache();
    CHECK(kinoko_texture_bind_stage(0,1)==S_OK && calls.size()==1);
    CHECK(kinoko_texture_bind_stage(0,1)==1 && calls.size()==1); // cached returns handle
    CHECK(kinoko_texture_bind_stage(7,1)==S_OK && calls.size()==2 && calls.back().stage==7);
    CHECK(kinoko_texture_bind_stage(0,2)==S_OK && calls.back().texture==&second);
    auto before=calls.size();
    status=E_FAIL;
    CHECK(kinoko_texture_bind_stage(7,2)==E_FAIL && calls.size()==before+1);
    CHECK(kinoko_texture_bind_stage(7,2)==2 && calls.size()==before+1); // original caches failed bind
    CHECK(kinoko_texture_bind_stage(7,0)==0 && !calls.back().texture);
    CHECK(kinoko_texture_bind_stage(7,0)==0 && calls.size()==before+3); // zero always calls device
    status=S_OK;
    CHECK(kinoko_texture_bind_stage(7,1)==S_OK);
    before=calls.size();kinoko_texture_forget_bindings(1);
    CHECK(kinoko_texture_bind_stage(0,2)==2 && calls.size()==before); // unrelated key preserved
    CHECK(kinoko_texture_bind_stage(7,1)==S_OK && calls.size()==before+1);
    kinoko_initialize_texture_cache();before=calls.size();
    CHECK(kinoko_texture_bind_stage(0,2)==S_OK && calls.size()==before+1);
    before=calls.size();
    CHECK(kinoko_texture_bind_stage(-1,1)==E_FAIL && kinoko_texture_bind_stage(8,1)==E_FAIL);
    CHECK(kinoko_texture_bind_stage(0,-1)==E_FAIL && kinoko_texture_bind_stage(0,4096)==E_FAIL);
    CHECK(kinoko_texture_bind_stage(0,3)==E_FAIL && diagnostics==3 && calls.size()==before);
    kinoko_graphics.device=nullptr;
    CHECK(kinoko_texture_bind_stage(0,0)==E_FAIL && calls.size()==before);
    std::puts("PASS: eight-stage cache, repeated zero, failed-bind semantics, invalidation and bounds");
}
