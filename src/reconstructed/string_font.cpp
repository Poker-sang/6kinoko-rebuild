#include "kinoko/string_font.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/texture_store.h"
#include "kinoko/com_owner.hpp"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include <list>

extern "C" {
extern int32_t g678;
extern char *g767;
extern CRITICAL_SECTION g676;
HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,
                                D3DFORMAT,D3DPOOL,IDirect3DTexture9**);
}
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
using Pixels=std::list<void*>;
Pixels*& pixels(int32_t renderer) { return field<Pixels*>(renderer+348); }
void clear_pixels(int32_t r) {
    for(void* value:*pixels(r)) std::free(value);
    pixels(r)->clear();
}
struct GraphicsLock {
    GraphicsLock() { EnterCriticalSection(&g676); }
    ~GraphicsLock() { LeaveCriticalSection(&g676); }
};
// 40F1C0/40F2E0. Each character creates/selects a font, then restores the DC.
struct FontSession {
    int32_t r;
    explicit FontSession(int32_t renderer):r(renderer) {
        auto font=CreateFontA(field<int>(r+276),0,0,0,field<int>(r+280),
            field<uint8_t>(r+284),0,0,128,4,0,2,49,pointer<char>(r+12));
        field<HFONT>(r+4)=font;
        auto dc=GetDC(reinterpret_cast<HWND>(g767));
        field<HDC>(r)=dc;field<HGDIOBJ>(r+8)=SelectObject(dc,font);
        TEXTMETRICA metrics{};GetTextMetricsA(dc,&metrics);
        field<int32_t>(r+316)=metrics.tmAscent;
        field<int32_t>(r+308)=field<uint8_t>(r+285)+field<int32_t>(r+292);
        field<int32_t>(r+312)=field<uint8_t>(r+285)+field<int32_t>(r+296);
        field<uint16_t>(r+364)=0;
        field<void*>(r+340)=field<void*>(r+344);
        clear_pixels(r);
    }
    ~FontSession() {
        clear_pixels(r);
        auto dc=field<HDC>(r);
        DeleteObject(SelectObject(dc,field<HGDIOBJ>(r+8)));
        ReleaseDC(reinterpret_cast<HWND>(g767),dc);
        field<int32_t>(r)=field<int32_t>(r+4)=field<int32_t>(r+8)=0;
    }
};
void glyph(int32_t r,UINT character,int32_t& width,int32_t& height) {
    MAT2 transform{};transform.eM11.value=transform.eM22.value=1;
    GLYPHMETRICS metrics{};
    auto dc=field<HDC>(r);
    const DWORD size=GetGlyphOutlineA(dc,character,GGO_GRAY4_BITMAP,&metrics,0,nullptr,&transform);
    if(size==GDI_ERROR) return;
    // 40F3C8 has no zero-height guard. Preserve the recovered operation rather
    // than inventing space metrics; this path still awaits user validation.
    const uint32_t pitch=(size/metrics.gmBlackBoxY)&~3u;
    const int32_t max_x=static_cast<int32_t>(metrics.gmBlackBoxX)+metrics.gmptGlyphOrigin.x+field<int32_t>(r+308);
    const int32_t max_y=static_cast<int32_t>(metrics.gmBlackBoxY)+field<int32_t>(r+312)+field<int32_t>(r+316)-metrics.gmptGlyphOrigin.y;
    if(max_y>=field<int32_t>(r+328)) return;
    if(max_x>field<int32_t>(r+332)) {
        if(!field<uint8_t>(r+286)) return;
        field<int32_t>(r+308)=field<int32_t>(r+292)+field<uint8_t>(r+285);
        field<int32_t>(r+312)+=field<int32_t>(r+276)+field<int32_t>(r+304);
    }
    if(size) {
        std::vector<unsigned char> bitmap(size);
        GetGlyphOutlineA(dc,character,GGO_GRAY4_BITMAP,&metrics,size,bitmap.data(),&transform);
        const int32_t stride=field<int32_t>(r+336);
        auto* output=field<uint32_t*>(r+320)+metrics.gmptGlyphOrigin.x+field<int32_t>(r+308)
            +stride*(field<int32_t>(r+312)+field<int32_t>(r+316)-metrics.gmptGlyphOrigin.y);
        const auto* gradient=field<uint32_t*>(r+340);
        if(gradient) gradient+=field<int32_t>(r+316)-metrics.gmptGlyphOrigin.y;
        for(uint32_t y=0;y<metrics.gmBlackBoxY;++y) {
            const uint32_t rgb=gradient?gradient[y]:field<uint32_t>(r+360);
            for(uint32_t x=0;x<metrics.gmBlackBoxX;++x)
                output[y*stride+x]=rgb|((0x0ff00000u*bitmap[y*pitch+x])&0xff000000u);
        }
    }
    // FontSession resets accent/ruby before each single-character call, so
    // neither branch of the generic tagged renderer is reachable here.
    field<int32_t>(r+308)+=metrics.gmCellIncX+field<int32_t>(r+300);
    width=(std::max)(width,max_x);height=(std::max)(height,max_y);
}
void outline(int32_t r,const uint32_t* source,uint32_t* destination) {
    const int32_t stride=field<int32_t>(r+336);
    for(int32_t y=1;y<field<int32_t>(r+328)-1;++y)
        for(int32_t x=1;x<field<int32_t>(r+332)-1;++x) {
            const int32_t i=y*stride+x;
            const uint32_t pixel=source[i],alpha=pixel>>24;
            if(alpha) {
                const uint32_t factor=(std::min)(alpha+33,256u);
                destination[i]=0xff000000u|
                    (((factor*((pixel>>16)&255))>>8)<<16)|
                    (((factor*((pixel>>8)&255))>>8)<<8)|((factor*(pixel&255))>>8);
            } else {
                const uint32_t a=(std::max)({source[i-1]>>24,source[i+1]>>24,
                                            source[i-stride]>>24,source[i+stride]>>24});
                destination[i]=(destination[i]&0x00ffffffu)|(a<<24);
            }
        }
}
}
extern "C" void kinoko_string_font_construct(int32_t r) {
    // 40EC70 initializes only these members; do not clear unrelated padding.
    field<int32_t>(r+280)=400;field<uint16_t>(r+284)=0;field<uint8_t>(r+286)=0;
    for(int offset:{292,296,300,304,352,384,0,4,8,324,340,344}) field<int32_t>(r+offset)=0;
    field<int32_t>(r+288)=100000;field<int32_t>(r+388)=15;field<uint8_t>(r+368)=0;
    pixels(r)=new Pixels;
}
extern "C" void kinoko_string_font_configure(int32_t r,int32_t layout) {
    // 440910/440CA0 preserve the other config bytes and set equal RGB endpoints.
    strcpy_s(pointer<char>(r+12),256,StringView(pointer<void>(layout+60)).data());
    for(int channel=0;channel<3;++channel) {
        const auto value=field<uint8_t>(layout+96+channel*4);
        field<uint8_t>(r+268+channel*2)=field<uint8_t>(r+269+channel*2)=value;
    }
    field<int32_t>(r+276)=field<int32_t>(layout+88);
    field<int32_t>(r+280)=field<int32_t>(layout+92);
    field<uint8_t>(r+285)=field<uint8_t>(layout+128);
    field<int32_t>(r+300)=field<int32_t>(layout+120);
    field<int32_t>(r+304)=field<int32_t>(layout+124);
    field<uint32_t>(r+360)=(uint32_t(field<uint8_t>(r+268))<<16)|
        (uint32_t(field<uint8_t>(r+270))<<8)|field<uint8_t>(r+272);
    // 40EE30's equal-color branch leaves an existing gradient allocation alone.
    clear_pixels(r);
}
extern "C" void kinoko_string_font_rasterize(int32_t r,const char* character,int32_t* width,int32_t* height) {
    const bool edge=field<uint8_t>(r+285)!=0;
    std::vector<uint32_t> temporary;
    if(edge) {
        temporary.resize(size_t(field<int32_t>(r+336))*field<int32_t>(r+328));
        field<void*>(r+320)=temporary.data();
    }
    int32_t w=0,h=0;
    {
        FontSession session(r);
        const auto lead=static_cast<unsigned char>(character[0]);
        const bool double_byte=(lead>=0x81 && lead<0xa0)||(lead>=0xe0 && lead<0xff);
        // The '<' tag prefix consumes the rest when no '>' exists (40FDC4).
        // CStringLayout supplies exactly one character, so it never has a tag.
        if(lead && lead!='<' && (!double_byte || character[1])) {
            const UINT code=double_byte?(UINT(lead)<<8)|static_cast<unsigned char>(character[1])
                :static_cast<UINT>(static_cast<signed char>(lead));
            glyph(r,code,w,h);
        }
    }
    if(edge) outline(r,temporary.data(),field<uint32_t*>(r+324));
    if(width) *width=w+edge;
    if(height) *height=h+edge;
}
extern "C" int32_t kinoko_string_font_texture(int32_t r) {
    kinoko::ComOwner<IDirect3DTexture9> texture;
    IDirect3DTexture9* value=nullptr;
    {
        GraphicsLock lock;
        if(FAILED(D3DXCreateTexture(pointer<IDirect3DDevice9>(g678),512,512,1,0,
            D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&value))) return 0;
    }
    texture.reset(value);
    {
        GraphicsLock lock;D3DLOCKED_RECT rect{};
        if(FAILED(value->LockRect(0,&rect,nullptr,0))) return 0;
        std::memset(rect.pBits,0,4*512*512);
        field<void*>(r+320)=field<void*>(r+324)=rect.pBits;
        field<int32_t>(r+328)=field<int32_t>(r+332)=512;
        field<int32_t>(r+336)=rect.Pitch/4;
        kinoko_string_font_rasterize(r,"",nullptr,nullptr);
        value->UnlockRect(0);
    }
    const int32_t handle=kinoko_texture_register(value,512,512);
    if(handle) texture.detach();
    return handle;
}
extern "C" void kinoko_string_font_upload(int32_t r,int32_t handle,const char* character,
                                          int32_t x,int32_t y,int32_t* width,int32_t* height) {
    if(handle<=0 || handle>=KINOKO_TEXTURE_CAPACITY) return;
    auto* texture=static_cast<IDirect3DTexture9*>(kinoko_texture_slots[handle].texture);
    if(!texture) return;
    GraphicsLock lock;
    kinoko::ComOwner<IDirect3DSurface9> surface;
    IDirect3DSurface9* value=nullptr;
    if(FAILED(texture->GetSurfaceLevel(0,&value))) return;
    surface.reset(value);D3DSURFACE_DESC description{};value->GetDesc(&description);surface.reset();
    RECT region{x,y,static_cast<LONG>(description.Width),static_cast<LONG>(description.Height)};
    D3DLOCKED_RECT rect{};
    if(FAILED(texture->LockRect(0,&rect,&region,0))) return;
    try {
        std::vector<unsigned char> pixels(size_t(description.Height-y)*rect.Pitch);
        field<void*>(r+320)=field<void*>(r+324)=pixels.data();
        field<int32_t>(r+328)=description.Height-y;field<int32_t>(r+332)=description.Width-x;
        field<int32_t>(r+336)=rect.Pitch/4;
        kinoko_string_font_rasterize(r,character,width,height);
        const uint32_t bytes_per_pixel=rect.Pitch/description.Width;
        for(int32_t row=0;row<*height;++row)
            memcpy_s(static_cast<unsigned char*>(rect.pBits)+row*rect.Pitch,
                (description.Width-x)*bytes_per_pixel,pixels.data()+row*rect.Pitch,
                *width*bytes_per_pixel);
    } catch(...) { texture->UnlockRect(0);throw; }
    texture->UnlockRect(0);
}

extern "C" void kinoko_string_font_copy_pixels(int32_t out,int32_t in) {
    // Original list assignment copies borrowed pixel pointers. It destroys old
    // list nodes without releasing their pointed-to allocations.
    *pixels(out)=*pixels(in);
}
extern "C" void kinoko_string_font_destroy_pixels(int32_t renderer) {
    clear_pixels(renderer);delete pixels(renderer);pixels(renderer)=nullptr;
}
