#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_source_runtime.h"
#include "sqpcheader.h"
#include "sqvm.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>

using namespace kinoko::script;
using namespace kinoko::script::binding;

namespace {
ObjectStorage root_storage{};
int32_t __fastcall type_name(void* self, void*) {
    return load<int32_t>(static_cast<unsigned char*>(self) + 8);
}
int32_t type_vtable[2]{};
int32_t descriptors[4][6]{};
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
}

// Only the narrow embedding services are supplied by this test. All stack,
// objects, classes, bytecode, errors, references and callbacks use the real
// vendored Squirrel 2.2.2 implementation, not a simulated VM.
extern "C" {
char* g644 = nullptr;
int32_t kinoko_squirrel_object_vtable(void) { return 0x12345678; }
int32_t kinoko_native_void_type(void) { return 0x13572468; }
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
void _3f__3f_3_40_YAXPAX_40_Z(int32_t* p) { std::free(p); }
int32_t function_4a8cc0(void) { return address(&root_storage); }
int32_t function_4a8db0(int32_t vm) { g644 = pointer<char>(vm); return 1; }
int32_t* kinoko_native_binding_type(int32_t category) {
    const int index = category == -1 ? 0 : category == 0 ? 1 : category == 2 ? 2 : 3;
    type_vtable[1] = address(reinterpret_cast<void*>(&type_name));
    auto* descriptor = descriptors[index];
    descriptor[0] = address(type_vtable);
    const char* names[] = {nullptr, "int", "float", "bool"};
    descriptor[2] = address(names[index]);
    descriptor[4] = -1;
    descriptor[5] = 1;
    return descriptor;
}
}

namespace {
int32_t exchange_vm(int32_t vm) {
    const auto old = address(g644); g644 = pointer<char>(vm); return old;
}
class Machine final {
public:
    Machine() : vm_(pointer<SQVM>(kinoko_sq_open(64))) {
        require(vm_ != nullptr, "open source VM");
        g644 = reinterpret_cast<char*>(vm_);
        kinoko_sq_set_context_exchange(exchange_vm);
        ObjectView(&root_storage).initialize(kinoko_squirrel_object_vtable());
        sq_pushroottable(vm_);
        ObjectView(&root_storage).capture(vm_, -1); sq_pop(vm_, 1);
    }
    ~Machine() {
        ObjectView(&root_storage).release(vm_);
        ObjectView(&root_storage).reset();
        kinoko_sq_set_context_exchange(nullptr); sq_close(vm_); g644 = nullptr;
    }
    HSQUIRRELVM get() { return vm_; }
private:
    HSQUIRRELVM vm_;
};
SQInteger integer(HSQUIRRELVM vm, int index = -1) {
    SQInteger result = 0;
    require(SQ_SUCCEEDED(sq_getinteger(vm, index, &result)), "integer result");
    return result;
}
SQFloat numeric(HSQUIRRELVM vm, int index = -1) {
    SQFloat result = 0;
    require(SQ_SUCCEEDED(sq_getfloat(vm, index, &result)), "float result");
    return result;
}
std::string text(HSQUIRRELVM vm, int index = -1) {
    const char* result = nullptr;
    require(SQ_SUCCEEDED(sq_getstring(vm, index, &result)), "string result");
    return result;
}
std::string last_error(HSQUIRRELVM vm) {
    StackTop stack(vm); sq_getlasterror(vm); return text(vm);
}
void run_script(HSQUIRRELVM vm, const char* code) {
    StackTop stack(vm);
    if (SQ_FAILED(sq_compilebuffer(vm, code, std::strlen(code), "binding-contract", SQTrue)))
        throw std::runtime_error("compile real bytecode: " + last_error(vm));
    sq_pushroottable(vm);
    if (SQ_FAILED(kinoko_sq_call(address(vm), 1, 0, 0))) {
        throw std::runtime_error("execute real bytecode: " + last_error(vm));
    }
}
void make_class(HSQUIRRELVM vm, Object& output, const char* name, const char* parent = nullptr) {
    const auto top = sq_gettop(vm);
    require(function_4607e0(pointer<int32_t>(output.location()), address(vm), address(name), address(parent)) == output.location(), "class returns output");
    require(output.view().value()._type == OT_CLASS, "class created");
    require(sq_gettop(vm) == top, "class creation restores stack");
}
void make_instance(HSQUIRRELVM vm, Object& klass, Object& output, void* native) {
    StackTop stack(vm);
    klass.view().push(vm);
    require(SQ_SUCCEEDED(sq_createinstance(vm, -1)), "create instance without constructor");
    require(SQ_SUCCEEDED(sq_setinstanceup(vm, -1, native)), "set native receiver");
    output.view().capture(vm, -1);
}
void install_method(HSQUIRRELVM vm, Object& klass, const char* name, void* method, void* wrapper) {
    const auto top = sq_gettop(vm);
    function_460e00_register_actor_method(address(vm), pointer<int32_t>(klass.location()), name,
        address(method), address(wrapper), 0);
    require(sq_gettop(vm) == top, "method registration preserves stack");
}
void publish(HSQUIRRELVM vm, const char* name, Object& value) {
    StackTop stack(vm); sq_pushroottable(vm); sq_pushstring(vm, name, -1);
    value.view().push(vm); require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "publish test object");
}
SQInteger noop(HSQUIRRELVM) { return 0; }

void class_contract(HSQUIRRELVM vm) {
    const auto top = sq_gettop(vm);
    Object base(vm), derived(vm), failed(vm), table(vm), closure(vm);
    make_class(vm, base, "BindingBase");
    make_class(vm, derived, "BindingChild", "BindingBase");
    Object base_map(vm), child_map(vm), base_ca(vm), child_ca(vm);
    require(get_slot(vm, base.view(), "__ot", base_map.view()), "base native map");
    require(get_slot(vm, derived.view(), "__ot", child_map.view()), "inherited native map");
    require(data_bits(base_map.view().value()) == data_bits(child_map.view().value()), "reuse inherited __ot");
    require(get_slot(vm, base.view(), "__ca", base_ca.view()), "base ancestor array");
    require(get_slot(vm, derived.view(), "__ca", child_ca.view()), "child ancestor array");
    require(data_bits(base_ca.view().value()) == data_bits(child_ca.view().value()), "reuse inherited __ca");
    base_ca.view().push(vm); require(sq_getsize(vm, -1) == 2, "append base and child once"); sq_pop(vm, 1);
    require(function_4aa540(address(vm), failed.location(), kinoko_native_binding_type(-1),
        address("MissingParentChild"), address("MissingParent")) == 0, "missing base fails");
    require(failed.view().value()._type == OT_NULL && sq_gettop(vm) == top, "failed class leaves null output and stack");

    std::array<unsigned char, 50> state{}; state.fill(0xA5);
    require(function_460d10_this(address(state.data() + 1), "UnalignedBinding", 0) == address(state.data() + 1), "unaligned class binding result");
    require(state.front() == 0xA5 && state.back() == 0xA5, "48-byte binding canaries");
    require(load<int32_t>(state.data() + 1) == address(vm), "binding vm layout");
    for (int offset : {8, 24, 36}) ObjectView(state.data() + 1 + offset).release(vm);
    require(function_460d10_actor(pointer<int32_t>(failed.location()), "ActorBinding", 0) == pointer<int32_t>(failed.location()), "actor class wrapper");
    require(failed.view().value()._type == OT_CLASS, "actor binding assignment");

    new_table(vm, table.view());
    function_4a9490(pointer<int32_t>(closure.location()), table.location(), address(reinterpret_cast<void*>(&noop)),
        const_cast<char*>("default"), nullptr);
    require(closure.view().value()._type == OT_NATIVECLOSURE, "capture created closure");
    closure.view().push(vm); table.view().push(vm);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 1, 0, 0)), "default receiver mask accepts table"); sq_pop(vm, 1);
    Object wildcard(vm), typed(vm), overflow(vm);
    function_4a9490(pointer<int32_t>(wildcard.location()), table.location(), address(reinterpret_cast<void*>(&noop)),
        const_cast<char*>("wild"), const_cast<char*>("*"));
    wildcard.view().push(vm); sq_pushinteger(vm, 1); sq_pushinteger(vm, 2);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 2, 0, 0)), "wildcard omits all parameter checks"); sq_pop(vm, 1);
    function_4a9490(pointer<int32_t>(typed.location()), table.location(), address(reinterpret_cast<void*>(&noop)),
        const_cast<char*>("typed"), const_cast<char*>("i"));
    typed.view().push(vm); table.view().push(vm); sq_pushinteger(vm, 7);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 2, 0, 0)), "typed closure accepts integer"); sq_pop(vm, 1);
    { StackTop call(vm); typed.view().push(vm); table.view().push(vm); sq_pushfloat(vm, 7);
      require(SQ_FAILED(kinoko_sq_call(address(vm), 2, 0, 0)), "typed closure rejects float"); }
    const std::string longmask(100, 'i');
    function_4a9490(pointer<int32_t>(overflow.location()), table.location(), address(reinterpret_cast<void*>(&noop)),
        const_cast<char*>("overflow"), const_cast<char*>(longmask.c_str()));
    overflow.view().push(vm); table.view().push(vm);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 1, 0, 0)), "oversized mask preserves receiver-only fallback"); sq_pop(vm, 1);
    Object string(vm);
    require(function_4a9250(string.location(), address("bound string")) == string.location(), "string object return");
    string.view().push(vm); require(text(vm) == "bound string", "string object value"); sq_pop(vm, 1);
    require(sq_gettop(vm) == top, "all class helpers balanced");
}

void lookup_contract(HSQUIRRELVM vm) {
    StackTop stack(vm);
    Object table(vm); new_table(vm, table.view());
    const std::string longname(300, 'n');
    const auto data = function_45fab0(table.location(), address(longname.c_str()));
    require(data != 0, "allocate variable metadata");
    require(function_45fab0(table.location(), address(longname.c_str())) == data, "reuse metadata identity");
    require(function_45fab0(table.location(), address(longname.substr(0, 255).c_str())) == data, "prefix truncates at 255 characters");
    const Variable info{17, 0, 0, 0, 4, Constant}; store(data, info);
    table.view().push(vm); sq_pushstring(vm, longname.c_str(), -1);
    int32_t context[2] = {static_cast<int32_t>(sq_gettop(vm)), address(vm)};
    // Use indices 1/2 exactly as a real native frame.
    require(context[0] == 2, "isolated variable test frame");
    int32_t found = 0;
    require(retdec_get_var_info(&found, context) == 0 && found == data, "source table metadata lookup");
    require(function_4aa5e0(context, &found) == 0 && found == data, "source raw metadata lookup");
    sq_pop(vm, 1); sq_pushstring(vm, "absent", -1);
    sq_throwerror(vm, "sentinel");
    require(retdec_get_var_info(&found, context) == -1 && found == 0, "missing table metadata");
    require(last_error(vm) == "sentinel", "table lookup does not overwrite last error");
    found = 123;
    require(function_4aa5e0(context, &found) == -1 && found == 123, "raw lookup preserves failed output");
    require(last_error(vm) == "getVarInfo: Could not retrieve UserData", "raw lookup installs original error");
    sq_settop(vm, 0);
    table.view().push(vm); sq_pushstring(vm, "_vshort", -1); sq_newuserdata(vm, 19);
    require(SQ_SUCCEEDED(sq_rawset(vm, -3)), "install short metadata"); sq_pop(vm, 1);
    require(function_45fab0(table.location(), address("short")) == 0, "reject undersized existing metadata");
    table.view().push(vm); sq_pushstring(vm, "_vwrong", -1); sq_pushinteger(vm, 55);
    require(SQ_SUCCEEDED(sq_rawset(vm, -3)), "install wrong-type metadata"); sq_pop(vm, 1);
    require(function_45fab0(table.location(), address("wrong")) == 0, "never replace wrong-type metadata");
    for (const char* key : {"short", "wrong"}) {
        table.view().push(vm); sq_pushstring(vm, key, -1);
        context[0] = 2;
        require(retdec_get_var_info(&found, context) == -1, "safe table malformed metadata");
        require(function_4aa5e0(context, &found) == -1, "safe raw malformed metadata");
        sq_settop(vm, 0);
    }
    require(retdec_get_var_info(&found, nullptr) == -1, "null metadata context");
}

void values_contract(HSQUIRRELVM vm) {
    StackTop stack(vm);
    std::array<unsigned char, 24> metadata{}; metadata.fill(0x6B);
    std::array<unsigned char, 10> bytes{}; bytes.fill(0xA5);
    const auto info_addr = address(metadata.data() + 1), value_addr = address(bytes.data() + 1);
    int32_t context[2] = {3, address(vm)};
    for (int size : {1, 2, 4}) {
        const Variable info{0, 0, 0, 0, static_cast<uint16_t>(size), 0}; store(info_addr, info);
        for (int32_t input : {-32769, -129, -1, 0, 127, 128, 32767, 65535, 65536}) {
            bytes.fill(0xA5); sq_settop(vm, 0); sq_pushnull(vm); sq_pushnull(vm); sq_pushinteger(vm, input);
            require(retdec_set_var_value(context, info_addr, value_addr) == 1, "integer setter returns one");
            require(integer(vm) == input, "setter returns full value before narrowing");
            sq_pop(vm, 1);
            require(retdec_get_var_value(context, info_addr, value_addr) == 1, "integer getter returns one");
            const int32_t expected = size == 1 ? static_cast<int8_t>(input) : size == 2 ? static_cast<int16_t>(input) : input;
            require(integer(vm) == expected, "signed narrow integer round trip");
            require(bytes.front() == 0xA5 && bytes[size + 1] == 0xA5, "unaligned narrow store canaries");
        }
    }
    Variable info{0, 0, 0, 0, 4, 0}; store(info_addr, info);
    sq_settop(vm, 0); sq_pushnull(vm); sq_pushnull(vm); sq_pushstring(vm, "not a number", -1);
    require(retdec_set_var_value(context, info_addr, value_addr) == 1 && load<int32_t>(value_addr) == 0, "legacy invalid integer conversion writes zero");
    info.category = 2; store(info_addr, info);
    store(value_addr, 42.5f); sq_pop(vm, 1);
    require(retdec_set_var_value(context, info_addr, value_addr) == -1 && load<float>(value_addr) == 42.5f, "invalid float conversion leaves destination unchanged");
    sq_settop(vm, 2); sq_pushinteger(vm, 17);
    require(retdec_set_var_value(context, info_addr, value_addr) == 1 && numeric(vm) == 17, "float property accepts integer conversion");
    info.flags = Constant; store(info_addr, info);
    require(retdec_get_var_value(context, info_addr, -7) == 1 && numeric(vm) == -7, "constant float is integer conversion not bit reinterpretation");
    for (int flag : {ReadOnly, Constant}) {
        info.flags = static_cast<uint16_t>(flag); store(info_addr, info);
        require(retdec_set_var_value(context, info_addr, value_addr) == -1, "immutable metadata blocks stores");
    }
    info = {0, 3, 0, 0, 1, 0}; store(info_addr, info);
    sq_settop(vm, 2); sq_pushinteger(vm, 1);
    require(retdec_set_var_value(context, info_addr, value_addr) == 1 && load<uint8_t>(value_addr) == 0, "boolean setter rejects truthy integer conversion");
    sq_settop(vm, 2); sq_pushbool(vm, SQTrue);
    require(retdec_set_var_value(context, info_addr, value_addr) == 1 && load<uint8_t>(value_addr) == 1, "boolean true round trip");
    const char* string_pointer = "pointer string";
    info = {0, 4, 0, 0, 4, 0}; store(info_addr, info);
    require(retdec_get_var_value(context, info_addr, address(&string_pointer)) == 1 && text(vm) == string_pointer, "pointer string getter");
    info.flags = Constant; store(info_addr, info);
    require(retdec_get_var_value(context, info_addr, address("constant")) == 1 && text(vm) == "constant", "constant string getter");
    info.category = 5; info.flags = 0; store(info_addr, info);
    require(retdec_get_var_value(context, info_addr, address("!buffer")) == 1 && text(vm) == "buffer", "buffer skips first byte");
    std::array<unsigned char, 24> old_string{};
    std::memcpy(old_string.data(), "inline", 7); store(old_string.data() + 20, uint32_t{15});
    info.category = 8; store(info_addr, info);
    require(retdec_get_var_value(context, info_addr, address(old_string.data())) == 1 && text(vm) == "inline", "old MSVC small string layout");
    store(old_string.data(), address("heap string")); store(old_string.data() + 20, uint32_t{16});
    require(retdec_get_var_value(context, info_addr, address(old_string.data())) == 1 && text(vm) == "heap string", "old MSVC heap string layout");
    require(metadata.front() == 0x6B && metadata[21] == 0x6B, "20-byte metadata canaries");
}

struct Native {
    int32_t guard = 0x1234;
    int32_t value = 11;
    float x = 2.5f;
    uint8_t enabled = 1;
    int calls = 0;
    std::array<int32_t, 4> arguments{};
};
void instance_contract(HSQUIRRELVM vm) {
    StackTop stack(vm);
    Object klass(vm), instance(vm);
    make_class(vm, klass, "PropertyActor");
    auto* type = kinoko_native_binding_type(-1);
    function_460920(pointer<int32_t>(klass.location()), type, offsetof(Native, value), const_cast<char*>("value"), 0);
    function_4609c0(pointer<int32_t>(klass.location()), type, offsetof(Native, x), const_cast<char*>("x"), 0);
    function_460a60(pointer<int32_t>(klass.location()), type, offsetof(Native, enabled), const_cast<char*>("enabled"), 0);
    Native native;
    make_instance(vm, klass, instance, &native); publish(vm, "propertyActor", instance);
    run_script(vm, R"nut(
propertyActor.value = 123;
propertyActor.x = 7.25;
propertyActor.enabled = false;
if(propertyActor.value != 123 || propertyActor.x != 7.25 || propertyActor.enabled != false) throw "property roundtrip";
)nut");
    require(native.value == 123 && native.x == 7.25f && native.enabled == 0 && native.guard == 0x1234, "script properties write exactly native fields");
    instance.view().push(vm); sq_pushstring(vm, "value", -1);
    require(function_4aabd0(address(vm)) == 1 && integer(vm) == 123, "direct instance getter");
    sq_settop(vm, 0);
    Object types(vm); require(get_slot(vm, ObjectView(&root_storage), "__SqTypes", types.view()), "published type dictionary");
    types.view().push(vm); sq_pushinteger(vm, address(kinoko_native_binding_type(2)));
    require(SQ_SUCCEEDED(sq_rawget(vm, -2)) && text(vm) == "float", "descriptor names keyed by descriptor identity"); sq_settop(vm, 0);
    // Access to a static field is independent of the instance native pointer.
    const auto metadata = function_45fab0(klass.location(), address("value"));
    auto info = load<Variable>(metadata); info.flags = Static; info.offset = address(&native.value); store(metadata, info);
    instance.view().push(vm); sq_setinstanceup(vm, -1, nullptr); sq_pushstring(vm, "value", -1);
    require(function_4aabd0(address(vm)) == 1 && integer(vm) == 123, "static property with null native instance"); sq_settop(vm, 0);
    info.flags = Constant; info.offset = -25; store(metadata, info);
    instance.view().push(vm); sq_pushstring(vm, "value", -1);
    require(function_4aabd0(address(vm)) == 1 && integer(vm) == -25, "constant instance property"); sq_settop(vm, 0);
    info.flags = 0; store(metadata, info);
    instance.view().push(vm); sq_pushstring(vm, "value", -1);
    int32_t resolved = 4, source = 4;
    require(retdec_resolve_instance_var(address(vm), 2, &resolved, &source) == 0 && !resolved && !source, "instance field rejects null native pointer");
}

int native_release_count = 0;
SQInteger released(SQUserPointer, SQInteger) { ++native_release_count; return 0; }
int32_t __fastcall no_args(void* object, void*) {
    auto& native = *static_cast<Native*>(object); ++native.calls; return native.value;
}
int32_t __fastcall one_int(void* object, void*, int32_t value) {
    auto& native = *static_cast<Native*>(object); ++native.calls; native.value = value; return 0;
}
int32_t __fastcall two_float(void* object, void*, int32_t a, int32_t b) {
    auto& native = *static_cast<Native*>(object); ++native.calls; native.arguments[0] = a; native.arguments[1] = b; return 0;
}
int32_t __fastcall four_float(void* object, void*, int32_t a, int32_t b, int32_t c, int32_t d) {
    auto& native = *static_cast<Native*>(object); ++native.calls; native.arguments = {a,b,c,d}; return 0x100;
}
int32_t __fastcall consume_object(void* object, void*, int32_t vtable, int32_t type, int32_t data) {
    auto& native = *static_cast<Native*>(object); ++native.calls;
    require(vtable == kinoko_squirrel_object_vtable() && type == OT_USERDATA, "by-value object ABI");
    int32_t value[3] = {vtable, type, data}; function_4a9d70_this(address(value)); return 0;
}
void methods_contract(HSQUIRRELVM vm) {
    StackTop stack(vm);
    Object klass(vm), instance(vm);
    make_class(vm, klass, "MethodActor");
    install_method(vm, klass, "touch", reinterpret_cast<void*>(&no_args), reinterpret_cast<void*>(&function_460b00));
    install_method(vm, klass, "value", reinterpret_cast<void*>(&no_args), reinterpret_cast<void*>(&function_460c10));
    install_method(vm, klass, "set", reinterpret_cast<void*>(&one_int), reinterpret_cast<void*>(&function_460bc0));
    install_method(vm, klass, "xy", reinterpret_cast<void*>(&two_float), reinterpret_cast<void*>(&function_460cc0));
    install_method(vm, klass, "rect", reinterpret_cast<void*>(&four_float), reinterpret_cast<void*>(&function_460c70));
    install_method(vm, klass, "consume", reinterpret_cast<void*>(&consume_object), reinterpret_cast<void*>(&function_460b50));
    Native native;
    make_instance(vm, klass, instance, &native); publish(vm, "methodActor", instance);
    run_script(vm, R"nut(
methodActor.touch();
methodActor.set(-123);
if(methodActor.value()!=-123) throw "return integer";
methodActor.xy(1.25,-0.0);
if(methodActor.rect(1.0,2.0,3.0,4.0)!=false) throw "low byte bool";
)nut");
    require(native.calls == 5 && native.value == -123 && native.arguments[3] == load<int32_t>(std::array<float,1>{4}.data()), "script calls native methods with original results");
    int calls_before = native.calls;
    run_script(vm, R"nut(
local caught=false;
try { methodActor.set(1.0);
} catch(e) { caught=true;
} if(!caught) throw "strict integer";
caught=false;
try { methodActor.xy(1,2.0);
} catch(e) { caught=true;
} if(!caught) throw "strict floats";
)nut");
    require(native.calls == calls_before, "wrong method argument types do not invoke target");
    // Real callback invocation with a by-value owning wrapper.
    Object consume(vm); require(get_slot(vm, instance.view(), "consume", consume.view()), "get object-taking method");
    consume.view().push(vm); instance.view().push(vm);
    sq_newuserdata(vm, 16); sq_setreleasehook(vm, -1, released);
    const auto release_before = native_release_count;
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 2, 0, 0)), "invoke by-value method"); sq_pop(vm, 1);
    require(native_release_count == release_before + 1, "callee consumes wrapper exactly once");
    // This-adjustment is a byte offset and float arguments are passed as bits.
    sq_pushfloat(vm, -0.0f); sq_pushfloat(vm, 2.5f);
    const auto before = native.calls;
    require(function_45f9d0(add_address(address(&native), -8), address(reinterpret_cast<void*>(&two_float)), 8, address(vm), 1) == 0, "receiver offset adapter");
    require(native.calls == before + 1 && static_cast<uint32_t>(native.arguments[0]) == 0x80000000u, "float sign bit preserved");
    sq_settop(vm, 0);
    require(function_45f850(address(&native), address(reinterpret_cast<void*>(&one_int)), 0, address(vm), INT32_MAX) == -1, "large integer index checked");
    require(function_45f8c0(address(&native), address(reinterpret_cast<void*>(&four_float)), 0, address(vm), INT32_MAX) == -1, "float range overflow checked");
    require(function_45f560(address(vm), 1) == 0 && function_45f5a0(address(vm), INT32_MIN) == 0, "invalid conversion outputs initialized");
    for (int size = 0; size < 8; ++size) {
        instance.view().push(vm); sq_newuserdata(vm, size);
        std::array<unsigned char, 10> result{}; result.fill(0xAB);
        function_460540_this(address(result.data()+1), address(vm));
        require(load<int32_t>(result.data()+5) == 0 && result.front()==0xAB && result.back()==0xAB, "short method descriptor rejected without overrunning result");
        require(function_460b00(address(vm)) == -1, "short method descriptor reports invalid instance");
        sq_settop(vm, 0);
    }
    instance.view().push(vm);
    auto* payload = sq_newuserdata(vm, sizeof(Method));
    store(payload, Method{address(reinterpret_cast<void*>(&no_args)), 0});
    sq_settypetag(vm, -1, reinterpret_cast<void*>(1));
    require(function_460b00(address(vm)) == -1, "tagged method descriptor rejected"); sq_settop(vm, 0);
    int32_t output[2] = {99,99};
    function_460540_this(address(output), address(vm));
    require(!output[0] && !output[1], "empty method frame cleared");
    require(load<int32_t>(function_460540(address(vm))) == 0, "legacy result wrapper");
}


// Exercise the exported table callbacks through an actual native call frame.
void table_callback_contract(HSQUIRRELVM vm) {
    StackTop stack(vm);
    Object table(vm); new_table(vm, table.view());
    int32_t score = 73;
    const auto metadata = function_45fab0(table.location(), address("score"));
    require(metadata != 0, "table callback metadata");
    store(metadata, Variable{address(&score), 0, 0, 0, 4, 0});
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(&function_4aab60), 0);
    table.view().push(vm); sq_pushstring(vm, "score", -1);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 2, 1, 0)) && integer(vm) == 73,
        "source native table getter");
    sq_pop(vm, 2);
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(&function_4aaf30), 0);
    table.view().push(vm); sq_pushstring(vm, "score", -1); sq_pushinteger(vm, -91);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 3, 1, 0)) && integer(vm) == -91 && score == -91,
        "source native table setter");
    sq_pop(vm, 2);
    require(current_vm() == vm && sq_gettop(vm) == 0, "table callback context restored");
}
int32_t __fastcall mapped_value(void* self, void*) { return load<int32_t>(self); }
void mapped_method_contract(HSQUIRRELVM vm) {
    StackTop stack(vm);
    Object klass(vm), instance(vm), mapping(vm);
    make_class(vm, klass, "MappedActor");
    static int foreign_type;
    klass.view().push(vm);
    require(SQ_SUCCEEDED(sq_settypetag(vm, -1, &foreign_type)), "foreign class tag");
    sq_pop(vm, 1);
    int32_t native[2] = {12, 34}, mapped[2] = {56, 78};
    make_instance(vm, klass, instance, native);
    require(get_slot(vm, klass.view(), "__ot", mapping.view()), "foreign receiver map");
    mapping.view().push(vm);
    sq_pushinteger(vm, address(kinoko_native_binding_type(-1)));
    sq_pushuserpointer(vm, mapped);
    require(SQ_SUCCEEDED(sq_rawset(vm, -3)), "install mapped actor receiver"); sq_pop(vm, 1);
    auto* payload = sq_newuserdata(vm, sizeof(Method));
    store(payload, Method{address(reinterpret_cast<void*>(&mapped_value)), 4});
    sq_newclosure(vm, reinterpret_cast<SQFUNCTION>(&function_460c10), 1);
    instance.view().push(vm);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 1, 1, 0)) && integer(vm) == 78,
        "foreign __ot mapping and descriptor receiver offset");
    sq_pop(vm, 2);
    require(native[0] == 12 && native[1] == 34 && mapped[0] == 56 && mapped[1] == 78,
        "mapped method leaves native records intact");
}
void child_binding_contract(HSQUIRRELVM vm) {
    StackTop stack(vm);
    Object klass(vm), instance(vm);
    make_class(vm, klass, "ThreadBoundActor");
    function_460920(pointer<int32_t>(klass.location()), kinoko_native_binding_type(-1),
        0, const_cast<char*>("score"), 0);
    int32_t native = 22;
    make_instance(vm, klass, instance, &native);
    // Keep the actual child VM on its parent's stack. Closing the root shared
    // state here would invalidate the other test's external object references.
    auto* child = sq_newthread(vm, 32);
    require(child != nullptr, "create real child VM");
    StackTop child_stack(child);
    const char code[] = "this.score = 908; return this.score;";
    require(SQ_SUCCEEDED(sq_compilebuffer(child, code, sizeof(code)-1, "child-binding", SQTrue)),
        "compile child property bytecode");
    instance.view().push(child);
    require(current_vm() == vm, "parent context before child call");
    require(SQ_SUCCEEDED(kinoko_sq_call(address(child), 1, 1, 0)) && integer(child) == 908 && native == 908,
        "child executes native property getter and setter");
    require(current_vm() == vm, "child native callbacks restore parent context");
}

void argument_guards(HSQUIRRELVM vm) {
    StackTop stack(vm);
    int32_t output = 99; float f = 7;
    for (int index : {0, 1, -1, INT32_MIN, INT32_MAX}) {
        require(!retdec_native_integer_arg(address(vm), index, &output) && output == 99, "invalid integer argument leaves output");
        require(!retdec_native_string_arg(address(vm), index, &output) && output == 99, "invalid string argument leaves output");
        require(!retdec_native_float_arg(address(vm), index, &f) && f == 7, "invalid float argument leaves output");
    }
    require(!retdec_native_string_arg(0, 1, &output), "null VM string argument");
    for (int size = 0; size < 4; ++size) {
        sq_newuserdata(vm, size);
        require(retdec_native_target_from_userdata(address(vm)) == 0, "undersized native target rejected");
        require(retdec_native_callback_from_stack(address(vm)) == 0, "undersized callback rejected");
        sq_pop(vm, 1);
    }
    auto* payload = sq_newuserdata(vm, 4); store(payload, int32_t{123});
    require(retdec_native_target_from_userdata(address(vm)) == 123 && retdec_native_callback_from_stack(address(vm)) == 123, "four-byte native payload retained");
    sq_pop(vm, 1); sq_pushinteger(vm, 17);
    require(retdec_native_integer_arg(address(vm), -1, &output) == 1 && output == 17, "valid negative argument index retained");
}
} // namespace

int main() {
    try {
        for (int pass = 0; pass < 8; ++pass) {
            Machine machine; auto* vm = machine.get();
            class_contract(vm); lookup_contract(vm); values_contract(vm);
            instance_contract(vm); methods_contract(vm); argument_guards(vm);
            table_callback_contract(vm); mapped_method_contract(vm); child_binding_contract(vm);
            require(sq_gettop(vm) == 0, "all binding contracts restore caller stack");
        }
        std::puts("Squirrel binding contracts passed: 8 full VM lifetimes, class/metadata/variables/methods/argument guards/table callbacks/mapped receivers/child VMs");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Squirrel binding contract failed: %s\n", error.what());
        return 1;
    }
}
