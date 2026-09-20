#include "kinoko/upstream_bindings.hpp"
#include <sqplus.h>

// We use the existing source VM and its bootstrap, not the snapshot's VM owner,
// compiler or standard libraries. Context is borrowed and scoped per thread:
// explicit-VM host operations must not introduce a shared global VM race.
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SquirrelObject) == 12, "snapshot object agrees with recovered Win32 size");
#endif
thread_local HSQUIRRELVM SquirrelVM::_VM = nullptr;
HSQUIRRELVM SquirrelVM::ExchangeVMForHost(HSQUIRRELVM vm) {
    auto previous = _VM;
    _VM = vm;
    return previous;
}
namespace kinoko::script::upstream {
namespace {
class VmScope final {
public:
    explicit VmScope(HSQUIRRELVM vm) : previous_(SquirrelVM::ExchangeVMForHost(vm)) {}
    ~VmScope() { SquirrelVM::ExchangeVMForHost(previous_); }
    VmScope(const VmScope&) = delete;
    VmScope& operator=(const VmScope&) = delete;
private:
    HSQUIRRELVM previous_;
};
// Adopt an already-owned EXTERNAL reference into an actual stack object. The
// upstream public mutable handle lets us detach without changing the count.
class Adopted final : public SquirrelObject {
public:
    explicit Adopted(HSQOBJECT value) { GetObjectHandle() = value; }
    HSQOBJECT detach() {
        const HSQOBJECT value = GetObjectHandle();
        sq_resetobject(&GetObjectHandle());
        return value;
    }
};
}
HSQOBJECT sqplus_assign(HSQUIRRELVM vm, HSQOBJECT previous, HSQOBJECT incoming) {
    VmScope context(vm);
    Adopted target(previous);
    // Snapshot assignment increments incoming BEFORE releasing previous.
    target.SquirrelObject::operator=(incoming);
    return target.detach();
}
void sqplus_retain(HSQUIRRELVM vm, HSQOBJECT value) {
    VmScope context(vm);
    SquirrelObject retained(value);
    sq_resetobject(&retained.GetObjectHandle());
}
void sqplus_release(HSQUIRRELVM vm, HSQOBJECT value) {
    VmScope context(vm);
    Adopted target(value); // snapshot destructor -> Reset -> sq_release
}
bool sqplus_create_class(HSQUIRRELVM vm, HSQOBJECT& output, SQUserPointer tag,
                         const SQChar* name, const SQChar* parent) {
    VmScope context(vm);
    Adopted target(output);
    const bool created = SqPlus::CreateClass(vm, target, tag, name, parent) != 0;
    output = target.detach();
    return created;
}
} // namespace kinoko::script::upstream
