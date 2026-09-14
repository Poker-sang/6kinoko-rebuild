#include "kinoko/squirrel_object.h"
#include <cstdio>
#include <intrin.h>

extern "C" {
int32_t function_4a9d70_this(int32_t object);
void _3f__3f_3_40_YAXPAX_40_Z(int32_t *object);
int32_t function_48ab90(int32_t vm, int32_t type, int32_t data);
int32_t function_48aa30(int32_t vm, int32_t count);
int32_t function_48c700(int32_t vm, int32_t index);
int32_t function_48c400(int32_t vm, int32_t index);
int32_t function_48a430(int32_t vm, int32_t value);
int32_t function_48abe0(int32_t value);
void retdec_trace(const char *message);
void retdec_trace_i32(const char *message, int32_t value);
}

namespace {
struct SquirrelObject {
    uint32_t vtable;
    int32_t type;
    int32_t data;
};
static_assert(sizeof(SquirrelObject) == 12);
SquirrelObject &wrapper(int32_t object) {
    return *reinterpret_cast<SquirrelObject *>(static_cast<uintptr_t>(object));
}
}

// Original 4A96D0/4A99F0 push the wrapped value onto the existing game VM,
// perform the API operation, then pop that value. No separate VM is created.
extern "C" int32_t kinoko_squirrel_object_size(int32_t object, int32_t vm) {
    if (!object || !vm) return 0;
    const auto &value = wrapper(object);
    if (value.type != 0x08000040 && value.type != 0x0a000020 &&
        value.type != 0x08000010) return 0;
    function_48ab90(vm, value.type, value.data);
    const auto size = function_48c700(vm, -1);
    function_48aa30(vm, 1);
    return size;
}

extern "C" int32_t kinoko_squirrel_object_clear(int32_t object, int32_t vm) {
    const auto &value = wrapper(object);
    function_48ab90(vm, value.type, value.data);
    const auto result = function_48c400(vm, -1);
    function_48aa30(vm, 1);
    return result == 0;
}

extern "C" int32_t kinoko_squirrel_object_destroy(int32_t object, int32_t vm, int32_t vtable) {
    if (!object) return 0;
    auto &value = wrapper(object);
    const int32_t pair = object + sizeof(value.vtable);
    retdec_trace("4a9d70:begin");
    retdec_trace_i32("4a9d70:this", object);
    retdec_trace_i32("4a9d70:caller", static_cast<int32_t>(reinterpret_cast<uintptr_t>(_ReturnAddress())));
    retdec_trace_i32("4a9d70:type", value.type);
    retdec_trace_i32("4a9d70:data", value.data);
    value.vtable = vtable;
    // SquirrelObject owns a reference in the VM's external-reference table.
    // It must use sq_release before sq_resetobject, not SQObjectPtr::~SQObjectPtr.
    if (vm) {
        function_48a430(vm, pair);
        retdec_trace_i32("4a9d70:gvm-after-release", vm);
        const auto result = function_48abe0(pair);
        retdec_trace("4a9d70:after-release");
        return result;
    }
    if (value.type != 0x01000001 && value.data)
        std::printf("SquirrelObject::~SquirrelObject - Cannot release\n");
    const auto result = function_48abe0(pair);
    retdec_trace("4a9d70:after-clear");
    return result;
}

extern "C" int32_t __fastcall kinoko_squirrel_object_delete(
    int32_t object, void *, int32_t flags) {
    function_4a9d70_this(object);
    if (flags & 1) {
        _3f__3f_3_40_YAXPAX_40_Z(
            reinterpret_cast<int32_t *>(static_cast<uintptr_t>(object)));
    }
    return object;
}
