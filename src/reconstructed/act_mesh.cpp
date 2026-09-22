#include "kinoko/act_mesh.hpp"
#include "kinoko/mesh_model.hpp"
#include "kinoko/act_layout_3d.hpp"
#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/boost_hash.h"
#include "kinoko/graphics_device.h"
#include "kinoko/com_owner.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

extern "C" int32_t function_43c860_this(int32_t,int32_t,int32_t);
namespace kinoko::mesh {
namespace {
using legacy::address;
using legacy::pointer;
using legacy::StringView;
struct ReaderOwner {
    KinokoArchiveReader *reader{};
    explicit ReaderOwner(const char *path) { kinoko_reader_open(&reader,path); }
    ~ReaderOwner() { kinoko_reader_close(reader); }
};
template<class T> using Cache=std::map<std::string,std::weak_ptr<T>>;
Cache<Node> models;
Cache<Material> materials;
template<class T,class Read> std::shared_ptr<T> acquire(Cache<T> &cache,const std::string &name,Read read) {
    if(auto found=cache.find(name);found!=cache.end())
        if(auto existing=found->second.lock()) return existing;
    ReaderOwner input(name.c_str());
    if(!input.reader) return {};
    std::shared_ptr<T> value(read(input.reader));
    if(value) cache[name]=value;
    return value;
}
// 457310 owns controller children, borrows models retained by its controller,
// and expands reference children before the node's own serialized children.
struct Controller {
    const Node *model{};
    int32_t registry_index{},reference_group{};
    std::vector<std::unique_ptr<Controller>> children;
    std::vector<std::shared_ptr<Material>> material_owners;
};
struct Renderer {
    const void *methods{};
    const Node *model{}; // borrowed from ResourceState
    ComOwner<IDirect3DIndexBuffer9> indices;
    ComOwner<IDirect3DVertexBuffer9> positions,normals,coordinates;
    ComOwner<IDirect3DVertexDeclaration9> declaration;
    // Original replacement handles are borrowed; insertion does not AddRef.
    std::multimap<std::string,int32_t> replacements;
};
static const D3DVERTEXELEMENT9 elements[]={
    {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
    {1,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0},
    {2,0,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},
    D3DDECL_END()
};
template<class Buffer> void fill(Buffer *buffer,const void *data,size_t size) {
    if(!buffer) return;
    void *destination=nullptr;
    if(SUCCEEDED(buffer->Lock(0,0,&destination,0))) {
        if(size) std::memcpy(destination,data,size);
        buffer->Unlock();
    }
}
bool bind_mesh(Renderer &render,const Node *model) {
    render.model=model;
    auto *device=kinoko_graphics.device;
    if(!device || !model || !model->geometry) return false;
    const auto &mesh=*model->geometry;
    // 456690 intentionally creates 16-bit indices even though the file reader
    // accepts 32-bit arrays. Preserve the existing renderer's byte count.
    bool failed=FAILED(device->CreateIndexBuffer(mesh.index_count*2,0,D3DFMT_INDEX16,
        D3DPOOL_MANAGED,render.indices.put(),nullptr));
    const auto vertex_count=static_cast<UINT>(mesh.positions.size());
    failed=FAILED(device->CreateVertexBuffer(vertex_count*12,0,D3DFVF_XYZ,D3DPOOL_MANAGED,render.positions.put(),nullptr))||failed;
    failed=FAILED(device->CreateVertexBuffer(vertex_count*12,0,D3DFVF_NORMAL,D3DPOOL_MANAGED,render.normals.put(),nullptr))||failed;
    failed=FAILED(device->CreateVertexBuffer(vertex_count*8,0,D3DFVF_TEX1,D3DPOOL_MANAGED,render.coordinates.put(),nullptr))||failed;
    failed=FAILED(device->CreateVertexDeclaration(elements,render.declaration.put()))||failed;
    if(failed) {
        render.indices.reset();render.positions.reset();render.normals.reset();
        render.coordinates.reset();render.declaration.reset();return false;
    }
    fill(render.positions.get(),mesh.positions.data(),mesh.positions.size()*12);
    if(!mesh.layers.empty()) {
        const auto &layer=mesh.layers.front();
        // Reject corrupt oversized payloads instead of overwriting a COM buffer.
        if(layer.normals.size()>vertex_count || layer.coordinates.size()>vertex_count) return false;
        fill(render.coordinates.get(),layer.coordinates.data(),layer.coordinates.size()*8);
        fill(render.normals.get(),layer.normals.data(),layer.normals.size()*12);
    }
    fill(render.indices.get(),mesh.indices.data(),mesh.index_count*2);
    return true;
}
uint8_t __fastcall draw_mesh(Renderer *render,void *) {
    auto *device=kinoko_graphics.device;
    if(!device || !render->model || !render->model->geometry || !render->indices ||
        !render->positions || !render->normals || !render->coordinates || !render->declaration) return 0;
    const auto &mesh=*render->model->geometry;
    if(!mesh.visible) return 1;
    device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
    D3DMATRIX world{};device->GetTransform(D3DTS_WORLD,&world);
    // ACT's 44CA60 calls SetMesh, not SetController. Consequently no controller
    // matrix is multiplied here; adding node/world transforms would change it.
    device->SetVertexDeclaration(render->declaration.get());
    device->SetStreamSource(0,render->positions.get(),0,12);
    device->SetStreamSource(1,render->normals.get(),0,12);
    device->SetStreamSource(2,render->coordinates.get(),0,8);
    device->SetIndices(render->indices.get());
    for(const auto &attribute:mesh.attributes) {
        if(attribute.materials.empty()) continue;
        const auto material=attribute.materials.front();
        if(material<0 || static_cast<size_t>(material)>=mesh.material_names.size()) continue;
        const auto found=render->replacements.find(mesh.material_names[material]);
        retdec_set_texture_stage(0,found==render->replacements.end()?0:found->second);
        device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,attribute.minimum_vertex,
            attribute.vertex_count,attribute.start_index,attribute.primitive_count);
    }
    retdec_set_texture_stage(0,0);retdec_set_texture_stage(0,0);
    // 45662A..45665A retains the original final XYZ/RHW/diffuse triangle.
    struct DebugVertex { float x,y,z,rhw;DWORD color; };
    static const DebugVertex triangle[]={{200,10,1,1,0xffff0000},{400,200,1,1,0xffff0000},{10,200,1,1,0xffff0000}};
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,1,triangle,sizeof(DebugVertex));
    device->SetTransform(D3DTS_WORLD,&world);
    return 1;
}
const void *render_methods[]={nullptr,nullptr,nullptr,reinterpret_cast<const void*>(draw_mesh)};
}
struct RenderLink { RenderLink *next,*previous;Renderer render; };
static_assert(offsetof(RenderLink,render)==8);
struct ResourceState {
    std::unique_ptr<Controller> root;
    std::vector<std::shared_ptr<Node>> model_owners;
    std::vector<Controller*> registry;
    std::map<std::string,Controller*> named;
};
namespace {
std::unique_ptr<Controller> make_controller(ResourceState &state,const Node *model,
    int32_t group,const char *prefix,std::vector<const Node*> &ancestors) {
    if(std::find(ancestors.begin(),ancestors.end(),model)!=ancestors.end())
        throw std::runtime_error("cyclic MSH reference");
    ancestors.push_back(model);
    auto node=std::make_unique<Controller>();node->model=model;node->reference_group=group;
    const auto append=[&](const Node *child,int32_t child_group) {
        auto owned=make_controller(state,child,child_group,prefix,ancestors);
        owned->registry_index=static_cast<int32_t>(state.registry.size());
        state.registry.push_back(owned.get());state.named[child->name]=owned.get();
        node->children.push_back(std::move(owned));
    };
    if(model->type==NodeType::reference) {
        const auto name=prefix?std::string(prefix)+model->name+".msh":model->name;
        if(auto referenced=acquire(models,name,read_model)) {
            state.model_owners.push_back(referenced);
            for(const auto &child:referenced->children) append(child.get(),model->reference_group);
        }
    }
    for(const auto &child:model->children) append(child.get(),group);
    ancestors.pop_back();
    return node;
}
void load_materials(Controller &node,const char *prefix) {
    if(node.model->geometry) for(const auto &name:node.model->geometry->material_names) {
        const auto path=(prefix?std::string(prefix):std::string{})+name+".mat";
        node.material_owners.push_back(acquire(materials,path,read_material));
    }
    for(auto &child:node.children) load_materials(*child,prefix);
}
void clear_renders(Resource &resource) {
    auto *head=resource.renders;
    if(!head) return;
    while(head->next!=head) { auto *node=head->next;head->next=node->next;delete node; }
    head->previous=head;resource.render_count=0;
}
void collect_renders(Resource &resource,const Node *model) {
    if(model->type==NodeType::mesh) {
        auto node=std::make_unique<RenderLink>();
        node->render.methods=render_methods;
        bind_mesh(node->render,model); // original retains the node even on bind failure
        auto *head=resource.renders;
        node->next=head;node->previous=head->previous;
        head->previous->next=node.get();head->previous=node.get();
        node.release();++resource.render_count;
    }
    for(const auto &child:model->children) collect_renders(resource,child.get());
}
}
uint8_t load_resource(Resource *resource,const char *prefix) {
    const StringView name(&resource->mesh_name),stored_prefix(&resource->prefix);
    if(!name.length()) return 0;
    // Preserve null-prefix reuse and avoid alias invalidation when assigning it.
    const std::string path=prefix?prefix:stored_prefix.data();
    stored_prefix.assign(path.data(),static_cast<uint32_t>(path.size()));
    clear_renders(*resource);
    delete resource->state;resource->state=nullptr;
    resource->state=new ResourceState;
    auto &state=*resource->state;
    auto model=acquire(models,name.data(),read_model);
    if(model) {
        state.model_owners.push_back(model);
        std::vector<const Node*> ancestors;
        state.root=make_controller(state,model.get(),0,path.c_str(),ancestors);
        state.root->registry_index=static_cast<int32_t>(state.registry.size());
        state.registry.push_back(state.root.get());state.named[model->name]=state.root.get();
        load_materials(*state.root,path.c_str());
        // ORIGINAL BUG FIX — explicitly authorized by user on 2026-09-22.
        // 44CA41 used controller+20 (integer registry_index) as a model pointer.
        // Use controller+24's model instead. See ORIGINAL-BUG-FIX.md.
        collect_renders(*resource,state.root->model);
    }
    // 44CA50 returns false even after loading; this is NOT silently repaired.
    return 0;
}
void clear_resource(Resource *resource) {
    if(!resource) return;
    // 44C1C0 tears down the controller before the draw records.
    delete resource->state;resource->state=nullptr;
    clear_renders(*resource);delete resource->renders;resource->renders=nullptr;
    StringView(&resource->prefix).destroy();StringView(&resource->mesh_name).destroy();StringView(&resource->name).destroy();
}
int32_t replace_texture(Resource *resource,const char *name,KinokoActResource *texture) {
    if(!name || !texture) return E_FAIL;
    struct Descriptor { void *methods,*cache;char name[40]; };
    static const Descriptor target{nullptr,nullptr,".?AVCActRenderTarget@@"},image{nullptr,nullptr,".?AVCActResource2D@@"};
    using Query=uint8_t(__thiscall *)(KinokoActResource*,const void*,void**);
    const auto query=legacy::load<Query>(legacy::load<unsigned char*>(texture)+8);
    void *converted=nullptr;
    if(!query(texture,&target,&converted) && !query(texture,&image,&converted)) return E_FAIL;
    const auto handle=legacy::load<int32_t>(static_cast<unsigned char*>(converted)+68);
    for(auto *node=resource->renders->next;node!=resource->renders;node=node->next)
        if(*name && handle) node->render.replacements.emplace(name,handle); // map insert does not overwrite
    return S_OK;
}
namespace {
uint32_t hash_name(const char *name) {return static_cast<uint32_t>(kinoko_boost_hash_range(address(name),address(name+std::strlen(name))));}
Resource *__fastcall clone_resource(Resource *source,void *) {
    auto *copy=create_resource();
    if(!copy) return nullptr;
    try {
        copy->id=source->id;
        for(auto member:{&Resource::name,&Resource::mesh_name,&Resource::prefix}) {
            StringView input(&(source->*member));StringView(&(copy->*member)).assign(input.data(),input.length());
        }
        load_resource(copy,StringView(&copy->prefix).data());return copy;
    } catch(...) {clear_resource(copy);std::free(copy);return nullptr;}
}
void *__fastcall destroy_resource(Resource *resource,void *,uint32_t flags) {
    clear_resource(resource);if(flags&1) std::free(resource);return resource;
}
int32_t __fastcall dispose_resource(Resource *resource,void *) {destroy_resource(resource,nullptr,1);return 0;}
uint8_t __fastcall load_method(Resource *resource,void *,const char *prefix) {
    try{return load_resource(resource,prefix);}catch(...){return 0;}
}
act::Layout3DRecord *__fastcall clone_layout(act::Layout3DRecord *layout,void *) {return act::clone_layout_3d(layout);}
int32_t __fastcall bind_layout(act::Layout3DRecord *layout,void *,KinokoActLayer *owner) {return act::bind_layout_3d(layout,owner);}
int32_t __fastcall update_layout(act::Layout3DRecord *layout,void *) {return act::update_layout_3d(layout);}
int32_t __fastcall draw_layout(act::Layout3DRecord *layout,void *,float,float) {return act::draw_layout_3d(layout);}
void *__fastcall destroy_layout(act::Layout3DRecord *layout,void *,uint32_t flags) {if(flags&1)std::free(layout);return layout;}
int32_t __fastcall dispose_layout(act::Layout3DRecord *layout,void *) {std::free(layout);return 0;}
int32_t __fastcall read_layout(int32_t layout,void *,int32_t holder,int32_t version) {return function_43c860_this(layout,holder,version);}
// Named serialization supplies type hashes directly; retain usable GetType/
// GetName metadata for legacy callers rather than a numeric-address binder.
struct TypeInfo {const void *methods;const char *name;};
void *__fastcall type_name(TypeInfo *type,void *,void *output) {StringView(output).assign(type->name,static_cast<uint32_t>(std::strlen(type->name)));return output;}
const void *type_methods[]={nullptr,reinterpret_cast<const void*>(type_name)};
TypeInfo mesh_info{type_methods,".?AVCActResourceMesh@@"},layout_info{type_methods,".?AVC3DLayout@@"};
TypeInfo *__fastcall mesh_type_method(void *,void *) {return &mesh_info;}
TypeInfo *__fastcall layout_type_method(void *,void *) {return &layout_info;}
#define ENTRY(f) reinterpret_cast<const void*>(f)
const void *mesh_methods[]={ENTRY(kinoko_method_write_mesh_resource),ENTRY(kinoko_method_read_mesh_resource),
    ENTRY(kinoko_method_query_serializable),ENTRY(dispose_resource),ENTRY(destroy_resource),ENTRY(mesh_type_method),
    ENTRY(kinoko_method_register_mesh_resource),ENTRY(kinoko_method_bind_mesh_table),ENTRY(kinoko_method_bind_mesh_object),
    ENTRY(clone_resource),ENTRY(load_method)};
const void *layout_table[]={ENTRY(kinoko_method_write_layout_3d),ENTRY(read_layout),ENTRY(kinoko_method_query_serializable),
    ENTRY(dispose_layout),ENTRY(layout_type_method),ENTRY(clone_layout),ENTRY(bind_layout),ENTRY(update_layout),ENTRY(draw_layout),
    ENTRY(kinoko_method_register_layout_3d),ENTRY(destroy_layout)};
#undef ENTRY
}
const void *resource_methods(){return mesh_methods;}
const void *layout_methods(){return layout_table;}
uint32_t resource_type(){static const auto type=hash_name(mesh_info.name);return type;}
uint32_t layout_type(){static const auto type=hash_name(layout_info.name);return type;}
Resource *create_resource() {
    auto *resource=static_cast<Resource*>(std::calloc(1,sizeof(Resource)));
    if(!resource)return nullptr;
    resource->methods=resource_methods();resource->id=-1;
    resource->name.capacity=resource->mesh_name.capacity=resource->prefix.capacity=15;
    try {
        resource->state=new ResourceState;
        resource->renders=new RenderLink{};
        resource->renders->next=resource->renders->previous=resource->renders;
        StringView(&resource->name).assign("Resource#",9);return resource;
    }catch(...){clear_resource(resource);std::free(resource);return nullptr;}
}
act::Layout3DRecord *create_layout() {
    auto *layout=static_cast<act::Layout3DRecord*>(std::calloc(1,sizeof(act::Layout3DRecord)));
    if(layout){layout->methods=layout_methods();layout->scale={1,1,1};}return layout;
}
}
