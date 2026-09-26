#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"

extern "C" struct SQVM *kinoko_primary_vm;
namespace {
using namespace kinoko::script;
inline SQVM*& current_vm_storage = kinoko_primary_vm;
HSQUIRRELVM current_vm() { return reinterpret_cast<HSQUIRRELVM>(current_vm_storage); }
void initialize(int32_t* object) { ObjectView(object).initialize(kinoko_squirrel_object_vtable()); }
}
extern "C" int32_t kinoko_squirrel_object_from_pair(int32_t* object, int32_t type, int32_t data) {
    auto vm = current_vm();
    if (!object || !vm) return 0;
    initialize(object);
    sq_pushobject(vm, borrowed_value(type, data));
    ObjectView(object).capture(vm, -1);
    sq_pop(vm, 1);
    return 1;
}
extern "C" int32_t kinoko_squirrel_object_string(int32_t* object, const char** output) {
    auto vm = current_vm();
    if (!object || !output || !vm) return 0;
    ObjectView(object).push(vm);
    const SQChar* value = nullptr;
    const auto status = sq_getstring(vm, -1, &value);
    sq_pop(vm, 1);
    if (SQ_FAILED(status)) return 0; // Failure leaves caller output untouched.
    *output = value;
    return value != nullptr;
}
extern "C" int32_t kinoko_squirrel_object_copy(int32_t* destination, const int32_t* source) {
    auto vm = current_vm();
    if (!destination || !source || !vm) return 0;
    if (destination == source) return 1;
    const auto value = ObjectView(const_cast<int32_t*>(source)).value();
    initialize(destination); // This helper constructs a new scratch wrapper.
    ObjectView(destination).assign(vm, value);
    return 1;
}
extern "C" int32_t kinoko_squirrel_object_from_string(int32_t* object, const char* data, uint32_t length) {
    auto vm = current_vm();
    if (!object || !data || !vm || length > 0x1000000u) return 0;
    // The old temporary was NUL-terminated and pushed with length -1. Preserve
    // embedded-NUL truncation without allocating/copying a second text buffer.
    const auto* nul = static_cast<const char*>(std::memchr(data, '\0', length));
    const auto count = nul ? static_cast<SQInteger>(nul - data) : static_cast<SQInteger>(length);
    initialize(object);
    sq_pushstring(vm, data, count);
    ObjectView(object).capture(vm, -1);
    sq_pop(vm, 1);
    return 1;
}
