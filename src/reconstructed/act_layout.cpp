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
extern "C" float function_404130(long double);
extern "C" float function_4040d0(long double);
extern "C" void retdec_trace_i32(const char *,int32_t);
float retdec_sprite_scale_about(float value,float pivot,float scale) {
    return (value-pivot)*scale+pivot;
}
void retdec_sprite_scale_faithful(int32_t sprite,
                                          float scale_x,
                                          float pivot_x,
                                          float scale_y,
                                          float pivot_y,
                                          float scale_z,
                                          float pivot_z)
{
    static const uint32_t x_offsets[4] = { 176, 188, 200, 212 };
    static const uint32_t y_offsets[4] = { 180, 192, 204, 216 };
    static const uint32_t z_offsets[4] = { 184, 196, 208, 220 };
    uint32_t index;

    for (index = 0; index < 4; ++index) {
        float *x = pointer<float>(sprite + x_offsets[index]);
        float *y = pointer<float>(sprite + y_offsets[index]);
        float *z = pointer<float>(sprite + z_offsets[index]);
        *x = retdec_sprite_scale_about(*x, pivot_x, scale_x);
        *y = retdec_sprite_scale_about(*y, pivot_y, scale_y);
        *z = retdec_sprite_scale_about(*z, pivot_z, scale_z);
    }
}

void retdec_sprite_rotate_xy(float *x, float *y,
                                    float pivot_x, float pivot_y,
                                    float angle)
{
    float cosine;
    float sine;
    float old_x;
    float old_y;
    float dx;
    float dy;

    if (angle == 0.0f || x == nullptr || y == nullptr)
        return;
    cosine = function_404130((long double)angle);
    sine = function_4040d0((long double)angle);
    old_x = *x;
    old_y = *y;
    dx = old_x - pivot_x;
    dy = old_y - pivot_y;
    *x = dx * cosine + pivot_x - dy * sine;
    *y = dx * sine + pivot_y + dy * cosine;
}

void retdec_sprite_rotate_faithful(int32_t sprite,
                                           float angle_x,
                                           float angle_y,
                                           float angle_z,
                                           float pivot_x,
                                           float pivot_y,
                                           float pivot_z)
{
    static const uint32_t x_offsets[4] = { 176, 188, 200, 212 };
    static const uint32_t y_offsets[4] = { 180, 192, 204, 216 };
    static const uint32_t z_offsets[4] = { 184, 196, 208, 220 };
    float cosine;
    float sine;
    uint32_t index;

    /* 405320 applies Z, then Y, then X rotation around the supplied pivot. */
    if (angle_z != 0.0f) {
        for (index = 0; index < 4; ++index) {
            retdec_sprite_rotate_xy(
                pointer<float>(sprite + x_offsets[index]),
                pointer<float>(sprite + y_offsets[index]),
                pivot_x, pivot_y, angle_z);
        }
    }
    if (angle_y != 0.0f) {
        cosine = function_404130((long double)angle_y);
        sine = function_4040d0((long double)angle_y);
        for (index = 0; index < 4; ++index) {
            float *x = pointer<float>(sprite + x_offsets[index]);
            float *z = pointer<float>(sprite + z_offsets[index]);
            float old_x = *x;
            float old_z = *z;
            float dx = old_x - pivot_x;
            float dz = old_z - pivot_z;
            *x = dx * cosine + pivot_x + dz * sine;
            *z = dz * cosine + pivot_z - dx * sine;
        }
    }
    if (angle_x != 0.0f) {
        cosine = function_404130((long double)angle_x);
        sine = function_4040d0((long double)angle_x);
        for (index = 0; index < 4; ++index) {
            float *y = pointer<float>(sprite + y_offsets[index]);
            float *z = pointer<float>(sprite + z_offsets[index]);
            float old_y = *y;
            float old_z = *z;
            float dy = old_y - pivot_y;
            float dz = old_z - pivot_z;
            *y = dy * cosine + pivot_y + dz * sine;
            *z = dz * cosine + pivot_z - dy * sine;
        }
    }
}

void retdec_sprite_translate_faithful(int32_t sprite,
                                              float x,
                                              float y,
                                              float z)
{
    static const uint32_t x_offsets[4] = { 176, 188, 200, 212 };
    static const uint32_t y_offsets[4] = { 180, 192, 204, 216 };
    static const uint32_t z_offsets[4] = { 184, 196, 208, 220 };
    uint32_t index;

    for (index = 0; index < 4; ++index) {
        field<float>(sprite + x_offsets[index]) += x;
        field<float>(sprite + y_offsets[index]) += y;
        field<float>(sprite + z_offsets[index]) += z;
    }
}


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
    retdec_sprite_scale_faithful(address(&quad),scale.x,pivot.x,scale.y,pivot.y,scale.z,pivot.z);
    const auto rotation=view.get(&Layout2DRecord::rotation),center=view.get(&Layout2DRecord::rotation_pivot);
    retdec_sprite_rotate_faithful(address(&quad),rotation.x,rotation.y,rotation.z,center.x,center.y,center.z);
    Position3 world{};
    using Position=void (__thiscall *)(KinokoActLayer *,float *,float *,float *);
    const auto *methods=LayerView(layer).get(&Layer::methods);
    legacy::load<Position>(methods+7*sizeof(void*))(layer,&world.x,&world.y,&world.z);
    retdec_sprite_translate_faithful(address(&quad),world.x,world.y,world.z);
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
