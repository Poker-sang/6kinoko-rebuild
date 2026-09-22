#define CINTERFACE
#include "kinoko/texture_image.h"
#include "kinoko/bitmap.h"
#include "kinoko/graphics_device.h"
#include "kinoko/critical_section.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
struct Texture { IDirect3DTexture9Vtbl* methods; };
static IDirect3DTexture9Vtbl methods{};
static Texture texture{&methods};
static uint8_t surface[64];
static uint8_t depth=32;
static HRESULT create_result=S_OK, lock_result=S_OK;
static bool invalid_surface, missing, valid=true;
static unsigned releases, pixel_releases, unlocks, creates;
static UINT allocated_width, allocated_height;
static D3DFORMAT allocated_format;
static std::string loaded_path;
static HRESULT WINAPI lock_texture(IDirect3DTexture9*,UINT level,D3DLOCKED_RECT* out,const RECT* rect,DWORD flags) {
    valid=valid && level==0 && !rect && flags==0;
    out->pBits=invalid_surface?nullptr:surface;out->Pitch=20;return lock_result;
}
static HRESULT WINAPI unlock_texture(IDirect3DTexture9*,UINT level) {
    valid=valid && level==0; ++unlocks;return E_FAIL; // deliberately ignored by uploader
}
static ULONG WINAPI release_texture(IDirect3DTexture9*) { ++releases;return 0; }
extern "C" {
KinokoGraphics kinoko_graphics{};
KinokoCriticalSection kinoko_graphics_lock{};
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*,int32_t) {}
void retdec_trace_hresult(const char*,long) {}
int32_t kinoko_bitmap_load_cv2(KinokoBitmap* bitmap,const char* path) {
    loaded_path=path;
    if (missing) return 0;
    bitmap->width=3;bitmap->height=2;bitmap->row_width=4;bitmap->bit_depth=depth;
    const auto bytes=depth==16?16:32;
    bitmap->pixels=static_cast<uint8_t*>(std::malloc(bytes));
    for(int i=0;i<bytes;++i) bitmap->pixels[i]=static_cast<uint8_t>(i+1);
    return 1;
}
void kinoko_bitmap_release_pixels(KinokoBitmap* bitmap) {
    if(bitmap->pixels) { ++pixel_releases;std::free(bitmap->pixels);bitmap->pixels=nullptr; }
}
HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9* device,UINT width,UINT height,UINT levels,
    DWORD usage,D3DFORMAT format,D3DPOOL pool,IDirect3DTexture9** output) {
    ++creates;allocated_width=width;allocated_height=height;allocated_format=format;
    valid=valid && device==kinoko_graphics.device && levels==1 && usage==0 &&
        pool==D3DPOOL_MANAGED && kinoko_graphics_lock.native.RecursionCount==1;
    if(SUCCEEDED(create_result)) *output=reinterpret_cast<IDirect3DTexture9*>(&texture);
    return create_result;
}
}
int main() {
    methods.LockRect=lock_texture;methods.UnlockRect=unlock_texture;methods.Release=release_texture;
    InitializeCriticalSection(&kinoko_graphics_lock.native);
    IDirect3DDevice9 device{};kinoko_graphics.device=&device;
    IDirect3DTexture9* result=nullptr;uint32_t width=0,height=0;
    for(auto bits:{16,24,32}) {
        depth=static_cast<uint8_t>(bits);std::memset(surface,0xcc,sizeof(surface));
        const auto old_releases=releases;
        CHECK(kinoko_texture_load_image("portrait.bmp",&result,&width,&height)==S_OK);
        CHECK(loaded_path=="portrait.cv2" && width==3 && height==2 && allocated_width==3 && allocated_height==2);
        CHECK(allocated_format==(bits==16?D3DFMT_A1R5G5B5:D3DFMT_A8R8G8B8));
        CHECK(surface[0]==1 && surface[20]==(bits==16?9:17));
        CHECK(surface[bits==16?4:12]==0xcc && releases==old_releases && result);
        result->lpVtbl->Release(result);result=nullptr;
    }
    kinoko_graphics.capabilities.TextureCaps=D3DPTEXTURECAPS_SQUAREONLY;
    CHECK(kinoko_texture_load_image("square.cv2",&result,&width,&height)==S_OK);
    CHECK(width==3 && height==2 && allocated_width==3 && allocated_height==3);
    result->lpVtbl->Release(result);result=nullptr;
    auto old_releases=releases, old_unlocks=unlocks, old_pixels=pixel_releases;
    lock_result=E_FAIL;
    CHECK(kinoko_texture_load_image("lock.bmp",&result,nullptr,nullptr)==E_FAIL);
    CHECK(!result && releases==old_releases+1 && unlocks==old_unlocks && pixel_releases==old_pixels+1);
    lock_result=S_OK;invalid_surface=true;
    CHECK(kinoko_texture_load_image("bits.bmp",&result,nullptr,nullptr)==E_FAIL);
    CHECK(!result && releases==old_releases+2 && unlocks==old_unlocks+1);
    invalid_surface=false;create_result=E_OUTOFMEMORY;
    CHECK(kinoko_texture_load_image("create.bmp",&result,&width,&height)==E_OUTOFMEMORY);
    CHECK(!result && releases==old_releases+2 && width==3 && height==2);
    missing=true;const auto old_creates=creates;
    CHECK(kinoko_texture_load_image("missing.bmp",&result,nullptr,nullptr)==D3DERR_INVALIDCALL);
    CHECK(kinoko_texture_load_image("x",&result,nullptr,nullptr)==D3DERR_INVALIDCALL && creates==old_creates);
    CHECK(valid && kinoko_graphics_lock.native.RecursionCount==0);
    DeleteCriticalSection(&kinoko_graphics_lock.native);
    std::puts("PASS: typed image upload, raw row strides, square caps, COM transfer and failure cleanup");
}
