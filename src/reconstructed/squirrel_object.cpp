#include "kinoko/squirrel_object.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include <cstdio>
#include <intrin.h>

extern "C" {
void kinoko_host_free_allocation(int32_t* object);
void kinoko_trace(const char* message);
void kinoko_trace_i32(const char* message, int32_t value);
}

using namespace kinoko::script;

// Original 4A96D0 deliberately accepts only array/table/string, not every
// type for which sq_getsize has a meaning. Operate on the existing game VM.
extern "C" int32_t kinoko_squirrel_object_size(int32_t object, int32_t vm_address) {
    if (!object || !vm_address) return 0;
    return upstream::sqplus_length(pointer<SQVM>(vm_address), ObjectView(object).value());
}

extern "C" int32_t kinoko_squirrel_object_reverse(int32_t object, int32_t vm_address) {
    return upstream::sqplus_reverse(pointer<SQVM>(vm_address), ObjectView(object).value());
}

extern "C" int32_t kinoko_squirrel_object_destroy(int32_t object, int32_t vm_address, int32_t vtable) {
    if (!object) return 0;
    ObjectView destination(object);
    const auto value = destination.value();
    kinoko_trace("4a9d70:begin");
    kinoko_trace_i32("4a9d70:this", object);
    kinoko_trace_i32("4a9d70:caller", address(_ReturnAddress()));
    kinoko_trace_i32("4a9d70:type", value._type);
    kinoko_trace_i32("4a9d70:data", data_bits(value));
    destination.set_vtable(vtable);
    if (vm_address) {
        destination.release(pointer<SQVM>(vm_address));
        kinoko_trace_i32("4a9d70:gvm-after-release", vm_address);
        destination.reset();
        kinoko_trace("4a9d70:after-release");
    } else {
        if (value._type != OT_NULL && data_bits(value))
            std::printf("SquirrelObject::~SquirrelObject - Cannot release\n");
        destination.reset();
        kinoko_trace("4a9d70:after-clear");
    }
    return destination.payload_address();
}

extern "C" int32_t __fastcall kinoko_squirrel_object_delete(int32_t object, void*, int32_t flags) {
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(object)));
    if (flags & 1) kinoko_host_free_allocation(pointer<int32_t>(object));
    return object;
}
