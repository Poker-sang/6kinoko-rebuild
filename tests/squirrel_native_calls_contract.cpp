#include "kinoko/squirrel_native_calls.h"
#include "squirrel_bridge_test_support.hpp"
#include <climits>

extern "C" {
char kinoko_sqrat_trace_enabled = 0;
const void* kinoko_squirrel_object_vtable(void) { return reinterpret_cast<const void*>(0x13572468); }
void kinoko_trace_i32(const char*, int32_t) {}
}
namespace {
using namespace bridge_test;
using kinoko::script::ObjectStorage;
using kinoko::script::data_bits;
using kinoko::script::borrowed_value;
using Export = SQFUNCTION;
SQFUNCTION entry(Export fn) { return reinterpret_cast<SQFUNCTION>(fn); }
int native_calls = 0, consumed = 0, released = 0, property_value = 0;
HSQUIRRELVM active_vm = nullptr;
struct Native { int32_t marker = 31337; int32_t value = 0; };
Native native;

int32_t __cdecl set_name(int32_t self, const char* value) {
    require(self == address(&native) && std::string(value) == "name", "explicit cdecl receiver/string");
    require(receiver == address(active_vm), "cdecl callback active VM receiver");
    ++native_calls; return 98;
}
int32_t __cdecl set_three(int32_t self, int32_t a, int32_t b, int32_t c) {
    require(self == address(&native) && a == -3 && b == 17 && c == 29, "cdecl argument order");
    ++native_calls; return 97;
}
int32_t __cdecl two(int32_t self, int32_t a, int32_t b) {
    require(self == address(&native) && a == 19 && b == 7, "cdecl two arguments");
    ++native_calls; return 0x100; // Boolean is full-word !=0, not low-byte truth.
}
void __fastcall zero_method(Native* self, void*) { require(self == &native, "zero thiscall ECX"); ++self->value; }
void __fastcall one_method(Native* self, void*, int32_t value) {
    require(self == &native, "one thiscall ECX"); self->value = value;
}
int32_t __fastcall draw_method(Native* self, void*, int32_t x, int32_t y,
    int32_t w, int32_t h, int32_t resource, int32_t sx, int32_t sy, int32_t blend, float alpha) {
    require(self == &native && x == -2 && y == 3 && w == 4 && h == 5, "draw dimensions and receiver");
    require(resource == address(&native) && sx == 6 && sy == 7 && blend == 8, "draw resource and flags");
    require(alpha == 0.375f, "draw floating point argument bits");
    ++native_calls; return 1234;
}
SQInteger release_userdata(SQUserPointer, SQInteger) { ++released; return 0; }
void consume(ObjectStorage object) {
    require(object.vtable == kinoko_squirrel_object_vtable(), "by-value vtable");
    require(object.value._type == OT_USERDATA, "by-value object tag");
    // A single external handle was transferred to this native callee. No caller
    // destructor may consume it a second time; stack ownership is independent.
    require(sq_release(active_vm, &object.value) == SQTrue, "callee consumes transferred last external handle");
    ++consumed;
}
int32_t __cdecl pair_id(int32_t id, ObjectStorage closure, ObjectStorage environment) {
    require(id == 42, "callback integer id"); consume(closure); consume(environment); return 0;
}
int32_t __cdecl pair_name(int32_t text, ObjectStorage closure, ObjectStorage environment) {
    require(std::string(pointer<char>(text)) == "pair", "callback name");
    consume(closure); consume(environment); return 0;
}
int32_t __cdecl one_name(const char* text, ObjectStorage environment) {
    require(std::string(text) == "one", "one-object callback name"); consume(environment); return 0;
}
int32_t __cdecl constant() { ++native_calls; return -971; }
void __cdecl optional_name(const char* name) { require(std::string(name) == "optional", "optional string"); ++native_calls; }
void __cdecl optional_two(int32_t a, int32_t b) { require(a == -5 && b == 13, "optional two integer order"); ++native_calls; }
int32_t __cdecl bool_name(int32_t text) {
    require(std::string(pointer<char>(text)) == "loaded", "single string callback value");
    ++native_calls; return 0x100;
}
int32_t __cdecl string_object(int32_t text, int32_t vtable, int32_t type, int32_t data) {
    require(std::string(pointer<char>(text)) == "table", "string/object callback value");
    ObjectStorage object{static_cast<uint32_t>(vtable), borrowed_value(type, data)};
    require(object.vtable == kinoko_squirrel_object_vtable(), "string/object wrapper vtable");
    require(sq_release(active_vm, &object.value) == SQTrue, "string/object callee consumes external handle");
    ++consumed; return 1;
}
void __cdecl play_four(int32_t text, int32_t a, int32_t b, int32_t truth) {
    require(std::string(pointer<char>(text)) == "bgm" && a == 2 && b == 3 && truth == 1, "four argument callback");
    ++native_calls;
}
void __cdecl play_five(int32_t text, int32_t a, int32_t b, int32_t c, int32_t truth) {
    require(std::string(pointer<char>(text)) == "se" && a == 4 && b == 5 && c == 6 && truth == 0, "five argument callback");
    ++native_calls;
}
int32_t __cdecl integer_callback(int32_t value) { require(value == -44, "integer callback"); ++native_calls; return 0; }
int32_t __cdecl float_callback(float a, float b) { require(a == 1.5f && b == -2.25f, "float callback"); ++native_calls; return 0; }

void captured_closure(HSQUIRRELVM vm, Export fn, int32_t target,
    SQInteger size = sizeof(int32_t), bool tagged = false) {
    void* payload = sq_newuserdata(vm, size);
    if (size >= 4) store(payload, target);
    if (tagged) require(SQ_SUCCEEDED(sq_settypetag(vm, -1, &native)), "set capture typetag");
    sq_newclosure(vm, entry(fn), 1);
}
void register_method(HSQUIRRELVM vm, const char* name, Export fn, int32_t target,
    bool tagged = false) {
    sq_pushstring(vm, name, -1); captured_closure(vm, fn, target, 4, tagged);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "register native method");
}
void register_root(HSQUIRRELVM vm, const char* name, Export fn, int32_t target) {
    Top restore(vm); sq_pushroottable(vm); register_method(vm, name, fn, target);
}
void push_instance(HSQUIRRELVM vm) {
    require(SQ_SUCCEEDED(sq_newclass(vm, SQFalse)), "new source class");
    require(SQ_SUCCEEDED(sq_createinstance(vm, -1)), "new source instance");
    sq_remove(vm, -2); require(SQ_SUCCEEDED(sq_setinstanceup(vm, -1, &native)), "native receiver");
}
void methods(HSQUIRRELVM vm) {
    Top restore(vm);
    const auto before = native_calls;
    sq_pushroottable(vm); sq_pushstring(vm, "Host", -1); sq_newclass(vm, SQFalse);
    register_method(vm,"name",kinoko_input_save_entry,address(reinterpret_cast<void*>(set_name)));
    register_method(vm,"three",kinoko_input_assign_entry,address(reinterpret_cast<void*>(set_three)));
    register_method(vm,"truth",kinoko_input_wait_entry,address(reinterpret_cast<void*>(two)));
    register_method(vm,"number",kinoko_input_get_entry,address(reinterpret_cast<void*>(two)));
    register_method(vm,"zero",kinoko_native_nullary_member_callback,address(reinterpret_cast<void*>(zero_method)),true);
    register_method(vm,"one",kinoko_native_integer_member_callback,address(reinterpret_cast<void*>(one_method)),true);
    register_method(vm,"draw",kinoko_native_draw_member_callback,address(reinterpret_cast<void*>(draw_method)),true);
    sq_pushstring(vm,"weakref",-1); sq_newclosure(vm,entry(kinoko_native_class_weakref_callback),0); sq_newslot(vm,-3,SQFalse);
    require(SQ_SUCCEEDED(sq_newslot(vm,-3,SQFalse)),"publish class");
    sq_pushstring(vm,"host",-1);
    sq_pushstring(vm,"Host",-1); require(SQ_SUCCEEDED(sq_get(vm,-3)),"get Host");
    require(SQ_SUCCEEDED(sq_createinstance(vm,-1)),"create Host"); sq_remove(vm,-2);
    sq_setinstanceup(vm,-1,&native); require(SQ_SUCCEEDED(sq_newslot(vm,-3,SQFalse)),"publish receiver");
    sq_pop(vm,1);
    evaluate(vm,
        "host.name(\"name\");\n"
        "host.three(-3,17,29);\n"
        "if (host.truth(19,7) != true) throw \"truth\";\n"
        "if (host.number(19,7) != 256) throw \"number\";\n"
        "host.zero();\n"
        "host.one(37.9);\n"
        "if (host.draw(-2,3,4,5,host,6,7,8,0.375) != 1234) throw \"draw\";\n"
        "if (host.weakref().ref() != host) throw \"weak reference\";\n"
        "local caught = false;\n"
        "try { host.three(-3,17.0,29); } catch(e) { caught = true; }\n"
        "if (!caught) throw \"strict arguments\";\n"
        "host.one(\"bad\");\n");
    require(native_calls == before + 5 && native.value == 37, "all method families and rejected argument");
}
void push_owned_userdata(HSQUIRRELVM vm) {
    sq_newuserdata(vm, 8); sq_setreleasehook(vm,-1,release_userdata);
}
void ownership(HSQUIRRELVM vm) {
    Top restore(vm);
    const auto saved_consumed=consumed, saved_released=released;
    for (auto fn : {kinoko_native_integer_pair_entry,kinoko_native_create_event_callback,kinoko_native_string_object_callback}) {
        const auto base=sq_gettop(vm);
        auto callback = fn == kinoko_native_integer_pair_entry ? reinterpret_cast<void*>(pair_id) :
            fn == kinoko_native_create_event_callback ? reinterpret_cast<void*>(pair_name) : reinterpret_cast<void*>(one_name);
        captured_closure(vm,fn,address(callback)); sq_pushroottable(vm);
        if (fn == kinoko_native_integer_pair_entry) sq_pushinteger(vm,42);
        else sq_pushstring(vm,fn == kinoko_native_string_object_callback ? "one" : "pair",-1);
        push_owned_userdata(vm);
        if (fn != kinoko_native_string_object_callback) push_owned_userdata(vm);
        const auto arguments=fn == kinoko_native_string_object_callback ? 3 : 4;
        require(SQ_SUCCEEDED(kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), arguments, SQFalse, SQTrue)),"by-value real native callback");
        top(vm,base+1,"sq_call retains native closure"); sq_pop(vm,1);
    }
    require(consumed == saved_consumed+5 && released == saved_released+5,"exactly once external consumption and internal final release");
    // Missing string must not transfer any handles or call the target.
    captured_closure(vm,kinoko_native_integer_pair_entry,address(reinterpret_cast<void*>(pair_id)));
    sq_pushroottable(vm); sq_pushfloat(vm,42); push_owned_userdata(vm); push_owned_userdata(vm);
    require(SQ_FAILED(kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), 4, SQFalse, SQFalse)),"wrong id rejected");
    sq_pop(vm,1);
    require(consumed == saved_consumed+5 && released == saved_released+7,"failure does not create/leak external handles");
}
SQInteger read_property(HSQUIRRELVM vm) {
    require(receiver == address(vm) && sq_gettop(vm)==1,"getter receiver and frame");
    sq_pushinteger(vm,property_value); return 1;
}
SQInteger write_property(HSQUIRRELVM vm) {
    require(receiver == address(vm) && sq_gettop(vm)==2,"setter receiver and frame");
    property_value=get_integer(vm,2); return 0;
}
SQInteger fail_property(HSQUIRRELVM vm) { return sq_throwerror(vm,"inner failure"); }
void property_table(HSQUIRRELVM vm, SQFUNCTION callback) {
    sq_newtable(vm); sq_pushstring(vm,"value",-1); sq_newclosure(vm,callback,0);
    require(SQ_SUCCEEDED(sq_newslot(vm,-3,SQFalse)),"property lookup table");
}
void properties(HSQUIRRELVM vm) {
    Top restore(vm);
    for (int write=0;write<2;++write) {
        const auto base=sq_gettop(vm);
        property_table(vm,write ? write_property : read_property);
        sq_newclosure(vm,entry(write ? kinoko_native_property_get_callback : kinoko_native_property_set_callback),1);
        sq_pushroottable(vm); sq_pushstring(vm,"value",-1);
        if (write) sq_pushinteger(vm,779);
        require(SQ_SUCCEEDED(kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), write ? 3 : 2, SQTrue, SQFalse)),"property dispatch call");
        require(write ? sq_gettype(vm,-1)==OT_NULL : get_integer(vm)==property_value,"getter/setter returns");
        sq_settop(vm,base);
    }
    require(property_value==779,"setter passed value");
    property_table(vm,read_property); sq_newclosure(vm,entry(kinoko_native_property_set_callback),1);
    sq_pushroottable(vm); sq_pushstring(vm,"absent",-1);
    require(SQ_FAILED(kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), 2, SQFalse, SQFalse)),"missing member throws"); sq_pop(vm,1);
    // Original native entry ignores inner sq_call failure. Assert that unusual
    // result/stack convention directly instead of silently 'fixing' behavior.
    const auto base=sq_gettop(vm);
    sq_pushroottable(vm); sq_pushstring(vm,"value",-1); property_table(vm,fail_property);
    require(kinoko_native_property_set_callback((struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))))==1,"legacy getter return despite inner failure");
    top(vm,base+4,"failed inner call retains closure without invented cleanup");
}
void migrated_adapters(HSQUIRRELVM vm) {
    Top restore(vm);
    const auto base = sq_gettop(vm);
    auto truth = [&](auto push, int expected) {
        sq_settop(vm, base); push();
        require(kinoko_native_truthy_entry((struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), base + 1) == expected, "source truth conversion");
    };
    truth([&]{ sq_pushnull(vm); }, 0);
    truth([&]{ sq_pushbool(vm, SQFalse); }, 0);
    truth([&]{ sq_pushbool(vm, SQTrue); }, 1);
    truth([&]{ sq_pushinteger(vm, 0); }, 0);
    truth([&]{ sq_pushinteger(vm, -7); }, 1);
    truth([&]{ sq_pushfloat(vm, 0.0f); }, 0);
    truth([&]{ sq_pushfloat(vm, -0.5f); }, 1);
    truth([&]{ sq_pushstring(vm, "value", -1); }, 1);
    truth([&]{ sq_newtable(vm); }, 1);

    sq_settop(vm, base); sq_pushstring(vm, "loaded", -1);
    const auto before = native_calls;
    require(kinoko_native_string_bool_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(reinterpret_cast<void*>(bool_name)))), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), base + 1) == 1,
        "single string adapter return count");
    SQBool boolean = SQTrue; require(SQ_SUCCEEDED(sq_getbool(vm, -1, &boolean)) && boolean == SQFalse,
        "single string adapter preserves low-byte BOOL conversion");
    require(native_calls == before + 1, "single string callback called");

    sq_settop(vm, base); sq_pushstring(vm, "table", -1); push_owned_userdata(vm);
    const auto consumed_before = consumed, released_before = released;
    require(kinoko_native_string_object_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(reinterpret_cast<void*>(string_object)))), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), base + 1) == 1,
        "string/object adapter return count");
    require(consumed == consumed_before + 1, "string/object callback consumes one external handle");
    sq_settop(vm, base); sq_collectgarbage(vm);
    require(released == released_before + 1, "string/object stack ownership releases exactly once");

    sq_pushstring(vm, "table", -1); push_owned_userdata(vm);
    require(kinoko_native_string_object_entry((void*)(uintptr_t)((void*)(uintptr_t)(0)), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), base + 1) == 0, "null string/object callback");
    sq_settop(vm, base); sq_collectgarbage(vm);
    require(released == released_before + 2, "null callback balances temporary external handle");

    sq_pushstring(vm, "bgm", -1); sq_pushinteger(vm, 2); sq_pushinteger(vm, 3); sq_pushstring(vm, "truthy", -1);
    require(kinoko_native_string_two_integer_truth_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(reinterpret_cast<void*>(play_four)))), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), base + 1) == 0,
        "four argument source adapter");
    sq_settop(vm, base); sq_pushstring(vm, "se", -1); sq_pushinteger(vm, 4); sq_pushinteger(vm, 5);
    sq_pushinteger(vm, 6); sq_pushnull(vm);
    require(kinoko_native_string_three_integer_truth_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(reinterpret_cast<void*>(play_five)))), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), base + 1) == 0,
        "five argument source adapter");

    sq_settop(vm, base); captured_closure(vm, kinoko_native_integer_entry, address(reinterpret_cast<void*>(integer_callback)));
    sq_pushroottable(vm); sq_pushinteger(vm, -44);
    require(SQ_SUCCEEDED(kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), 2, SQFalse, SQFalse)), "captured integer wrapper");
    sq_settop(vm, base); captured_closure(vm, kinoko_native_two_floats_entry, address(reinterpret_cast<void*>(float_callback)));
    sq_pushroottable(vm); sq_pushfloat(vm, 1.5f); sq_pushfloat(vm, -2.25f);
    require(SQ_SUCCEEDED(kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), 3, SQFalse, SQFalse)), "captured float wrapper");
    require(native_calls == before + 5, "all migrated callbacks invoked");
    sq_settop(vm, base);
}
void invalid_and_optional(HSQUIRRELVM vm) {
    Top restore(vm);
    const auto base=sq_gettop(vm);
    for (int32_t index : {INT_MIN,-100,0,100,INT_MAX}) {
        require(kinoko_native_three_integer_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(&native))), (void*)(uintptr_t)((void*)(uintptr_t)(0)), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), index)<0,"invalid index checked before API");
        require(kinoko_native_two_integer_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(0)), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), index)<0,"two argument index arithmetic bounded");
        require(kinoko_native_string_pair_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(1)), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), index)<0,"pair argument index bounded");
    }
    const auto transfers_before = consumed;
    sq_pushstring(vm,"pair",-1); push_owned_userdata(vm); push_owned_userdata(vm);
    require(kinoko_native_string_pair_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(reinterpret_cast<void*>(pair_name)))), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), -3)<0,
        "pair conversion preserves positive-index-only rule");
    require(consumed==transfers_before,"negative pair indices transfer no references");
    sq_settop(vm,base);
    for (int size=0;size<4;++size) {
        push_instance(vm); sq_newuserdata(vm,size); int32_t output[2]={-1,-1};
        kinoko_native_capture_receiver_pair((struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), (void**)((void**)((void**)(output))));
        require(output[0]==address(&native) && output[1]==0,"short capture rejected"); sq_settop(vm,base);
    }
    push_instance(vm); sq_newuserdata(vm,4); sq_settypetag(vm,-1,&native);
    int32_t output[2]{}; kinoko_native_capture_receiver_pair((struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), (void**)((void**)((void**)(output))));
    require(output[1]==0,"cdecl captures must be untagged"); sq_settop(vm,base);
    sq_pushstring(vm,"optional",-1);
    require(kinoko_native_string_only_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(0)), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), 1)==0,"optional null string callback");
    require(kinoko_native_string_only_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(reinterpret_cast<void*>(optional_name)))), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), 1)==0,"optional string callback");
    sq_settop(vm,base); sq_pushinteger(vm,-5); sq_pushinteger(vm,13);
    require(kinoko_native_two_integer_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(0)), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), 1)==0,"optional null pair callback");
    require(kinoko_native_two_integer_callback_entry((void*)(uintptr_t)((void*)(uintptr_t)(address(reinterpret_cast<void*>(optional_two)))), (struct SQVM*)(uintptr_t)((struct SQVM*)(uintptr_t)(address(vm))), 1)==0,"optional two integers callback");
    sq_settop(vm,base);
    register_root(vm,"constant",kinoko_native_integer_result_entry,address(reinterpret_cast<void*>(constant)));
    register_root(vm,"none",kinoko_native_integer_result_entry,0);
    evaluate(vm,"if (constant() != -971) throw \"constant\";\nif (none() != 0) throw \"null target\";\n");
}
}
int main() {
    try {
        for (int pass=0;pass<8;++pass) {
            Machine machine; auto vm=machine.get(); active_vm=vm;
            methods(vm); ownership(vm); properties(vm); migrated_adapters(vm); invalid_and_optional(vm);
            sq_newthread(vm,64);
            HSQUIRRELVM child=nullptr; require(SQ_SUCCEEDED(sq_getthread(vm,-1,&child)),"real child VM");
            active_vm=child; properties(child); ownership(child); top(child,0,"child frames balanced");
            active_vm=vm; sq_pop(vm,1); top(vm,0,"root frames balanced");
            std::printf("native calls pass %d: receiver, ownership, dispatch, stack, child VM OK\n",pass+1);
        }
        return 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"FAIL: %s\n",error.what()); return 1; }
}
