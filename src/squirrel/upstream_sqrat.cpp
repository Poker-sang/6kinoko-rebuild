#include "kinoko/upstream_bindings.hpp"
#include <sqrat/sqratTable.h>
#include <sqrat/sqratFunction.h>

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(Sqrat::Object) == 20, "Sqrat 0.8.1 agrees with recovered Win32 size");
#endif
namespace kinoko::script::upstream {
namespace {
class Adopted final : public Sqrat::Object {
public:
    Adopted(HSQUIRRELVM vm, HSQOBJECT value) : Object(vm, false) { obj = value; }
    Adopted(HSQUIRRELVM vm, HSQOBJECT value, bool owns) : Object(vm, owns) { obj = value; }
    SQRESULT bind(const SQChar* name, const void* payload, std::size_t size,
              SQFUNCTION function, bool static_slot) {
        return BindFunc(name, const_cast<void*>(payload), size, function, static_slot);
    }
    template<class T> SQRESULT bind_value(const SQChar* name, T& value, bool raw) {
        return BindValue<T&>(name, value, false, raw ? &raw_slot : &sq_newslot);
    }
    static SQRESULT raw_slot(HSQUIRRELVM vm, SQInteger index, SQBool) {
        return sq_rawset(vm, index);
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
void sqrat_execute(HSQUIRRELVM vm, HSQOBJECT environment, HSQOBJECT closure,
                   SQBool raiseerror,
                   SQRESULT (*invoke)(HSQUIRRELVM, SQInteger, SQBool, SQBool)) {
    // Construct the real source Function and borrow its handles without refcount
    // churn. Detach even if a native callback propagates a C++ exception.
    class BorrowedFunction final : public Sqrat::Function {
    public:
        BorrowedFunction(HSQUIRRELVM machine, HSQOBJECT env, HSQOBJECT function) {
            GetVM() = machine; GetEnv() = env; GetFunc() = function;
        }
        ~BorrowedFunction() { sq_resetobject(&GetEnv()); sq_resetobject(&GetFunc()); }
    } function(vm, environment, closure);
    function.ExecuteWithErrorHandling(raiseerror, invoke);
}
void sqrat_retain(HSQUIRRELVM vm, HSQOBJECT value) {
    Adopted::retain(vm, value);
}
void sqrat_release(HSQUIRRELVM vm, HSQOBJECT value) {
    Adopted adopted(vm, value);
    adopted.Release();
}
HSQOBJECT sqrat_object_value(HSQUIRRELVM vm, HSQOBJECT value) {
    const Adopted object(vm, value);
    return object.GetObject();
}
void sqrat_destroy_object(HSQUIRRELVM vm, HSQOBJECT value, bool owns) {
    Adopted object(vm, value, owns); // actual ~Sqrat::Object owns the release
}
bool sqrat_bind_value(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* name,
                      HSQOBJECT incoming, bool raw) {
    Adopted object(vm, receiver), value(vm, incoming);
    Sqrat::Object& base = value;
    return SQ_SUCCEEDED(object.bind_value(name, base, raw));
}
bool sqrat_bind_string(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* name,
                       const SQChar* text, bool raw) {
    Adopted object(vm, receiver);
    return SQ_SUCCEEDED(object.bind_value(name, text, raw));
}
bool sqrat_bind_function(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* name,
                          const void* payload, std::size_t size,
                          SQFUNCTION function, bool static_slot) {
    Adopted object(vm, receiver);
    return SQ_SUCCEEDED(object.bind(name, payload, size, function, static_slot));
}
} // namespace kinoko::script::upstream
