#include "kinoko/act_host.h"
#include "kinoko/act_mesh.hpp"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_method_entries.h"
#include <cstring>

using kinoko::legacy::address;
using kinoko::legacy::field;
using kinoko::legacy::pointer;


#include "kinoko/string_layout.h"

extern "C" const char* kinoko_act_serialized_type_name(const void* object) {
    if (!object) return nullptr;
    const auto table=kinoko::legacy::load<const void*>(object);
    const auto* host=kinoko_act_host_symbols();
    if(table==kinoko::mesh::resource_methods()) return ".?AVCActResourceMesh@@";
    if(table==kinoko::mesh::layout_methods()) return ".?AVC3DLayout@@";
    // Recovered records are not modern C++ polymorphic objects. Their explicit
    // vtable identity supplies the original dynamic type at this ABI boundary;
    // never pass their bytes to the compiler's typeid or fake __RTtypeid.
    if (table==host->script_vtable) return ".?AVCActScript@@";
    if (table==host->layer_vtable) return ".?AVCActLayer@@";
    if (table==host->key_vtable) return ".?AVCActKey@@";
    if (table==host->act_vtable) return ".?AVCAct@@";
    if (table==host->layout_vtable) return ".?AVC2DLayout@@";
    if (table==host->map_layout_vtable) return ".?AVC2DMapLayout@@";
    if (table==host->texture_resource_vtable) return ".?AVCActResource2D@@";
    if (table==host->chip_resource_vtable) return ".?AVCActResourceChip@@";
    if (table==host->render_target_vtable) return ".?AVCActRenderTarget@@";
    if (table==kinoko_act_timeline_vtable()) return ".?AVCActTimeLine@@";
    if (table==kinoko_string_layout_methods()) return ".?AVCStringLayout@@";
    return nullptr;
}

extern "C" int32_t __fastcall kinoko_method_query_serializable(
    void* object,void*,const void* type,void* output) {
    if (!output) return 0;
    const auto* name=kinoko_act_serialized_type_name((const void*)(uintptr_t)(object));
    // 4461D0 checks the exact dynamic type (no base-class conversion).
    // Original type_info::operator== at 4AB2E2 compares descriptor+9,
    // intentionally ignoring the leading byte of the raw decorated name.
    const bool match=type && name && std::strcmp(name+1,static_cast<const char*>(type)+9)==0;
    kinoko::legacy::store(output, match ? object : nullptr);
    return match;
}

extern "C" int32_t __fastcall kinoko_method_destroy_serializable(void* object,void*) {
    if (!object) return 0;
    // 446210: only classes with their deleting destructor at slot four use
    // this entry. C2DLayout and C2DMapLayout have a different slot arrangement.
    using Delete = int32_t (__thiscall*)(void*, unsigned char);
    const auto* methods = kinoko::legacy::load<const unsigned char*>(object);
    return kinoko::legacy::load<Delete>(methods + 16)(object, 1);
}
