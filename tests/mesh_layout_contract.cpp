#define CINTERFACE
#include "kinoko/mesh_model.hpp"
#include "kinoko/act_layout_3d.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/act_host.h"
#include "kinoko/map_layout_records.hpp"
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <vector>

KinokoGraphics kinoko_graphics{};
namespace {
std::vector<uint8_t> bytes;
size_t cursor{};
void require(bool value) { if (!value) throw std::runtime_error("mesh/layout contract"); }
void word(uint32_t value) {
    for (unsigned n=0;n!=4;++n) bytes.push_back(static_cast<uint8_t>(value>>(8*n)));
}
void zeros(size_t count) { bytes.insert(bytes.end(),count,0); }
void name(const char *value) {
    word(static_cast<uint32_t>(std::strlen(value)));
    bytes.insert(bytes.end(),value,value+std::strlen(value));
}
void matrix() { for (int i=0;i!=16;++i) word(i%5==0?0x3f800000:0); }
void node_tail(uint32_t version,const char *label,uint32_t children) {
    name(label);matrix();
    if(version>=12) word(42);
    if(version>=14) word(71);
    word(children);
}
void format_contract(uint32_t version) {
    bytes.clear();cursor=0;
    word(version);node_tail(version,"root",1);word(2);
    bytes.push_back(1);zeros(40); // visible, bounds, origin, radius
    word(3);bytes.insert(bytes.end(),{0,0,1,0,2,0});
    word(3);zeros(36); // positions
    word(0); // separate mesh normals
    word(1);name("stone");
    word(1);word(0);word(0);word(3);word(1);word(1);word(0); // attribute
    word(1);word(3);zeros(36);word(3);zeros(24); // layer normals/UV
    if(version>=11) { word(3);word(0x12345678);word(0x87654321);word(0xffaabbcc); }
    word(1);word(1);zeros(16);word(1);name("bone");matrix(); // skin
    word(1);word(1);zeros(24); // shape: one position and normal
    word(1);word(0);word(1);word(0); // shape vertex/normal index arrays
    node_tail(version,"triangle",1);word(1);node_tail(version,"reference",0);
    auto root=kinoko::mesh::read_model(1);
    require(root && cursor==bytes.size() && root->name=="root");
    require(root->reference_group==(version>=12?42:0));
    require(root->flags==(version>=14?71:0));
    require(root->children.size()==1 && root->children[0]->children.size()==1);
    auto &mesh=*root->children[0]->geometry;
    require(mesh.index_count==3 && mesh.indices[4]==2);
    require(mesh.material_names[0]=="stone" && mesh.attributes[0].primitive_count==1);
    require(mesh.layers[0].coordinates.size()==3);
    require(mesh.layers[0].colors.size()==(version>=11?3:0));
    require(mesh.skins[0].bone_names[0]=="bone" && mesh.skins[0].bone_matrices[0][15]==1.0f);
    require(mesh.shapes[0].positions.size()==1);
    require(root->children[0]->children[0]->type==kinoko::mesh::NodeType::reference);
    bytes.pop_back();cursor=0;
    bool rejected=false;
    try { (void)kinoko::mesh::read_model(1); } catch (const std::runtime_error &) { rejected=true; }
    require(rejected);
}
DWORD states[256]{};
D3DMATRIX transform{};
int submissions{};
HRESULT WINAPI get_state(IDirect3DDevice9 *,D3DRENDERSTATETYPE key,DWORD *value) { *value=states[key];return S_OK; }
HRESULT WINAPI set_state(IDirect3DDevice9 *,D3DRENDERSTATETYPE key,DWORD value) { states[key]=value;return S_OK; }
HRESULT WINAPI get_transform(IDirect3DDevice9 *,D3DTRANSFORMSTATETYPE,D3DMATRIX *value) { *value=transform;return S_OK; }
HRESULT WINAPI set_transform(IDirect3DDevice9 *,D3DTRANSFORMSTATETYPE,const D3DMATRIX *value) { transform=*value;return S_OK; }
bool valid_type=true;
uint8_t __fastcall query(void *resource,void *,const void *,void **out) { *out=valid_type?resource:nullptr;return valid_type; }
uint8_t __fastcall draw(void *,void *) { ++submissions;return 0; }
void layout_contract() {
    using namespace kinoko::act;
    IDirect3DDevice9Vtbl device_methods{};
    device_methods.GetRenderState=get_state;device_methods.SetRenderState=set_state;
    device_methods.GetTransform=get_transform;device_methods.SetTransform=set_transform;
    IDirect3DDevice9 device{&device_methods};kinoko_graphics.device=&device;
    void *resource_methods[3]={nullptr,nullptr,reinterpret_cast<void*>(query)};
    void *render_methods[4]={nullptr,nullptr,nullptr,reinterpret_cast<void*>(draw)};
    struct Link { Link *next,*previous;void *methods; } head{},item{};
    head.next=head.previous=&item;item.next=item.previous=&head;item.methods=render_methods;
    struct Resource { void *methods;uint8_t padding[232];Link *head; } resource{resource_methods,{},&head};
    kinoko::map::LayerRecord owner{};
    owner.visible=1;owner.resource=reinterpret_cast<KinokoActResource*>(&resource);
    Layout3DRecord layout{};layout.scale={2,3,4};layout.translation={1,2,3};
    require(bind_layout_3d(&layout,reinterpret_cast<KinokoActLayer*>(&owner))==S_OK);
    require(update_layout_3d(&layout)==S_OK);
    require(layout.world[12]==2 && layout.world[13]==6 && layout.world[14]==12);
    states[D3DRS_ZENABLE]=2;states[D3DRS_ZWRITEENABLE]=0;states[D3DRS_ALPHABLENDENABLE]=0;
    transform._41=99;
    require(draw_layout_3d(&layout)==S_OK && submissions==1);
    require(transform._41==99 && states[D3DRS_ZENABLE]==2 && states[D3DRS_ZWRITEENABLE]==0);
    valid_type=false;
    require(draw_layout_3d(&layout)==E_FAIL && submissions==1);
    require(transform._41==2 && states[D3DRS_ZENABLE]==1 && states[D3DRS_ZWRITEENABLE]==1);
    owner.visible=0;layout.translation.x=9;
    require(update_layout_3d(&layout)==S_OK && layout.world[12]==2);
    auto *copy=clone_layout_3d(&layout);
    require(copy && copy->layer==layout.layer && std::memcmp(copy->world,layout.world,64)==0);
    std::free(copy);
}
}
extern "C" int32_t retdec_reader_read_exact(int32_t,void *out,uint32_t size) {
    if (size>bytes.size()-cursor) return 0;
    std::memcpy(out,bytes.data()+cursor,size);cursor+=size;return 1;
}
int main() {
    try { for(auto version:{10u,11u,12u,14u}) format_contract(version);layout_contract(); }
    catch (...) { return 1; }
    return 0;
}
