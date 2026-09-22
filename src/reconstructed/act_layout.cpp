#include "kinoko/quad_transform.hpp"
#include "kinoko/act_layout_render.hpp"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/quad_render.h"
#include "kinoko/act_runtime.h"
#include "kinoko/act_layer_access.h"
#include "kinoko/texture_store.h"
#include "kinoko/diagnostics.h"
#include "kinoko/game_math.h"
#include <algorithm>
#include <cstring>
using kinoko::legacy::field;
using kinoko::legacy::pointer;
using kinoko::legacy::address;
extern "C" void retdec_trace_i32(const char *,int32_t);
namespace kinoko::act {
namespace {
using native::RecordView;
using View=RecordView<Layout2DRecord>;
using LayerView=kinoko::map::LayerView;
using Layer=kinoko::map::LayerRecord;
using Texture=TextureResourcePrefix;
KinokoActResource *texture_resource(KinokoActLayer *layer) {
    auto *resource=LayerView(layer).get(&Layer::resource);
    if(!resource) return nullptr;
    using Query=uint8_t (__thiscall *)(KinokoActResource *,const void *,KinokoActResource **);
    struct Descriptor { void *methods,*cache;char name[sizeof(".?AVCActResource2D@@")]; };
    static const Descriptor type{nullptr,nullptr,".?AVCActResource2D@@"};
    struct Methods { void *prefix[2];Query query; };
    const auto *methods=legacy::load<const Methods *>(resource);
    KinokoActResource *converted=nullptr;
    return methods->query(resource,&type,&converted) ? converted : nullptr;
}
int32_t bind_texture(KinokoActLayout *layout) {
    const View view(layout);
    auto *layer=view.get(&Layout2DRecord::layer);
    if(!layer) return E_FAIL;
    auto *resource=texture_resource(layer);
    if(!resource) return E_FAIL;
    const RecordView<Texture> texture(resource);
    const auto handle=texture.get(&Texture::texture);
    if(!handle) return E_FAIL;
    if(!view.get(&Layout2DRecord::pivots_initialized)) {
        auto rotation=view.get(&Layout2DRecord::rotation_pivot);
        auto scale=view.get(&Layout2DRecord::scale_pivot);
        rotation.x=scale.x=texture.get(&Texture::source_width)*0.5f;
        rotation.y=scale.y=texture.get(&Texture::source_height)*0.5f;
        view.set(&Layout2DRecord::rotation_pivot,rotation);view.set(&Layout2DRecord::scale_pivot,scale);
        view.set(&Layout2DRecord::pivots_initialized,uint8_t{1});
    }
    view.set(&Layout2DRecord::texture,handle);return 0;
}
}
int32_t bind_layout_2d(KinokoActLayout *layout,KinokoActLayer *layer) {
    if(!layout || !layer) return E_FAIL;
    const View view(layout);
    view.set(&Layout2DRecord::layer,layer);
    // 42BA50 exposes aliases to 17 consecutive transform/color property words.
    struct Aliases { const void *methods;std::array<void *,17> properties; };
    const RecordView<Aliases> aliases(layer);
    auto pointers=aliases.get(&Aliases::properties);
    auto *first=view.bytes(&Layout2DRecord::rotation);
    for(size_t i=0;i<pointers.size();++i) pointers[i]=first+i*sizeof(float);
    aliases.set(&Aliases::properties,pointers);
    if(!view.get(&Layout2DRecord::texture)) bind_texture(layout);
    return 0;
}
int32_t update_layout_2d(KinokoActLayout *layout) {
    if(!layout) return E_FAIL;
    const View view(layout);
    auto *layer=view.get(&Layout2DRecord::layer);
    if(!layer) return E_FAIL;
    if(!LayerView(layer).get(&Layer::visible)) return 0;
    auto *resource=texture_resource(layer);
    if(!resource) return E_FAIL;
    const auto texture=RecordView<Texture>(resource).load();
    if(!view.get(&Layout2DRecord::texture) || view.get(&Layout2DRecord::texture)!=texture.texture) bind_texture(layout);
    const int handle=view.get(&Layout2DRecord::texture);
    if(!handle || static_cast<uint32_t>(handle)>=KINOKO_TEXTURE_CAPACITY) return E_FAIL;
    const auto &slot=kinoko_texture_slots[handle];
    if(!slot.width || !slot.height) return E_FAIL; // inherited invalid-texture boundary
    auto quad=view.get(&Layout2DRecord::quad);
    quad.texture=handle;quad.texture_width=static_cast<float>(slot.width);quad.texture_height=static_cast<float>(slot.height);
    const auto left=static_cast<int32_t>(texture.source_x),top=static_cast<int32_t>(texture.source_y);
    const auto width=static_cast<int32_t>(texture.source_width),height=static_cast<int32_t>(texture.source_height);
    const float u=left/quad.texture_width,v=top/quad.texture_height;
    quad.source_u_extent=width/quad.texture_width;quad.source_v_extent=height/quad.texture_height;
    quad.vertices[0].u=quad.vertices[2].u=u;quad.vertices[0].v=quad.vertices[1].v=v;
    quad.vertices[1].u=quad.vertices[3].u=u+quad.source_u_extent;
    quad.vertices[2].v=quad.vertices[3].v=v+quad.source_v_extent;
    quad.base_positions={{{-0.0f,-0.0f,0},{static_cast<float>(width),-0.0f,0},
        {-0.0f,static_cast<float>(height),0},{static_cast<float>(width),static_cast<float>(height),0}}};
    quad.positions=quad.base_positions;
    const auto scale=view.get(&Layout2DRecord::scale),pivot=view.get(&Layout2DRecord::scale_pivot);
    render::scale_quad(quad,scale,pivot);
    const auto rotation=view.get(&Layout2DRecord::rotation),center=view.get(&Layout2DRecord::rotation_pivot);
    render::rotate_quad(quad,rotation,center);
    Position3 world{};
    using Position=void (__thiscall *)(KinokoActLayer *,float *,float *,float *);
    const auto *methods=LayerView(layer).get(&Layer::methods);
    legacy::load<Position>(methods+7*sizeof(void*))(layer,&world.x,&world.y,&world.z);
    render::translate_quad(quad,world);
    const auto alpha=std::clamp(static_cast<int32_t>(static_cast<double>(view.get(&Layout2DRecord::alpha))*255.0),0,255);
    // Original uses the LOW BYTE of each integer color, not saturation.
    const uint32_t color=(static_cast<uint32_t>(alpha)<<24)|
        (uint32_t(static_cast<uint8_t>(view.get(&Layout2DRecord::red)))<<16)|
        (uint32_t(static_cast<uint8_t>(view.get(&Layout2DRecord::green)))<<8)|
        static_cast<uint8_t>(view.get(&Layout2DRecord::blue));
    for(auto &vertex:quad.vertices) vertex.color=color;
    view.set(&Layout2DRecord::quad,quad);
    return 0;
}
int32_t draw_layout_2d(KinokoActLayout *layout,float x,float y) {
    if(!layout) return E_FAIL;
    const View view(layout);
    auto *layer=view.get(&Layout2DRecord::layer);
    if(!layer) return E_FAIL;
    if(!LayerView(layer).get(&Layer::visible)) return 0;
    if(!texture_resource(layer)) return E_FAIL;
    auto *device=kinoko_graphics.device;
    if(!device) return E_FAIL; // inherited absent-device boundary
    const D3DRENDERSTATETYPE types[]={D3DRS_SRCBLEND,D3DRS_DESTBLEND,D3DRS_BLENDOP,D3DRS_ALPHABLENDENABLE};
    DWORD saved[4]{};
    for(int i=0;i<4;++i) device->GetRenderState(types[i],&saved[i]);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    set_layout_blend(view.get(&Layout2DRecord::blend));
    kinoko_texture_bind_stage(0,view.get(&Layout2DRecord::texture));
    kinoko_quad_submit(reinterpret_cast<KinokoQuad *>(view.bytes(&Layout2DRecord::quad)),x,y);
    kinoko_texture_bind_stage(0,0);
    for(int i=0;i<4;++i) device->SetRenderState(types[i],saved[i]);
    return 0; // original ignores submit HRESULT; update belongs to PrepareDraw
}
}
extern "C" int32_t retdec_c2dlayout_set_layer_impl(int32_t layout,int32_t layer) {
    return kinoko::act::bind_layout_2d(pointer<KinokoActLayout>(layout),pointer<KinokoActLayer>(layer));
}
extern "C" int32_t retdec_c2dlayout_update_faithful_impl(int32_t layout) {
    return kinoko::act::update_layout_2d(pointer<KinokoActLayout>(layout));
}
extern "C" int32_t retdec_c2dlayout_draw_impl(int32_t layout,float x,float y) {
    return kinoko::act::draw_layout_2d(pointer<KinokoActLayout>(layout),x,y);
}
extern "C" void retdec_c2dlayout_world_position(int32_t layer,float *x,float *y,float *z) {
    kinoko_act_layer_world_position(pointer<KinokoActLayer>(layer),nullptr,x,y,z);
}
