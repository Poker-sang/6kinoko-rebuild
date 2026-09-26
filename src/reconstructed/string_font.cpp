#include "kinoko/critical_section.h"
#include "kinoko/graphics_device.h"
#include "kinoko/string_font.h"
#include "kinoko/string_font_renderer_records.hpp"
#include "kinoko/act_layout_records.hpp"
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
extern char *kinoko_game_window_slot;

HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,
                                D3DFORMAT,D3DPOOL,IDirect3DTexture9**);
}
namespace {
inline char*& game_window_slot = kinoko_game_window_slot;
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
using Pixels=std::list<void*>;
using Renderer=kinoko::text::FontRendererRecord;
Pixels* pixels(void* renderer) {
    const kinoko::native::RecordView<Renderer> record(renderer);
    return static_cast<Pixels*>(record.get(&Renderer::pixel_owner));
}
void set_pixels(void* renderer,Pixels* value) {
    const kinoko::native::RecordView<Renderer> record(renderer);
    record.set(&Renderer::pixel_owner,static_cast<void*>(value));
}
void clear_pixels(void* r) {
    for(void* value:*pixels(r)) std::free(value);
    pixels(r)->clear();
}
struct GraphicsLock {
    GraphicsLock() { EnterCriticalSection(&kinoko_graphics_lock.native); }
    ~GraphicsLock() { LeaveCriticalSection(&kinoko_graphics_lock.native); }
};
// 40F1C0/40F2E0. Each character creates/selects a font, then restores the DC.
struct FontSession {
    void* r;
    explicit FontSession(void* renderer):r(renderer) {
        const kinoko::native::RecordView<Renderer> record(r);
        auto font=CreateFontA(record.get(&Renderer::font_height),0,0,0,
            record.get(&Renderer::font_weight),record.get(&Renderer::style284),
            0,0,128,4,0,2,49,reinterpret_cast<char*>(record.bytes(&Renderer::face)));
        record.set(&Renderer::font_handle,static_cast<void*>(font));
        auto dc=GetDC(reinterpret_cast<HWND>(game_window_slot));
        record.set(&Renderer::device_context,static_cast<void*>(dc));
        record.set(&Renderer::previous_font,static_cast<void*>(SelectObject(dc,font)));
        TEXTMETRICA metrics{};GetTextMetricsA(dc,&metrics);
        record.set(&Renderer::ascent,static_cast<int32_t>(metrics.tmAscent));
        record.set(&Renderer::cursor_x,int32_t(record.get(&Renderer::edge))+record.get(&Renderer::margin_left));
        record.set(&Renderer::cursor_y,int32_t(record.get(&Renderer::edge))+record.get(&Renderer::margin_top));
        std::memset(record.bytes(&Renderer::state364), 0, sizeof(uint16_t));
        record.set(&Renderer::gradient,record.get(&Renderer::bitmap));
        clear_pixels(r);
    }
    ~FontSession() {
        clear_pixels(r);
        const kinoko::native::RecordView<Renderer> record(r);
        auto dc=static_cast<HDC>(record.get(&Renderer::device_context));
        DeleteObject(SelectObject(dc,static_cast<HGDIOBJ>(record.get(&Renderer::previous_font))));
        ReleaseDC(reinterpret_cast<HWND>(game_window_slot),dc);
        record.set(&Renderer::device_context,static_cast<void*>(nullptr));
        record.set(&Renderer::font_handle,static_cast<void*>(nullptr));
        record.set(&Renderer::previous_font,static_cast<void*>(nullptr));
    }
};
void glyph(void* r,UINT character,int32_t& width,int32_t& height) {
    MAT2 transform{};transform.eM11.value=transform.eM22.value=1;
    GLYPHMETRICS metrics{};
    const kinoko::native::RecordView<Renderer> record(r);
    auto dc=static_cast<HDC>(record.get(&Renderer::device_context));
    const DWORD size=GetGlyphOutlineA(dc,character,GGO_GRAY4_BITMAP,&metrics,0,nullptr,&transform);
    if(size==GDI_ERROR) return;
    // 40F3C8 has no zero-height guard. Preserve the recovered operation rather
    // than inventing space metrics; this path still awaits user validation.
    const uint32_t pitch=(size/metrics.gmBlackBoxY)&~3u;
    const int32_t max_x=static_cast<int32_t>(metrics.gmBlackBoxX)+metrics.gmptGlyphOrigin.x+record.get(&Renderer::cursor_x);
    const int32_t max_y=static_cast<int32_t>(metrics.gmBlackBoxY)+record.get(&Renderer::cursor_y)+record.get(&Renderer::ascent)-metrics.gmptGlyphOrigin.y;
    if(max_y>=record.get(&Renderer::bound_height)) return;
    if(max_x>record.get(&Renderer::bound_width)) {
        if(!record.get(&Renderer::style286)) return;
        record.set(&Renderer::cursor_x,record.get(&Renderer::margin_left)+record.get(&Renderer::edge));
        record.set(&Renderer::cursor_y,record.get(&Renderer::cursor_y)+
            record.get(&Renderer::font_height)+record.get(&Renderer::line_space));
    }
    if(size) {
        std::vector<unsigned char> bitmap(size);
        GetGlyphOutlineA(dc,character,GGO_GRAY4_BITMAP,&metrics,size,bitmap.data(),&transform);
        const int32_t stride=record.get(&Renderer::stride);
        auto* output=static_cast<uint32_t*>(record.get(&Renderer::output))+metrics.gmptGlyphOrigin.x+record.get(&Renderer::cursor_x)
            +stride*(record.get(&Renderer::cursor_y)+record.get(&Renderer::ascent)-metrics.gmptGlyphOrigin.y);
        const auto* gradient=static_cast<uint32_t*>(record.get(&Renderer::gradient));
        if(gradient) gradient+=record.get(&Renderer::ascent)-metrics.gmptGlyphOrigin.y;
        for(uint32_t y=0;y<metrics.gmBlackBoxY;++y) {
            const uint32_t rgb=gradient?gradient[y]:record.get(&Renderer::color);
            for(uint32_t x=0;x<metrics.gmBlackBoxX;++x)
                output[y*stride+x]=rgb|((0x0ff00000u*bitmap[y*pitch+x])&0xff000000u);
        }
    }
    // FontSession resets accent/ruby before each single-character call, so
    // neither branch of the generic tagged renderer is reachable here.
    record.set(&Renderer::cursor_x,record.get(&Renderer::cursor_x)+
        metrics.gmCellIncX+record.get(&Renderer::character_space));
    width=(std::max)(width,max_x);height=(std::max)(height,max_y);
}
void outline(void* r,const uint32_t* source,uint32_t* destination) {
    const kinoko::native::RecordView<Renderer> record(r);
    const int32_t stride=record.get(&Renderer::stride);
    for(int32_t y=1;y<record.get(&Renderer::bound_height)-1;++y)
        for(int32_t x=1;x<record.get(&Renderer::bound_width)-1;++x) {
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
extern "C" void kinoko_string_font_construct(void* r) {
    // 40EC70 initializes only these members; do not clear unrelated padding.
    const kinoko::native::RecordView<Renderer> record(r);
    record.set(&Renderer::font_weight,400);
    record.set(&Renderer::style284,uint8_t{0});
    record.set(&Renderer::edge,uint8_t{0});
    record.set(&Renderer::style286,uint8_t{0});
    record.set(&Renderer::margin_left,0);
    record.set(&Renderer::margin_top,0);
    record.set(&Renderer::character_space,0);
    record.set(&Renderer::line_space,0);
    std::memset(record.bytes(&Renderer::state352), 0, sizeof(uint32_t));
    for (auto member : {&Renderer::device_context, &Renderer::font_handle,
                        &Renderer::previous_font, &Renderer::destination, &Renderer::gradient})
        record.set(member, static_cast<void*>(nullptr));
    record.set(&Renderer::bitmap,static_cast<void*>(nullptr));
    record.set(&Renderer::setting288,100000);
    const kinoko::native::RecordView<kinoko::legacy::StringRecord> label(record.bytes(&Renderer::label));
    label.set(&kinoko::legacy::StringRecord::length,uint32_t{0});
    label.set(&kinoko::legacy::StringRecord::capacity,uint32_t{15});
    label.bytes(&kinoko::legacy::StringRecord::characters)[0]=0;
    set_pixels(r,new Pixels);
}
extern "C" void kinoko_string_font_configure(void* r,KinokoStringLayout* layout) {
    // 440910/440CA0 preserve the other config bytes and set equal RGB endpoints.
    using Layout=kinoko::act::StringLayoutRecord;
    const kinoko::native::RecordView<Layout> text(layout);
    const kinoko::native::RecordView<Renderer> renderer(r);
    strcpy_s(reinterpret_cast<char*>(renderer.bytes(&Renderer::face)),256,
        StringView(text.bytes(&Layout::face)).data());
    const uint8_t colors[]={static_cast<uint8_t>(text.get(&Layout::red)),
        static_cast<uint8_t>(text.get(&Layout::green)),
        static_cast<uint8_t>(text.get(&Layout::blue))};
    auto* endpoints=renderer.bytes(&Renderer::colors);
    for(int channel=0;channel<3;++channel)
        endpoints[channel*2]=endpoints[channel*2+1]=colors[channel];
    renderer.set(&Renderer::font_height,text.get(&Layout::font_height));
    renderer.set(&Renderer::font_weight,text.get(&Layout::font_weight));
    renderer.set(&Renderer::edge,text.get(&Layout::edge));
    renderer.set(&Renderer::character_space,text.get(&Layout::character_space));
    renderer.set(&Renderer::line_space,text.get(&Layout::line_space));
    renderer.set(&Renderer::color,(uint32_t(endpoints[0])<<16)|
        (uint32_t(endpoints[2])<<8)|endpoints[4]);
    // 40EE30's equal-color branch leaves an existing gradient allocation alone.
    clear_pixels(r);
}
extern "C" void kinoko_string_font_rasterize(void* r,const char* character,int32_t* width,int32_t* height) {
    const kinoko::native::RecordView<Renderer> record(r);
    const bool edge=record.get(&Renderer::edge)!=0;
    std::vector<uint32_t> temporary;
    if(edge) {
        temporary.resize(size_t(record.get(&Renderer::stride))*record.get(&Renderer::bound_height));
        record.set(&Renderer::output,static_cast<void*>(temporary.data()));
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
    if(edge) outline(r,temporary.data(),
        static_cast<uint32_t*>(record.get(&Renderer::destination)));
    if(width) *width=w+edge;
    if(height) *height=h+edge;
}
extern "C" int32_t kinoko_string_font_texture(void* r) {
    kinoko::ComOwner<IDirect3DTexture9> texture;
    IDirect3DTexture9* value=nullptr;
    {
        GraphicsLock lock;
        if(FAILED(D3DXCreateTexture(kinoko_graphics.device,512,512,1,0,
            D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&value))) return 0;
    }
    texture.reset(value);
    {
        GraphicsLock lock;D3DLOCKED_RECT rect{};
        if(FAILED(value->LockRect(0,&rect,nullptr,0))) return 0;
        std::memset(rect.pBits,0,4*512*512);
        const kinoko::native::RecordView<Renderer> record(r);
        record.set(&Renderer::output,rect.pBits);
        record.set(&Renderer::destination,rect.pBits);
        record.set(&Renderer::bound_height,512);
        record.set(&Renderer::bound_width,512);
        record.set(&Renderer::stride,static_cast<int32_t>(rect.Pitch/4));
        kinoko_string_font_rasterize((void*)(uintptr_t)(r), "", nullptr, nullptr);
        value->UnlockRect(0);
    }
    const int32_t handle=kinoko_texture_register(value,512,512);
    if(handle) texture.detach();
    return handle;
}
extern "C" void kinoko_string_font_upload(void* r,int32_t handle,const char* character,
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
        const kinoko::native::RecordView<Renderer> record(r);
        record.set(&Renderer::output,static_cast<void*>(pixels.data()));
        record.set(&Renderer::destination,static_cast<void*>(pixels.data()));
        record.set(&Renderer::bound_height,static_cast<int32_t>(description.Height-y));
        record.set(&Renderer::bound_width,static_cast<int32_t>(description.Width-x));
        record.set(&Renderer::stride,static_cast<int32_t>(rect.Pitch/4));
        kinoko_string_font_rasterize((void*)(uintptr_t)(r), character, width, height);
        const uint32_t bytes_per_pixel=rect.Pitch/description.Width;
        for(int32_t row=0;row<*height;++row)
            memcpy_s(static_cast<unsigned char*>(rect.pBits)+row*rect.Pitch,
                (description.Width-x)*bytes_per_pixel,pixels.data()+row*rect.Pitch,
                *width*bytes_per_pixel);
    } catch(...) { texture->UnlockRect(0);throw; }
    texture->UnlockRect(0);
}

extern "C" void kinoko_string_font_copy_pixels(void* out,void* in) {
    // Original list assignment copies borrowed pixel pointers. It destroys old
    // list nodes without releasing their pointed-to allocations.
    *pixels(out)=*pixels(in);
}
extern "C" void kinoko_string_font_destroy_pixels(void* renderer) {
    clear_pixels(renderer);delete pixels(renderer);set_pixels(renderer,nullptr);
}

