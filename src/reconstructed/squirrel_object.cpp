#include "kinoko/squirrel_object.h"

extern "C" {
int32_t function_4a9d70_this(int32_t object);
void _3f__3f_3_40_YAXPAX_40_Z(int32_t *object);
int32_t function_48ab90(int32_t vm, int32_t type, int32_t data);
int32_t function_48aa30(int32_t vm, int32_t count);
int32_t function_48c700(int32_t vm, int32_t index);
int32_t function_48c400(int32_t vm, int32_t index);
}

namespace {
struct SquirrelObject {
    uint32_t vtable;
    int32_t type;
    int32_t data;
};
static_assert(sizeof(SquirrelObject) == 12);
const SquirrelObject &wrapper(int32_t object) {
    return *reinterpret_cast<const SquirrelObject *>(static_cast<uintptr_t>(object));
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

extern "C" int32_t __fastcall kinoko_squirrel_object_delete(
    int32_t object, void *, int32_t flags) {
    function_4a9d70_this(object);
    if (flags & 1) {
        _3f__3f_3_40_YAXPAX_40_Z(
            reinterpret_cast<int32_t *>(static_cast<uintptr_t>(object)));
    }
    return object;
}
