#include "kinoko/act_layout_3d.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>
#include <cstring>

// The repository's import library exposes undecorated x86 symbols. The DLL
// functions still use stdcall; keep that calling convention at every call site.
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma comment(linker,"/alternatename:__imp__D3DXMatrixRotationYawPitchRoll@16=__imp__D3DXMatrixRotationYawPitchRoll")
#pragma comment(linker,"/alternatename:__imp__D3DXMatrixTranslation@16=__imp__D3DXMatrixTranslation")
#pragma comment(linker,"/alternatename:__imp__D3DXMatrixScaling@16=__imp__D3DXMatrixScaling")
#pragma comment(linker,"/alternatename:__imp__D3DXMatrixMultiply@12=__imp__D3DXMatrixMultiply")
#endif

extern "C" {
__declspec(dllimport) D3DMATRIX *__stdcall D3DXMatrixRotationYawPitchRoll(D3DMATRIX *,float,float,float);
__declspec(dllimport) D3DMATRIX *__stdcall D3DXMatrixTranslation(D3DMATRIX *,float,float,float);
__declspec(dllimport) D3DMATRIX *__stdcall D3DXMatrixScaling(D3DMATRIX *,float,float,float);
__declspec(dllimport) D3DMATRIX *__stdcall D3DXMatrixMultiply(D3DMATRIX *,const D3DMATRIX *,const D3DMATRIX *);
}

namespace kinoko::act {
namespace {
using kinoko::legacy::load;
using Query = uint8_t (__thiscall *)(KinokoActResource *,const void *,void **);
struct ResourceMethods { void *write,*read; Query query; };
struct ResourcePrefix { const ResourceMethods *methods; };
struct RenderNode;
using Draw = uint8_t (__thiscall *)(void *);
struct RenderMethods { void *destroy,*set_controller,*set_mesh; Draw draw; };
struct RenderPrefix { const RenderMethods *methods; };
struct RenderNode { RenderNode *next,*previous; RenderPrefix render; };
struct MeshListPrefix { unsigned char preceding[236]; RenderNode *head; };
static_assert(offsetof(RenderNode,render)==8);
static_assert(offsetof(MeshListPrefix,head)==236);
}

// 43C690 does not install the 2D layer's transform-property aliases.
int32_t bind_layout_3d(Layout3DRecord *layout, KinokoActLayer *layer) {
    if (!layer) return E_FAIL;
    layout->layer=layer;
    return S_OK;
}

int32_t update_layout_3d(Layout3DRecord *layout) {
    if (!layout->layer) return E_FAIL;
    const map::LayerView owner(layout->layer);
    if (!owner.get(&map::LayerRecord::visible)) return S_OK;
    if (!owner.get(&map::LayerRecord::resource)) return E_FAIL;
    D3DMATRIX world{}, operation{};
    world._11=world._22=world._33=world._44=1.0f;
    // 43C920: yaw/pitch/roll in radians, then translation, then scaling.
    // Do not substitute the usual S*R*T or the owning layer's world position.
    D3DXMatrixRotationYawPitchRoll(&operation,layout->rotation.x,layout->rotation.y,layout->rotation.z);
    D3DXMatrixMultiply(&world,&world,&operation);
    D3DXMatrixTranslation(&operation,layout->translation.x,layout->translation.y,layout->translation.z);
    D3DXMatrixMultiply(&world,&world,&operation);
    D3DXMatrixScaling(&operation,layout->scale.x,layout->scale.y,layout->scale.z);
    D3DXMatrixMultiply(&world,&world,&operation);
    std::memcpy(layout->world,&world,sizeof(world));
    return S_OK;
}

int32_t draw_layout_3d(Layout3DRecord *layout) {
    if (!layout->layer) return E_FAIL;
    const map::LayerView owner(layout->layer);
    if (!owner.get(&map::LayerRecord::visible)) return S_OK;
    auto *resource=owner.get(&map::LayerRecord::resource);
    if (!resource) return E_FAIL;
    auto *device=kinoko_graphics.device;
    DWORD depth{},write_depth{},alpha{};
    D3DMATRIX previous{},world{};
    device->GetRenderState(D3DRS_ZENABLE,&depth);
    device->GetRenderState(D3DRS_ZWRITEENABLE,&write_depth);
    device->GetRenderState(D3DRS_ALPHABLENDENABLE,&alpha);
    device->SetRenderState(D3DRS_ZENABLE,1);
    device->SetRenderState(D3DRS_ZWRITEENABLE,1);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,1);
    device->GetTransform(D3DTS_WORLD,&previous);
    std::memcpy(&world,layout->world,sizeof(world));
    device->SetTransform(D3DTS_WORLD,&world);
    struct Descriptor { void *methods,*cache; char name[sizeof(".?AVCActResourceMesh@@")]; };
    static const Descriptor type{nullptr,nullptr,".?AVCActResourceMesh@@"};
    void *converted=nullptr;
    const auto methods=load<ResourcePrefix>(resource).methods;
    // 43CB5D deliberately returns before restoring the state on failed Query.
    if (!methods->query(resource,&type,&converted)) return E_FAIL;
    auto *head=load<MeshListPrefix>(converted).head;
    for (auto *node=head->next;node!=head;node=node->next)
        node->render.methods->draw(&node->render);
    device->SetTransform(D3DTS_WORLD,&previous);
    device->SetRenderState(D3DRS_ZENABLE,depth);
    device->SetRenderState(D3DRS_ZWRITEENABLE,write_depth);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,alpha);
    return S_OK;
}

// 43C220 copies all nine scalar fields, the borrowed layer, and the matrix.
Layout3DRecord *clone_layout_3d(const Layout3DRecord *layout) {
    auto *copy=static_cast<Layout3DRecord *>(std::malloc(sizeof(Layout3DRecord)));
    if (copy) std::memcpy(copy,layout,sizeof(*copy));
    return copy;
}
}
