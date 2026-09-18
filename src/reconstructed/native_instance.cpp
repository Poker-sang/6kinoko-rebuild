#include "kinoko/squirrel_object.h"
#include "kinoko/squirrel_host_object.hpp"

extern "C" int32_t kinoko_native_void_type(void);

namespace {
using namespace kinoko::script;

class ExternalObject final {
public:
    ExternalObject(HSQUIRRELVM vm, int32_t vtable) : vm_(vm) {
        view().initialize(vtable);
    }
    ExternalObject(const ExternalObject&) = delete;
    ExternalObject& operator=(const ExternalObject&) = delete;
    ~ExternalObject() {
        kinoko_squirrel_object_destroy(address(&storage_), address(vm_), storage_.vtable);
    }
    void capture(SQInteger index) { view().capture(vm_, index); }
    void push() const { sq_pushobject(vm_, storage_.value); }
    SQInteger size() { return kinoko_squirrel_object_size(address(&storage_), address(vm_)); }
    void finish_get(ExternalObject& target) const {
        if (SQ_SUCCEEDED(sq_get(vm_, -2))) {
            target.capture(-1);
            sq_pop(vm_, 1);
        }
        sq_pop(vm_, 1);
    }
    void set_pointer(SQInteger key, SQUserPointer value) const {
        StackTop stack(vm_);
        push();
        sq_pushinteger(vm_, key);
        sq_pushuserpointer(vm_, value);
        sq_rawset(vm_, -3);
    }
private:
    ObjectView view() { return ObjectView(&storage_); }
    HSQUIRRELVM vm_;
    ObjectStorage storage_{};
};

// Original 4AB020: __ot maps ClassType identities to the native pointer.
// __ca's final element is intentionally excluded, matching the original loop.
void bind_types(HSQUIRRELVM vm, const ExternalObject& instance,
                SQUserPointer native_pointer, int32_t vtable) {
    ExternalObject types(vm, vtable);
    sq_newtable(vm);
    types.capture(-1);
    sq_pop(vm, 1);
    types.set_pointer(kinoko_native_void_type(), native_pointer);
    {
        StackTop stack(vm);
        instance.push();
        sq_pushstring(vm, "__ot", -1);
        types.push();
        sq_rawset(vm, -3);
    }
    ExternalObject classes(vm, vtable);
    instance.push();
    sq_pushstring(vm, "__ca", -1);
    instance.finish_get(classes);
    const auto count = classes.size();
    for (SQInteger index = 0; index < count - 1; ++index) {
        ExternalObject base(vm, vtable);
        classes.push();
        sq_pushinteger(vm, index);
        classes.finish_get(base);
        base.push();
        SQUserPointer type = nullptr;
        sq_gettypetag(vm, -1, &type);
        types.set_pointer(address(type), native_pointer);
        sq_pop(vm, 1);
    }
}
} // namespace

// Original 4AB170 creates an instance WITHOUT executing its script constructor.
// Failure restores the incoming stack; success leaves exactly the instance.
extern "C" int32_t kinoko_native_instance_create(int32_t vm_address, int32_t class_name,
    int32_t native_pointer, int32_t release_hook, int32_t object_vtable) {
    auto* vm = pointer<SQVM>(vm_address);
    const auto top = sq_gettop(vm);
    sq_pushroottable(vm);
    sq_pushstring(vm, pointer<const char>(class_name), -1);
    if (SQ_FAILED(sq_rawget(vm, -2)) || SQ_FAILED(sq_createinstance(vm, -1))) {
        sq_settop(vm, top);
        return 0;
    }
    ExternalObject instance(vm, object_vtable);
    instance.capture(-1);
    bind_types(vm, instance, pointer(native_pointer), object_vtable);
    sq_remove(vm, -3);
    sq_remove(vm, -2);
    if (SQ_FAILED(sq_setinstanceup(vm, -1, pointer(native_pointer)))) {
        sq_settop(vm, top);
        return 0;
    }
    sq_setreleasehook(vm, -1, reinterpret_cast<SQRELEASEHOOK>(pointer(release_hook)));
    return 1;
}
