#include "kinoko/upstream_bindings.hpp"
#include <sqrat/sqratTable.h>

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(Sqrat::Object) == 20, "Sqrat 0.8.1 agrees with recovered Win32 size");
#endif
namespace kinoko::script::upstream {
namespace {
class Adopted final : public Sqrat::Object {
public:
    Adopted(HSQUIRRELVM vm, HSQOBJECT value) : Object(vm, false) { obj = value; }
    void bind(const SQChar* name, const void* payload, std::size_t size,
              SQFUNCTION function, bool static_slot) {
        BindFunc(name, const_cast<void*>(payload), size, function, static_slot);
    }
    static void retain(HSQUIRRELVM vm, HSQOBJECT value) {
        Adopted acquired(vm, value, Retain{});
    }
private:
    struct Retain {};
    Adopted(HSQUIRRELVM vm, HSQOBJECT value, Retain) : Object(value, vm) { release = false; }
};
// Construct real upstream objects. Take ownership without a compensating
// addref/release pair; no virtual object is installed into legacy byte storage.
template<class Base> class Detached final : public Base {
public:
    explicit Detached(HSQUIRRELVM vm) : Base(vm) {}
    HSQOBJECT take() { this->release = false; return this->obj; }
};
}
bool sqrat_get(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key, HSQOBJECT& output) {
    Adopted object(vm, receiver); // borrowed, not another reference to the host
    bool found = false;
    auto result = object.GetSlot(key, &found);
    output = result.GetObject();
    sq_resetobject(&result.GetObject()); // transfer the owned external reference
    return found;
}
HSQOBJECT sqrat_root(HSQUIRRELVM vm) { Detached<Sqrat::RootTable> root(vm); return root.take(); }
HSQOBJECT sqrat_table(HSQUIRRELVM vm) { Detached<Sqrat::Table> table(vm); return table.take(); }
void sqrat_retain(HSQUIRRELVM vm, HSQOBJECT value) {
    Adopted::retain(vm, value);
}
void sqrat_release(HSQUIRRELVM vm, HSQOBJECT value) {
    Adopted adopted(vm, value);
    adopted.Release();
}
void sqrat_bind_function(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* name,
                          const void* payload, std::size_t size,
                          SQFUNCTION function, bool static_slot) {
    Adopted object(vm, receiver);
    object.bind(name, payload, size, function, static_slot);
}
} // namespace kinoko::script::upstream
