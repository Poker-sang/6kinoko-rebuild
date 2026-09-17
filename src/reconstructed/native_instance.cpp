#include "kinoko/squirrel_object.h"

extern "C" {
int32_t kinoko_native_void_type(void);
int32_t function_48aa20(int32_t);
int32_t function_48a670(int32_t);
int32_t function_48a480(int32_t, int32_t, int32_t);
int32_t function_48ce70(int32_t, int32_t);
int32_t function_48b490(int32_t, int32_t);
int32_t function_48c910(int32_t, int32_t);
int32_t function_48ab40(int32_t, int32_t, int32_t *);
int32_t function_48a400(int32_t, int32_t);
int32_t function_48a600(int32_t);
int32_t function_48ab90(int32_t, int32_t, int32_t);
int32_t function_48a4f0(int32_t, int32_t);
int32_t function_48a5c0(int32_t, int32_t);
int32_t function_48cb10(int32_t, int32_t);
int32_t function_48ce00(int32_t, int32_t);
int32_t function_48aa30(int32_t, int32_t);
int32_t function_48c7f0(int32_t, int32_t, int32_t);
int32_t function_48aa60(int32_t, int32_t);
int32_t function_48c840(int32_t, int32_t, int32_t);
int32_t function_48af30(int32_t, int32_t, int32_t);
}

namespace {
template<class T> int32_t address(T *pointer) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(pointer));
}

// SquirrelObject is an external VM reference, not an SQObjectPtr. Each
// temporary acquires its own external reference and releases it on scope exit.
class Object final {
public:
    Object(int32_t vm, int32_t vtable) : vm_(vm), value_{vtable, 0x01000001, 0} {}
    Object(const Object &) = delete;
    Object &operator=(const Object &) = delete;
    ~Object() { kinoko_squirrel_object_destroy(address(value_), vm_, value_[0]); }
    void capture(int32_t index) {
        function_48ab40(vm_, index, value_ + 1);
        function_48a400(vm_, address(value_ + 1));
    }
    void push() const { function_48ab90(vm_, value_[1], value_[2]); }
    int32_t size() { return kinoko_squirrel_object_size(address(value_), vm_); }
    // The caller pushes the key after this object's value. sq_get consumes it.
    void finish_get(Object &target) const {
        if (function_48ce00(vm_, -2) >= 0) {
            target.capture(-1);
            function_48aa30(vm_, 1);
        }
        function_48aa30(vm_, 1);
    }
    void set_pointer(int32_t key, int32_t pointer) const {
        const auto top = function_48aa20(vm_);
        push();
        function_48a4f0(vm_, key);
        function_48a5c0(vm_, pointer);
        function_48cb10(vm_, -3);
        function_48c910(vm_, top);
    }
private:
    int32_t vm_;
    int32_t value_[3];
};

// Original 4AB020: __ot maps ClassType identities to the native pointer.
// __ca's final element is intentionally excluded, matching the original loop.
void bind_types(int32_t vm, const Object &instance, int32_t pointer,
                int32_t vtable) {
    Object types(vm, vtable);
    function_48a600(vm);
    types.capture(-1);
    function_48aa30(vm, 1);
    types.set_pointer(kinoko_native_void_type(), pointer);

    const auto top = function_48aa20(vm);
    instance.push();
    function_48a480(vm, address("__ot"), -1);
    types.push();
    function_48cb10(vm, -3);
    function_48c910(vm, top);

    Object classes(vm, vtable);
    instance.push();
    function_48a480(vm, address("__ca"), -1);
    instance.finish_get(classes);
    const auto count = classes.size();
    for (int32_t index = 0; index < count - 1; ++index) {
        Object base(vm, vtable);
        classes.push();
        function_48a4f0(vm, index);
        classes.finish_get(base);
        base.push();
        int32_t type = 0;
        function_48c7f0(vm, -1, address(&type));
        types.set_pointer(type, pointer);
        function_48aa30(vm, 1);
    }
}
}

// Original 4AB170 creates an instance without invoking its script constructor.
// Failure restores the incoming stack; success leaves exactly the instance.
extern "C" int32_t kinoko_native_instance_create(
    int32_t vm, int32_t class_name, int32_t native_pointer,
    int32_t release_hook, int32_t object_vtable) {
    const auto top = function_48aa20(vm);
    function_48a670(vm);
    function_48a480(vm, class_name, -1);
    if (function_48ce70(vm, -2) < 0 || function_48b490(vm, -1) < 0) {
        function_48c910(vm, top);
        return 0;
    }
    Object instance(vm, object_vtable);
    instance.capture(-1);
    bind_types(vm, instance, native_pointer, object_vtable);
    function_48aa60(vm, -3);
    function_48aa60(vm, -2);
    if (function_48c840(vm, -1, native_pointer) < 0) {
        function_48c910(vm, top);
        return 0;
    }
    function_48af30(vm, -1, release_hook);
    return 1;
}
