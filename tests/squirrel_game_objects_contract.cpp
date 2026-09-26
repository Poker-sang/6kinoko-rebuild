#include "kinoko/script_callbacks.h"
#include "kinoko/script_file.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/sqrat_object_bridge.h"
#include "squirrel_bridge_test_support.hpp"
#include <cstdlib>
#include <vector>

static_assert(!noexcept(kinoko_script_load_file(nullptr, nullptr)), "SqPlus file load may throw across C linkage");
static_assert(!noexcept(kinoko_script_compile_file_argument(0, 0, 0, 0, 0, 0)), "owning argument must unwind on file errors");

extern "C" {
struct SQVM *kinoko_primary_vm = nullptr;
char kinoko_sqrat_trace_enabled = 0;
int32_t kinoko_squirrel_object_vtable(void) { return 0x12345678; }
int32_t kinoko_sqrat_object_vtable(void) { return 0x12121212; }
int32_t kinoko_sqrat_root_vtable(void) { return 0x34343434; }
int32_t kinoko_native_void_type(void) { return 0x13572468; }
void kinoko_trace(const char*) {}
void kinoko_trace_i32(const char*, int32_t) {}
void kinoko_trace_squirrel_name(const char*, int32_t) {}
void kinoko_host_free_allocation(int32_t* value) { std::free(value); }
}
namespace {
using namespace bridge_test;
using kinoko::script::ObjectView;
using kinoko::script::ObjectStorage;
using kinoko::script::data_bits;
int releases = 0;
int native = 31337;
SQInteger release_native(SQUserPointer value, SQInteger) {
    require(value == &native,"release native instance pointer"); ++releases; return 0;
}
SQInteger release_userdata(SQUserPointer, SQInteger) { ++releases; return 0; }
HSQOBJECT empty() { HSQOBJECT value; sq_resetobject(&value); return value; }
int32_t exchange_vm(int32_t value) {
    const auto previous=address(kinoko_primary_vm); kinoko_primary_vm=pointer<SQVM>(value); receiver=value; return previous;
}
void destroy(Pair& value) {
    auto old=value.get(); sq_release(value.vm,&old); value.write(empty());
}
void root(HSQUIRRELVM vm, Pair& value) { sq_pushroottable(vm); value.capture(); }
void erase_slot(HSQUIRRELVM vm, const Pair& object, const char* name) {
    Top restore(vm); object.push(); sq_pushstring(vm,name,-1);
    require(SQ_SUCCEEDED(sq_deleteslot(vm,-2,SQFalse)),"delete test slot");
}
void constructors(HSQUIRRELVM vm) {
    Top restore(vm);
    evaluate(vm,"ctor_calls <- 0;\nclass Bound { constructor() { ++ctor_calls; } }\n");
    Pair class_value(vm), parent(vm), bound(vm), unbound(vm), wrong(vm);
    evaluate(vm,"return Bound;",&class_value); root(vm,parent);
    const auto base=sq_gettop(vm); const auto before=releases;
    require(kinoko_create_bound_instance((struct SQVM*)(uintptr_t)(address(vm)), parent.data(), "bound", class_value.data(), (void*)(uintptr_t)(address(&native)), bound.data())==1,"create/publish instance");
    require(kinoko_create_unbound_instance((struct SQVM*)(uintptr_t)(address(vm)), class_value.data(), (void*)(uintptr_t)(address(&native)), unbound.data())==1,"create unbound instance");
    top(vm,base,"instance creation balances stack");
    evaluate(vm,"if (ctor_calls != 0) throw \"constructor executed\";\nif (typeof bound != \"instance\") throw \"published type\";\n");
    for (auto* object : {&bound,&unbound}) {
        object->push(); SQUserPointer value=nullptr;
        require(SQ_SUCCEEDED(sq_getinstanceup(vm,-1,&value,nullptr)) && value==&native,"instance pointer");
        sq_setreleasehook(vm,-1,release_native); sq_pop(vm,1);
    }
    const auto unchanged=std::array<int32_t,2>{17,29}; auto output=unchanged;
    require(!kinoko_create_unbound_instance((struct SQVM*)(uintptr_t)(address(vm)), wrong.data(), (void*)(uintptr_t)(address(&native)), output.data()) && output==unchanged,"invalid class preserves outputs");
    // The supplied 2.2.2 sq_newslot returns SQ_OK for a non-table/class parent,
    // without publishing anything. Preserve the recovered helper's success and
    // its owned instance output instead of inventing a failure for this case.
    require(kinoko_create_bound_instance((struct SQVM*)(uintptr_t)(address(vm)), wrong.data(), "unpublished", class_value.data(), (void*)(uintptr_t)(address(&native)), output.data())==1,
        "non-table parent retains 2.2.2 newslot success convention");
    auto unpublished=load<HSQOBJECT>(output.data());
    require(unpublished._type==OT_INSTANCE && data_bits(unpublished)!=0,"unpublished instance output is owned");
    require(sq_release(vm,&unpublished)==SQTrue,"release unpublished output handle");
    top(vm,base,"nonpublishing newslot path balances stack");
    erase_slot(vm,parent,"bound"); require(releases==before,"external bound handle survives slot deletion");
    destroy(bound); destroy(unbound); require(releases==before+2,"instances release exactly once");
}
void resource_roots(HSQUIRRELVM vm) {
    Top restore(vm);
    Pair table(vm), weak(vm);
    sq_newtable(vm); table.capture();
    table.push(); sq_weakref(vm,-1); weak.capture(); sq_pop(vm,1);
    std::array<unsigned char,180> storage; storage.fill(0xa7);
    auto* resource=storage.data()+1; // Deliberately unaligned legacy storage.
    store(resource+152,address(vm)); store(resource+156,empty());
    require(kinoko_bind_act_resource_root((void*)(uintptr_t)(address(resource)), (struct SQVM*)(uintptr_t)(address(vm)), table.data())==1,"bind resource root");
    destroy(table); // The resource is now the table's only strong owner.
    require(kinoko_bind_act_resource_root((void*)(uintptr_t)(address(resource)), (struct SQVM*)(uintptr_t)(address(vm)), reinterpret_cast<int32_t*>(resource+156))==1,"self-rebinding retains last root");
    weak.push(); require(SQ_SUCCEEDED(sq_getweakrefval(vm,-1)) && sq_gettype(vm,-1)==OT_TABLE,"self-bind does not invalidate weakref"); sq_pop(vm,2);
    auto value=load<HSQOBJECT>(resource+156); sq_release(vm,&value); store(resource+156,empty());
    weak.push(); sq_getweakrefval(vm,-1); require(sq_gettype(vm,-1)==OT_NULL,"last root releases table"); sq_pop(vm,2);
    for (size_t i=0;i<storage.size();++i) if(i<153 || i>=165) require(storage[i]==0xa7,"resource root canaries");
}
void callbacks(HSQUIRRELVM vm) {
    Top restore(vm);
    Pair table(vm), closure(vm), weak_table(vm), weak_closure(vm);
    sq_newtable(vm); table.capture();
    evaluate(vm,"return function() { return 17; };",&closure);
    require(kinoko_sqrat_set_pair((struct SQVM *)(vm), table.data(), "Update", closure.data()),"publish callback");
    auto object=wrapper(vm,table);
    std::array<unsigned char,84> script; script.fill(0xa7);
    const auto offset=25; store(script.data()+offset,address(vm));
    store(script.data()+offset+4,empty()); store(script.data()+offset+12,empty());
    const auto before=script;
    auto environment=table.get(), function=closure.get();
    table.push(); sq_weakref(vm,-1); weak_table.capture(); sq_pop(vm,1);
    closure.push(); sq_weakref(vm,-1); weak_closure.capture(); sq_pop(vm,1);
    auto has_extra_handle = [&](HSQOBJECT value) {
        sq_pushobject(vm,value); // Keep internal ownership during the probe.
        const auto last = sq_release(vm,&value);
        sq_addref(vm,&value); sq_pop(vm,1);
        return last == SQFalse;
    };
    kinoko_copy_act_callback((struct SQVM*)(uintptr_t)(address(vm)), (void*)(uintptr_t)(address(script.data())), offset, (void*)(uintptr_t)(address(object.data())), "Update");
    require(load<int32_t>(script.data()+offset)==address(vm),"callback stores actual VM");
    require(data_bits(load<HSQOBJECT>(script.data()+offset+4))==data_bits(environment),"callback environment");
    require(data_bits(load<HSQOBJECT>(script.data()+offset+12))==data_bits(function),"callback closure");
    require(has_extra_handle(environment) && has_extra_handle(function),"callback owns both pairs independently");
    kinoko_copy_act_callback((struct SQVM*)(uintptr_t)(address(vm)), (void*)(uintptr_t)(address(script.data())), offset, (void*)(uintptr_t)(address(object.data())), "Update");
    require(has_extra_handle(environment) && has_extra_handle(function),"callback replacement balances handles");
    kinoko_release_act_callback((void*)(uintptr_t)(address(script.data()+offset)));
    kinoko_release_act_callback((void*)(uintptr_t)(address(script.data()+offset)));
    require(!has_extra_handle(environment) && !has_extra_handle(function),"source callback destruction is balanced and repeatable");
    kinoko_copy_act_callback((struct SQVM*)(uintptr_t)(address(vm)), (void*)(uintptr_t)(address(script.data())), offset, (void*)(uintptr_t)(address(object.data())), "Update");
    kinoko_copy_act_callback((struct SQVM*)(uintptr_t)(address(vm)), (void*)(uintptr_t)(address(script.data())), offset, (void*)(uintptr_t)(address(object.data())), "Missing");
    require(load<HSQOBJECT>(script.data()+offset+4)._type==OT_NULL && load<HSQOBJECT>(script.data()+offset+12)._type==OT_NULL,"missing callback clears old pairs");
    require(!has_extra_handle(environment) && !has_extra_handle(function),"missing lookup releases callback references");
    for (auto* owner : {&table,&closure}) {
        auto value=owner->get();
        require(sq_release(vm,&value)==SQTrue,"callback external handles fully released");
        owner->write(empty());
    }
    // 2.2.2's _OP_RETURN stores its result in SQVM::temp_reg, an INTERNAL
    // reference that outlives sq_call. Overwrite it by executing a real return,
    // not by clearing VM internals or collecting away an external-ref leak.
    weak_closure.push(); sq_getweakrefval(vm,-1);
    require(sq_gettype(vm,-1)==OT_CLOSURE,"source return register retains closure"); sq_pop(vm,2);
    evaluate(vm,"return null;");
    for (auto* weak : {&weak_table,&weak_closure}) {
        weak->push(); sq_getweakrefval(vm,-1);
        require(sq_gettype(vm,-1)==OT_NULL,"callback replacement leaves no leaked external handle"); sq_pop(vm,2);
    }
    for(size_t i=0;i<script.size();++i) if(i<offset || i>=offset+20) require(script[i]==before[i],"callback neighbor canaries");
}
SQInteger write_bytecode(SQUserPointer destination, SQUserPointer data, SQInteger length) {
    auto& output=*static_cast<std::vector<unsigned char>*>(destination);
    const auto* first=static_cast<unsigned char*>(data); output.insert(output.end(),first,first+length); return length;
}
std::vector<unsigned char> bytecode(HSQUIRRELVM vm, const char* source) {
    Top restore(vm); std::vector<unsigned char> data;
    require(SQ_SUCCEEDED(sq_compilebuffer(vm,source,static_cast<SQInteger>(std::strlen(source)),"embedded-contract",SQFalse)),"compile embedded source");
    require(SQ_SUCCEEDED(sq_writeclosure(vm,write_bytecode,&data)),"serialize real bytecode"); return data;
}
void embedded(HSQUIRRELVM vm) {
    Top restore(vm); Pair environment(vm); sq_newtable(vm); environment.capture();
    auto data=bytecode(vm,"this.answer <- 713;\n");
    std::array<unsigned char,110> script{};
    store(script.data()+92,address(data.data())); store(script.data()+96,static_cast<int32_t>(data.size()));
    sq_pushinteger(vm,812); const auto base=sq_gettop(vm);
    require(kinoko_execute_embedded_act_script((struct SQVM*)(uintptr_t)(address(vm)), (void*)(uintptr_t)(address(script.data())), environment.data())==1,"load and execute supplied bytecode");
    top(vm,base,"embedded execution restores stack"); require(get_integer(vm)==812,"embedded sentinel survives");
    environment.push(); sq_pushstring(vm,"answer",-1); require(SQ_SUCCEEDED(sq_get(vm,-2)) && get_integer(vm)==713,"uses supplied environment, not root"); sq_pop(vm,2);
    for(int32_t truncated : {0,1,2,7,static_cast<int32_t>(data.size()-1)}) {
        store(script.data()+96,truncated);
        require(kinoko_execute_embedded_act_script((struct SQVM*)(uintptr_t)(address(vm)), (void*)(uintptr_t)(address(script.data())), environment.data())==0,"truncated bytecode rejected");
        top(vm,base,"failed read restores stack");
    }
    auto throwing=bytecode(vm,"throw \"embedded failure\";\n");
    store(script.data()+92,address(throwing.data())); store(script.data()+96,static_cast<int32_t>(throwing.size()));
    require(!kinoko_execute_embedded_act_script((struct SQVM*)(uintptr_t)(address(vm)), (void*)(uintptr_t)(address(script.data())), environment.data()),"script execution failure reported"); top(vm,base,"throwing closure cleanup");
    unsigned char raw[5]={1,2,3,4,5}, result[9]; std::memset(result,0xa7,sizeof(result));
    std::array<int32_t,3> stream{address(raw),5,address(raw)};
    require(kinoko_script_read_memory(stream.data(),result+1,3)==3,"partial stream read");
    require(kinoko_script_read_memory(stream.data(),result+4,9)==2,"clamped final stream read");
    require(kinoko_script_read_memory(stream.data(),result+6,1)==0,"stream EOF");
    require(result[0]==0xa7 && result[6]==0xa7 && std::memcmp(result+1,raw,5)==0,"read length and canaries");
    const auto exhausted=stream;
    require(kinoko_script_read_memory(stream.data(),result,-1)==0 && stream==exhausted,"negative request is nonmutating");
}
SQRESULT text_call(HSQUIRRELVM vm, SQInteger count, SQBool result, SQBool errors) {
    require(count == 1 && result == SQTrue && errors == SQTrue, "text script call flags");
    return kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), count, result, errors);
}
void text_scripts(HSQUIRRELVM vm) {
    Top restore(vm);
    const auto base = sq_gettop(vm);
    auto run = [&](const char* source, const HSQOBJECT* environment) {
        kinoko::script::upstream::sqplus_compile_and_run(vm, source,
            "script-file-contract.nut", environment, text_call);
    };
    run("return 42;", nullptr);
    top(vm, base, "text root execution balances stack");
    Pair environment(vm); sq_newtable(vm); environment.capture();
    auto scope = environment.get();
    run("this.answer <- 713; return this;", &scope);
    top(vm, base, "text custom environment balances stack");
    environment.push(); sq_pushstring(vm, "answer", -1);
    require(SQ_SUCCEEDED(sq_get(vm, -2)) && get_integer(vm) == 713,
        "text executes in supplied environment");
    sq_pop(vm, 2);
    HSQOBJECT null_scope; sq_resetobject(&null_scope);
    run("if (this != null) throw \"unexpected root fallback\";", &null_scope);
    top(vm, base, "text explicit null does not use root");
    for (const char* source : {"local = ;", "throw \"text failure\";"}) {
        bool threw = false;
        try { run(source, nullptr); } catch (...) { threw = true; }
        require(threw, "text compile/run failures preserve SqPlus exception policy");
        top(vm, base + 1, "SqPlus exception retains last-error object");
        sq_settop(vm, base);
    }
}
void values(HSQUIRRELVM vm) {
    Top restore(vm); std::array<int32_t,3> object{},copy{};
    const char raw[5]={'a','b','\0','c','d'};
    require(kinoko_squirrel_object_from_string(object.data(),raw,5),"construct from bounded string");
    const char* text=nullptr; require(kinoko_squirrel_object_string(object.data(),&text) && std::string(text)=="ab","embedded NUL retains old string semantics");
    require(kinoko_squirrel_object_copy(copy.data(),object.data()),"construct owned copy");
    require(kinoko_squirrel_object_copy(copy.data(),copy.data()),"self-copy keeps owned value");
    const auto slot=kinoko_push_script_object(vm, copy.data());
    require(slot==kinoko_sq_get_up(vm, -1) && get_string(vm)=="ab","push returns source slot address"); sq_pop(vm,1);
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(object.data())));
    require(kinoko_squirrel_object_string(copy.data(),&text) && std::string(text)=="ab","copy survives source destruction");
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(copy.data())));
    require(kinoko_squirrel_object_from_pair(object.data(),OT_INTEGER,-71),"integer pair construction");
    text=raw; require(!kinoko_squirrel_object_string(object.data(),&text) && text==raw,"failed string conversion leaves output");
    const auto previous=object;
    require(!kinoko_squirrel_object_from_string(object.data(),raw,0x1000001u) && object==previous,"oversized length rejected before read/write");
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(object.data())));
    require(kinoko_squirrel_object_from_string(object.data(),raw,0),"empty string construction");
    require(kinoko_squirrel_object_string(object.data(),&text) && !*text,"zero length is empty"); (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(object.data())));
    require(kinoko_squirrel_object_from_string(object.data(),raw,2),"nonterminated bounded prefix");
    require(kinoko_squirrel_object_string(object.data(),&text) && std::string(text)=="ab","bounded no-overread text"); (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(object.data())));
}
void callback_call(HSQUIRRELVM vm) {
    Top restore(vm); Pair closure(vm), environment(vm); root(vm,environment);
    std::array<unsigned char,28> state{};
    store(state.data(),address(vm)); store(state.data()+8,environment.get());
    std::array<int32_t,3> temporary{};
    for(bool fail : {false,true}) {
        evaluate(vm,fail ? "return function(arg) { throw \"failed\"; };" : "return function(arg) { if (arg != 93) throw \"arg\";\nreturn 77; };",&closure);
        store(state.data()+20,closure.get());
        ObjectView value(temporary.data()); value.initialize(kinoko_squirrel_object_vtable());
        sq_newuserdata(vm,4); sq_setreleasehook(vm,-1,release_userdata); value.capture(vm,-1); sq_pop(vm,1);
        const auto before=releases; const auto base=sq_gettop(vm);
        const auto result=kinoko_script_callback_invoke_owned((KinokoScriptCallback *)(intptr_t)(address(state.data())), (KinokoOwnedObjectWords *)(intptr_t)(address(temporary.data())), OT_INTEGER, 93);
        require(fail ? SQ_FAILED(result) : SQ_SUCCEEDED(result),"callback forwards call status");
        require(releases==before+1 && value.value()._type==OT_NULL,"temporary consumed on either call result");
        top(vm,base+(fail ? 1 : 0),"success pops closure/result; failure retains closure");
        sq_settop(vm,base);
    }
}
}
int main() {
    try {
        for(int pass=0;pass<8;++pass) {
            Machine machine; auto vm=machine.get();
            kinoko_primary_vm=reinterpret_cast<SQVM*>(vm); receiver=address(vm); kinoko_sq_set_context_exchange(exchange_vm);
            constructors(vm); resource_roots(vm); callbacks(vm); embedded(vm); values(vm); callback_call(vm); text_scripts(vm);
            sq_newthread(vm,64); HSQUIRRELVM child=nullptr; sq_getthread(vm,-1,&child);
            embedded(child); callbacks(child);
            require(kinoko_primary_vm==reinterpret_cast<SQVM*>(vm),"child execution restores host VM");
            sq_pop(vm,1); top(vm,0,"all root test operations balanced");
            std::printf("game objects pass %d: construction, bytecode, callback, strings, ownership OK\n",pass+1);
            kinoko_primary_vm=nullptr;
        }
        return 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"FAIL: %s\n",error.what()); return 1; }
}
