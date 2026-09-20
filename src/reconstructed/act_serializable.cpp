#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/legacy_method_entries.h"
#include <cstring>

using kinoko::legacy::address;
using kinoko::legacy::field;
using kinoko::legacy::pointer;
extern "C" int32_t g350;

extern "C" const char* kinoko_act_serialized_type_name(int32_t object) {
    if (!object) return nullptr;
    const auto table=field<int32_t>(object);
    const auto* host=kinoko_act_host_symbols();
    // Recovered records are not modern C++ polymorphic objects. Their explicit
    // vtable identity supplies the original dynamic type at this ABI boundary;
    // never pass their bytes to the compiler's typeid or fake __RTtypeid.
    if (table==address(host->script_vtable)) return ".?AVCActScript@@";
    if (table==address(host->layer_vtable)) return ".?AVCActLayer@@";
    if (table==address(host->key_vtable)) return ".?AVCActKey@@";
    if (table==address(host->act_vtable)) return ".?AVCAct@@";
    if (table==address(host->layout_vtable)) return ".?AVC2DLayout@@";
    if (table==address(host->map_layout_vtable)) return ".?AVC2DMapLayout@@";
    if (table==address(host->texture_resource_vtable)) return ".?AVCActResource2D@@";
    if (table==address(host->chip_resource_vtable)) return ".?AVCActResourceChip@@";
    if (table==address(host->render_target_vtable)) return ".?AVCActRenderTarget@@";
    if (table==address(kinoko_act_timeline_vtable())) return ".?AVCActTimeLine@@";
    if (table==address(&g350)) return ".?AVCStringLayout@@";
    return nullptr;
}

extern "C" int32_t __fastcall kinoko_method_query_serializable(
    int32_t object,void*,int32_t type,int32_t output) {
    if (!output) return 0;
    const auto* name=kinoko_act_serialized_type_name(object);
    // 4461D0 checks the exact dynamic type (no base-class conversion).
    // Original type_info::operator== at 4AB2E2 compares descriptor+9,
    // intentionally ignoring the leading byte of the raw decorated name.
    const bool match=type && name && std::strcmp(name+1,pointer<const char>(type+9))==0;
    field<int32_t>(output)=match?object:0;
    return match;
}

extern "C" int32_t __fastcall kinoko_method_destroy_serializable(int32_t object,void*) {
    if (!object) return 0;
    // 446210: only classes with their deleting destructor at slot four use
    // this entry. C2DLayout and C2DMapLayout have a different slot arrangement.
    return retdec_call_thiscall1_result(pointer<void>(object),
        field<void*>(field<int32_t>(object)+16),1);
}
