struct SQVM;
#pragma once
#include "kinoko/act_layout_records.hpp"
#include "kinoko/legacy_string.hpp"
#include <cstdint>
namespace kinoko::mesh {
struct ResourceState;
struct RenderLink;
struct Resource {
    const void *methods;
    int32_t id;
    legacy::StringRecord name;uint32_t pad32;
    legacy::StringRecord mesh_name;uint32_t pad60;
    legacy::StringRecord prefix;uint32_t pad88;
    // Replaces the private 144-byte VC8 controller's container storage.
    ResourceState *state;
    uint8_t reserved_controller[140];
    RenderLink *renders;
    uint32_t render_count,pad244;
};
static_assert(sizeof(Resource)==248 && offsetof(Resource,renders)==236);
Resource *create_resource();
int32_t read_resource_properties(Resource *resource,int32_t *reader_holder,int32_t version);
void clear_resource(Resource *resource);
uint8_t load_resource(Resource *resource,const char *prefix);
int32_t replace_texture(Resource *resource,const char *name,KinokoActResource *texture);
const void *resource_methods();
const void *layout_methods();
act::Layout3DRecord *create_layout();
uint32_t resource_type();
uint32_t layout_type();
}
extern "C" {
int32_t kinoko_publish_mesh_resource_class(struct SQVM*,int32_t,int32_t *);
int32_t __fastcall kinoko_method_read_mesh_resource(int32_t,void *,int32_t,int32_t);
int32_t __fastcall kinoko_method_write_mesh_resource(int32_t,void *,int32_t);
int32_t __fastcall kinoko_method_write_layout_3d(int32_t,void *,int32_t);
int32_t __fastcall kinoko_method_register_mesh_resource(int32_t,void *,struct SQVM*);
int32_t __fastcall kinoko_method_bind_mesh_object(int32_t,void *,int32_t,const char *);
int32_t __fastcall kinoko_method_bind_mesh_table(int32_t,void *,int32_t,const char *);
int32_t __fastcall kinoko_method_register_layout_3d(int32_t,void *);
}
