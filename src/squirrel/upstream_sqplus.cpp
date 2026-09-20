#include "kinoko/upstream_bindings.hpp"
#include <sqplus.h>
#include "kinoko/sqplus_source_entries.hpp"
#include "kinoko/squirrel_variable_record.hpp"

// We use the existing source VM and its bootstrap, not the snapshot's VM owner,
// compiler or standard libraries. Context is borrowed and scoped per thread:
// explicit-VM host operations must not introduce a shared global VM race.
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SquirrelObject) == 12, "snapshot object agrees with recovered Win32 size");
#endif
#if defined(_MSC_VER) && defined(_M_IX86)
// Matching layout is necessary but not sufficient to call the variable switch:
// its native descriptor, string and error policies still need host adaptation.
using HostVariable = kinoko::script::binding::Variable;
static_assert(sizeof(SqPlus::VarRef) == sizeof(HostVariable));
static_assert(offsetof(SqPlus::VarRef, offsetOrAddrOrConst) == offsetof(HostVariable, offset));
static_assert(offsetof(SqPlus::VarRef, m_type) == offsetof(HostVariable, category));
static_assert(offsetof(SqPlus::VarRef, instanceType) == offsetof(HostVariable, instance_type));
static_assert(offsetof(SqPlus::VarRef, varType) == offsetof(HostVariable, value_type));
static_assert(offsetof(SqPlus::VarRef, m_size) == offsetof(HostVariable, size));
static_assert(offsetof(SqPlus::VarRef, m_access) == offsetof(HostVariable, flags));
static_assert(SqPlus::VAR_ACCESS_READ_ONLY == kinoko::script::binding::ReadOnly);
static_assert(SqPlus::VAR_ACCESS_CONSTANT == kinoko::script::binding::Constant);
static_assert(SqPlus::VAR_ACCESS_STATIC == kinoko::script::binding::Static);
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
// Reading through a real upstream object must not acquire or release the
// caller's reference. Detach before the base destructor calls Reset().
class Borrowed final : public SquirrelObject {
public:
    explicit Borrowed(HSQOBJECT value) { GetObjectHandle() = value; }
    ~Borrowed() { sq_resetobject(&GetObjectHandle()); }
    Borrowed(const Borrowed&) = delete;
    Borrowed& operator=(const Borrowed&) = delete;
};
HSQOBJECT take(SquirrelObject& object) {
    const auto result = object.GetObjectHandle();
    sq_resetobject(&object.GetObjectHandle());
    return result;
}
}
std::array<char, 258> sqplus_variable_key(const SQChar* name) noexcept {
    std::array<char, 258> key{};
    SqPlus::getVarNameTag(key.data(), static_cast<INT>(key.size()), name ? name : "");
    return key;
}
bool sqplus_new_instance(HSQUIRRELVM vm, HSQOBJECT klass, HSQOBJECT& output) {
    VmScope context(vm); Borrowed source(klass);
    const auto top = sq_gettop(vm);
    try {
        auto result = SquirrelVM::CreateInstance(source);
        output = take(result);
        return true;
    } catch (const SquirrelError&) {
        // The source error constructor pushes the last-error object. The C ABI
        // reports failure via a null wrapper, retaining the VM's original error.
        sq_settop(vm, top);
        return false;
    }
}
HSQOBJECT sqplus_new_table(HSQUIRRELVM vm) {
    VmScope context(vm);
    auto result = SquirrelVM::CreateTable();
    return take(result);
}
HSQOBJECT sqplus_new_array(HSQUIRRELVM vm, int size) {
    VmScope context(vm);
    auto result = SquirrelVM::CreateArray(size);
    return take(result);
}
bool sqplus_set_string(HSQUIRRELVM vm, HSQOBJECT receiver, int key, const SQChar* value) {
    VmScope context(vm); Borrowed object(receiver);
    return object.SetValue(key, value) != 0;
}
bool sqplus_new_userdata(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key,
                          int size, SQUserPointer tag) {
    VmScope context(vm); Borrowed object(receiver);
    // Snapshot NewUserData passes this pointer value to sq_settypetag; it does
    // not dereference it despite its historical pointer-to-pointer signature.
    return object.NewUserData(key, size, static_cast<SQUserPointer*>(tag)) != 0;
}
bool sqplus_get_userdata(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key,
                          SQUserPointer* data, SQUserPointer* tag, bool raw) {
    VmScope context(vm); Borrowed object(receiver);
    return (raw ? object.RawGetUserData(key, data, tag) :
                  object.GetUserData(key, data, tag)) != 0;
}
bool sqplus_get_typetag(HSQUIRRELVM vm, HSQOBJECT receiver, SQUserPointer* tag) {
    VmScope context(vm); Borrowed object(receiver);
    return object.GetTypeTag(tag) != 0;
}
bool sqplus_set_delegate(HSQUIRRELVM vm, HSQOBJECT receiver, HSQOBJECT delegate) {
    VmScope context(vm); Borrowed object(receiver), incoming(delegate);
    return object.SetDelegate(incoming) != 0;
}
HSQOBJECT sqplus_new_string(HSQUIRRELVM vm, const SQChar* text) {
    VmScope context(vm);
    auto result = SquirrelVM::CreateString(text);
    return take(result);
}
HSQOBJECT sqplus_new_closure(HSQUIRRELVM vm, SQFUNCTION native) {
    VmScope context(vm);
    auto result = SquirrelVM::CreateFunction(native);
    return take(result);
}
HSQOBJECT sqplus_assign(HSQUIRRELVM vm, HSQOBJECT previous, HSQOBJECT incoming) {
    VmScope context(vm);
    Adopted target(previous);
    // Snapshot assignment increments incoming BEFORE releasing previous.
    target.SquirrelObject::operator=(incoming);
    return target.detach();
}
HSQOBJECT sqplus_capture(HSQUIRRELVM vm, HSQOBJECT previous, int index) {
    VmScope context(vm);
    Adopted target(previous);
    target.AttachToStackObject(index);
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
bool sqplus_instance_base(HSQUIRRELVM vm, HSQOBJECT receiver,
                          SQUserPointer declaring_type, SQUserPointer& result) {
    result = nullptr;
    if (!vm || receiver._type != OT_INSTANCE) return false;
    VmScope context(vm);
    Borrowed instance(receiver);
    SqPlus::VarRef metadata;
    metadata.instanceType = static_cast<SqPlus::ClassTypeBase*>(declaring_type);
    metadata.varType = nullptr;
    try {
        result = SqPlus::ReadInstanceBaseForHost(instance, metadata);
        return result != nullptr;
    } catch (const SquirrelError& error) {
        // Native source exceptions cannot escape the recovered C callback ABI.
        // Preserve the source text while returning the VM's error status.
        sq_throwerror(vm, error.desc);
        return false;
    }
}
int sqplus_length(HSQUIRRELVM vm, HSQOBJECT receiver) {
    VmScope context(vm); Borrowed object(receiver);
    return object.Len();
}
bool sqplus_reverse(HSQUIRRELVM vm, HSQOBJECT receiver) {
    VmScope context(vm); Borrowed object(receiver);
    return object.ArrayReverse() != 0;
}
void sqplus_append(HSQUIRRELVM vm, HSQOBJECT receiver, HSQOBJECT value) {
    VmScope context(vm); Borrowed object(receiver), item(value);
    object.ArrayAppend(static_cast<const SquirrelObject&>(item));
}
bool sqplus_raw_set(HSQUIRRELVM vm, HSQOBJECT receiver, HSQOBJECT key, HSQOBJECT value) {
    VmScope context(vm); Borrowed object(receiver), index(key), item(value);
    return object.SetValue(index, item) != 0;
}
bool sqplus_raw_set(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key, HSQOBJECT value) {
    VmScope context(vm); Borrowed object(receiver), item(value);
    return object.SetValue(key, item) != 0;
}
int sqplus_get_integer(HSQUIRRELVM vm, HSQOBJECT receiver, int key) {
    VmScope context(vm); Borrowed object(receiver);
    return object.GetInt(key);
}
const SQChar* sqplus_get_string(HSQUIRRELVM vm, HSQOBJECT receiver, int key) {
    VmScope context(vm); Borrowed object(receiver);
    return object.GetString(key);
}
SQUserPointer sqplus_get_userpointer(HSQUIRRELVM vm, HSQOBJECT receiver, int key) {
    VmScope context(vm); Borrowed object(receiver);
    return object.GetUserPointer(key);
}
SQUserPointer sqplus_get_instance_up(HSQUIRRELVM vm, HSQOBJECT receiver, SQUserPointer tag) {
    VmScope context(vm); Borrowed object(receiver);
    return object.GetInstanceUP(tag);
}
bool sqplus_set_instance_up(HSQUIRRELVM vm, HSQOBJECT receiver, SQUserPointer value) {
    VmScope context(vm); Borrowed object(receiver);
    return object.SetInstanceUP(value) != 0;
}
bool sqplus_begin_iteration(HSQUIRRELVM vm, HSQOBJECT receiver) {
    VmScope context(vm); Borrowed object(receiver);
    // Deliberately leaves the receiver and iterator on the caller's VM stack.
    return object.BeginIteration() != 0;
}
bool sqplus_exists(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key) {
    VmScope context(vm); Borrowed object(receiver);
    return object.Exists(key) != 0;
}
HSQOBJECT sqplus_get_delegate(HSQUIRRELVM vm, HSQOBJECT receiver) {
    VmScope context(vm); Borrowed object(receiver);
    auto result = object.GetDelegate();
    return take(result);
}
HSQOBJECT sqplus_get_value(HSQUIRRELVM vm, HSQOBJECT receiver, const SQChar* key) {
    VmScope context(vm); Borrowed object(receiver);
    auto result = object.GetValue(key);
    return take(result);
}
} // namespace kinoko::script::upstream
