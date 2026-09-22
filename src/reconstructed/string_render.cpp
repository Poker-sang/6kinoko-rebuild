#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/string_layout.h"
#include "kinoko/string_font.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/texture_store.h"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cstring>
extern "C" {
extern int32_t g702,g703,g704;
int32_t function_4410c0(int32_t layout);
}
namespace {
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::StringView;
int32_t glyph_at(int32_t layout,uint32_t index) {
    return kinoko_string_queue_at(layout,index);
}
int32_t new_page(int32_t layout) {
    const int32_t page=kinoko_string_append_atlas(layout);
    for(int offset:{0,4,8,432}) field<int32_t>(page+offset)=0;
    field<int32_t>(page+12)=field<int32_t>(page+16)=512;
    kinoko_string_font_configure(page+24,layout);
    field<int32_t>(page+428)=kinoko_string_font_texture(page+24);
    return page;
}
// 404EE0's CSpriteEx geometry and texture coordinates. The generic RetDec
// wrapper lost the queried texture dimensions; use the actual texture store.
void rectangle(int32_t s,int32_t handle,int32_t x,int32_t y,int32_t w,int32_t h) {
    field<int32_t>(s+4)=handle;
    if(handle) {
        const float tw=static_cast<float>(kinoko_texture_slots[handle].width);
        const float th=static_cast<float>(kinoko_texture_slots[handle].height);
        field<float>(s+120)=tw;field<float>(s+124)=th;
        const float du=static_cast<float>(w/tw),dv=static_cast<float>(h/th);
        field<float>(s+224)=du;field<float>(s+228)=dv;
        const float u=static_cast<float>(x/tw),v=static_cast<float>(y/th);
        field<float>(s+28)=field<float>(s+84)=u;
        field<float>(s+32)=field<float>(s+60)=v;
        field<float>(s+56)=field<float>(s+112)=du+u;
        field<float>(s+88)=field<float>(s+116)=dv+v;
    } else for(int offset:{120,124,224,228}) field<float>(s+offset)=0;
    for(int offset:{24,52,80,108}) field<uint32_t>(s+offset)=0xffffffffu;
    for(int offset:{128,132,144,152}) field<float>(s+offset)=-0.0f;
    field<float>(s+140)=field<float>(s+164)=static_cast<float>(w);
    field<float>(s+156)=field<float>(s+168)=static_cast<float>(h);
    for(int offset:{136,148,160,172}) field<float>(s+offset)=0;
}
void blend(int32_t mode) {
    auto* device=kinoko_graphics.device;
    if(mode<1 || mode>4) {
        device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SRCBLEND,mode==5?D3DBLEND_DESTCOLOR:D3DBLEND_ONE);
        device->SetRenderState(D3DRS_DESTBLEND,mode==5?D3DBLEND_ONE:D3DBLEND_ZERO);
        return;
    }
    if(g703==mode) return;
    auto* cached=pointer<IDirect3DDevice9>(g702);
    auto op=[&](DWORD value){cached->SetRenderState(D3DRS_BLENDOP,value);};
    auto src=[&](DWORD value){cached->SetRenderState(D3DRS_SRCBLEND,value);};
    auto dst=[&](DWORD value){cached->SetRenderState(D3DRS_DESTBLEND,value);};
    // Original 42AC20 shares the transition key with 402770. Case 32 has
    // no blend-op write here, so do not substitute that near-duplicate helper.
    switch(8*g703+mode-1) {
    case 0:op(1);src(5);dst(6);break;
    case 1:op(1);src(5);dst(2);break;
    case 2:case 34:op(3);src(5);dst(2);break;
    case 3:case 27:op(1);src(1);dst(3);break;
    case 9:dst(2);break;
    case 10:op(3);dst(2);break;
    case 11:case 19:src(1);dst(3);break;
    case 16:dst(6);break;
    case 18:op(3);break;
    case 24:op(1);dst(6);break;
    case 25:op(1);break;
    case 32:src(5);dst(6);break;
    case 33:src(5);dst(2);break;
    default:break;
    }
    g703=mode;
}
}
extern "C" int32_t kinoko_string_add_character(int32_t layout,const char* character) {
    auto& cursor=field<int32_t>(layout+204);
    if(*character=='\t') { const int32_t tab=4*field<int32_t>(layout+88);cursor+=tab-cursor%tab;return 1; }
    if(*character=='\n') {
        field<int32_t>(layout+208)+=field<int32_t>(layout+216);cursor=0;
        field<int32_t>(layout+216)=field<int32_t>(layout+88);return 1;
    }
    for(;;) {
        if(kinoko_string_atlas_size(layout)==0) new_page(layout);
        const int32_t page=kinoko_string_atlas_at(layout,kinoko_string_atlas_size(layout)-1);
        kinoko_string_font_configure(page+24,layout);
        int32_t width=0,height=0;
        kinoko_string_font_upload(page+24,field<int32_t>(page+428),character,
            field<int32_t>(page),field<int32_t>(page+4),&width,&height);
        if(width>=field<int32_t>(page+12) || height>=field<int32_t>(page+16)) return 0;
        if(field<int32_t>(page)+width>=field<int32_t>(page+12) ||
            (!width && field<int32_t>(page+16)-field<int32_t>(page+8)-field<int32_t>(page+4)>field<int32_t>(layout+88))) {
            field<int32_t>(page+4)+=field<int32_t>(page+8);
            field<int32_t>(page)=field<int32_t>(page+8)=0;
            continue;
        }
        if(field<int32_t>(page+4)+height>=field<int32_t>(page+16) || !height) {new_page(layout);continue;}
        // 440A9B-440BF4 is absent from IDA's decompilation: width/height are
        // output parameters of 405F80, not constants. Follow the assembly.
        ++field<int32_t>(page+432);
        const int32_t glyph=kinoko_string_append_glyph(layout),s=glyph+20;
        rectangle(s,field<int32_t>(page+428),field<int32_t>(page),field<int32_t>(page+4),width,height);
        std::copy_n(pointer<unsigned char>(s+128),48,pointer<unsigned char>(s+176));
        field<int32_t>(glyph)=cursor;field<int32_t>(glyph+4)=field<int32_t>(layout+208);
        field<int32_t>(glyph+12)=width;field<int32_t>(glyph+16)=height;
        field<int32_t>(glyph+8)=field<int32_t>(page+20)=field<int32_t>(layout+200);
        field<int32_t>(glyph+252)=page;++field<uint32_t>(layout+200);
        field<int32_t>(page+8)=(std::max)(field<int32_t>(page+8),height);
        field<int32_t>(page)+=width;cursor+=width;
        auto& line_height=field<int32_t>(layout+216);line_height=(std::max)(line_height,height);
        if(field<int32_t>(layout+144)>=0 && cursor>=field<int32_t>(layout+144)) {
            field<int32_t>(layout+208)+=line_height;cursor=0;line_height=field<int32_t>(layout+88);
        }
        field<int32_t>(layout+212)=(std::max)(field<int32_t>(layout+212),cursor);
        return 1;
    }
}
extern "C" int32_t __fastcall kinoko_method_set_string_layer(int32_t object,void*,int32_t layer) {
    if(!layer) return E_FAIL;
    field<int32_t>(object+148)=layer;return 0;
}
extern "C" int32_t __fastcall kinoko_method_update_string_layout(int32_t layout,void*) {
    const int32_t layer=field<int32_t>(layout+148);
    if(!layer) return E_FAIL;
    if(field<uint8_t>(layout+228)) {function_4410c0(layout);field<uint8_t>(layout+228)=0;}
    StringView pending(pointer<void>(layout+32)),text(pointer<void>(layout+4));
    while(pending.length()) {
        const auto bytes=static_cast<uint32_t>(CharNextA(pending.data())-pending.data());
        char character[8]{};memcpy_s(character,sizeof(character),pending.data(),bytes);
        kinoko_string_add_character(layout,character);
        text.append(character,static_cast<uint32_t>(std::strlen(character)));
        pending.assign(pending,bytes,UINT32_MAX);
    }
    if(!field<uint8_t>(layer+140)) return 0;
    const uint32_t color=(static_cast<uint32_t>(static_cast<int64_t>(field<float>(layout+152)*255.0))<<24)|
        (uint32_t(field<uint8_t>(layout+108))<<16)|(uint32_t(field<uint8_t>(layout+112))<<8)|field<uint8_t>(layout+116);
    const uint32_t count=kinoko_string_queue_size(layout);
    for(uint32_t i=0;i<count;++i)
        for(int offset:{24,52,80,108}) field<uint32_t>(glyph_at(layout,i)+20+offset)=color;
    float x=0,y=0,z=0;
    using Position=void(__thiscall*)(int32_t,float*,float*,float*);
    field<Position>(field<int32_t>(layer)+28)(layer,&x,&y,&z);
    field<int32_t>(layout+220)=static_cast<int32_t>(x);field<int32_t>(layout+224)=static_cast<int32_t>(y);
    for(uint32_t i=0;i<count;++i) {
        const int32_t glyph=glyph_at(layout,i),s=glyph+20;
        float gx=static_cast<float>(field<int32_t>(glyph));
        if(field<int32_t>(layout+132)==1) gx-=field<int32_t>(layout+212)/2;
        if(field<int32_t>(layout+132)==2) gx-=field<int32_t>(layout+212);
        const float sx=field<float>(layout+136),sy=field<float>(layout+140);
        const float dx=static_cast<float>(double(sx)*gx+field<int32_t>(layout+220));
        const float dy=static_cast<float>((double(field<int32_t>(glyph+16))-double(field<int32_t>(glyph+16))*sy)*0.5
            +field<int32_t>(layout+224)+double(sy)*static_cast<float>(field<int32_t>(glyph+4)));
        std::copy_n(pointer<unsigned char>(s+128),48,pointer<unsigned char>(s+176));
        for(int vertex=0;vertex<4;++vertex) {
            auto* p=pointer<float>(s+176+vertex*12);
            p[0]*=sx;p[1]*=sy;p[2]*=1.0f;
            p[0]+=dx;p[1]+=dy;p[2]+=0.0f;
        }
    }
    return 0;
}
extern "C" int32_t __fastcall kinoko_method_draw_string_layout(int32_t layout,void*,float x,float y) {
    const int32_t layer=field<int32_t>(layout+148);
    if(!layer) return E_FAIL;
    if(!field<uint8_t>(layer+140)) return 0;
    auto* device=kinoko_graphics.device;
    DWORD saved[4]{};
    const D3DRENDERSTATETYPE states[]={D3DRS_SRCBLEND,D3DRS_DESTBLEND,D3DRS_BLENDOP,D3DRS_ALPHABLENDENABLE};
    for(int i=0;i<4;++i) device->GetRenderState(states[i],&saved[i]);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);blend(field<int32_t>(layout+156));
    if(g704!=2) {
        auto* cached=pointer<IDirect3DDevice9>(g702);
        for(auto type:{D3DSAMP_MAGFILTER,D3DSAMP_MINFILTER,D3DSAMP_MIPFILTER}) cached->SetSamplerState(0,type,2);
        g704=2;
    }
    const uint32_t count=kinoko_string_queue_size(layout);
    for(uint32_t i=0;i<count;++i) kinoko_quad_submit(kinoko::legacy::pointer<KinokoQuad>(glyph_at(layout,i)+20),x,y);
    for(int i=0;i<4;++i) device->SetRenderState(states[i],saved[i]);
    return 0;
}
