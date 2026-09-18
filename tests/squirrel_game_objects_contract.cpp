#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/sqrat_object_bridge.h"
#include "squirrel_bridge_test_support.hpp"
#include <cstdlib>
#include <vector>

extern "C" {
char* g644 = nullptr;
char g560 = 0;
int32_t kinoko_squirrel_object_vtable(void) { return 0x12345678; }
int32_t kinoko_sqrat_object_vtable(void) { return 0x12121212; }
int32_t kinoko_sqrat_root_vtable(void) { return 0x34343434; }
int32_t kinoko_native_void_type(void) { return 0x13572468; }
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
void _3f__3f_3_40_YAXPAX_40_Z(int32_t* value) { std::free(value); }
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
    const auto previous=address(g644); g644=pointer<char>(value); receiver=value; return previous;
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
    Pair type(vm), parent(vm), bound(vm), unbound(vm), wrong(vm);
    evaluate(vm,"return Bound;",&type); root(vm,parent);
    const auto base=sq_gettop(vm), before=releases;
    require(retdec_create_bound_instance(address(vm),parent.data(),"bound",type.data(),address(&native),bound.data())==1,"create/publish instance");
    require(retdec_create_unbound_instance(address(vm),type.data(),address(&native),unbound.data())==1,"create unbound instance");
    top(vm,base,"instance creation balances stack");
    evaluate(vm,"if (ctor_calls != 0) throw \"constructor executed\";\nif (typeof bound != \"instance\") throw \"published type\";\n");
    for (auto* object : {&bound,&unbound}) {
        object->push(); SQUserPointer value=nullptr;
        require(SQ_SUCCEEDED(sq_getinstanceup(vm,-1,&value,nullptr)) && value==&native,"instance pointer");
        sq_setreleasehook(vm,-1,release_native); sq_pop(vm,1);
    }
    const auto unchanged=std::array<int32_t,2>{17,29}; auto output=unchanged;
    require(!retdec_create_unbound_instance(address(vm),wrong.data(),address(&native),output.data()) && output==unchanged,"invalid class preserves outputs");
    require(!retdec_create_bound_instance(address(vm),wrong.data(),"bad",type.data(),address(&native),output.data()),"failed publication");
    require(output[0]==OT_NULL && output[1]==0,"failed publication resets temporary handle");
    top(vm,base,"failed publication balances stack");
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
    require(retdec_bind_act_resource_root(address(resource),address(vm),table.data())==1,"bind resource root");
    destroy(table); // The resource is now the table's only strong owner.
    require(retdec_bind_act_resource_root(address(resource),address(vm),reinterpret_cast<int32_t*>(resource+156))==1,"self-rebinding retains last root");
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
    require(retdec_sqrat_set_pair(address(vm),table.data(),"Update",closure.data()),"publish callback");
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
    retdec_copy_act_callback(address(vm),address(script.data()),offset,address(object.data()),"Update");
    require(load<int32_t>(script.data()+offset)==address(vm),"callback stores actual VM");
    require(data_bits(load<HSQOBJECT>(script.data()+offset+4))==data_bits(environment),"callback environment");
    require(data_bits(load<HSQOBJECT>(script.data()+offset+12))==data_bits(function),"callback closure");
    require(has_extra_handle(environment) && has_extra_handle(function),"callback owns both pairs independently");
    retdec_copy_act_callback(address(vm),address(script.data()),offset,address(object.data()),"Update");
    require(has_extra_handle(environment) && has_extra_handle(function),"callback replacement balances handles");
    retdec_copy_act_callback(address(vm),address(script.data()),offset,address(object.data()),"Missing");
    require(load<HSQOBJECT>(script.data()+offset+4)._type==OT_NULL && load<HSQOBJECT>(script.data()+offset+12)._type==OT_NULL,"missing callback clears old pairs");
    require(!has_extra_handle(environment) && !has_extra_handle(function),"missing lookup releases callback references");
    destroy(table); destroy(closure);
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
    require(retdec_execute_embedded_act_script(address(vm),address(script.data()),environment.data())==1,"load and execute supplied bytecode");
    top(vm,base,"embedded execution restores stack"); require(get_integer(vm)==812,"embedded sentinel survives");
    environment.push(); sq_pushstring(vm,"answer",-1); require(SQ_SUCCEEDED(sq_get(vm,-2)) && get_integer(vm)==713,"uses supplied environment, not root"); sq_pop(vm,2);
    for(int32_t truncated : {0,1,2,7,static_cast<int32_t>(data.size()-1)}) {
        store(script.data()+96,truncated);
        require(retdec_execute_embedded_act_script(address(vm),address(script.data()),environment.data())==0,"truncated bytecode rejected");
        top(vm,base,"failed read restores stack");
    }
    auto throwing=bytecode(vm,"throw \"embedded failure\";\n");
    store(script.data()+92,address(throwing.data())); store(script.data()+96,static_cast<int32_t>(throwing.size()));
    require(!retdec_execute_embedded_act_script(address(vm),address(script.data()),environment.data()),"script execution failure reported"); top(vm,base,"throwing closure cleanup");
    unsigned char raw[5]={1,2,3,4,5}, result[9]; std::memset(result,0xa7,sizeof(result));
    std::array<int32_t,3> stream{address(raw),5,address(raw)};
    require(function_402a50(address(stream.data()),address(result+1),3)==3,"partial stream read");
    require(function_402a50(address(stream.data()),address(result+4),9)==2,"clamped final stream read");
    require(function_402a50(address(stream.data()),address(result+6),1)==0,"stream EOF");
    require(result[0]==0xa7 && result[6]==0xa7 && std::memcmp(result+1,raw,5)==0,"read length and canaries");
    const auto exhausted=stream;
    require(function_402a50(address(stream.data()),address(result),-1)==0 && stream==exhausted,"negative request is nonmutating");
}
void values(HSQUIRRELVM vm) {
    Top restore(vm); std::array<int32_t,3> object{},copy{};
    const char raw[5]={'a','b','\0','c','d'};
    require(retdec_squirrel_object_from_string(object.data(),raw,5),"construct from bounded string");
    const char* text=nullptr; require(retdec_squirrel_object_string(object.data(),&text) && std::string(text)=="ab","embedded NUL retains old string semantics");
    require(retdec_squirrel_object_copy(copy.data(),object.data()),"construct owned copy");
    require(retdec_squirrel_object_copy(copy.data(),copy.data()),"self-copy keeps owned value");
    const auto slot=function_4029b0(address(vm),copy.data());
    require(slot==kinoko_sq_get_up(address(vm),-1) && get_string(vm)=="ab","push returns source slot address"); sq_pop(vm,1);
    function_4a9d70_this(address(object.data()));
    require(retdec_squirrel_object_string(copy.data(),&text) && std::string(text)=="ab","copy survives source destruction");
    function_4a9d70_this(address(copy.data()));
    require(retdec_squirrel_object_from_pair(object.data(),OT_INTEGER,-71),"integer pair construction");
    text=raw; require(!retdec_squirrel_object_string(object.data(),&text) && text==raw,"failed string conversion leaves output");
    const auto previous=object;
    require(!retdec_squirrel_object_from_string(object.data(),raw,0x1000001u) && object==previous,"oversized length rejected before read/write");
    function_4a9d70_this(address(object.data()));
    require(retdec_squirrel_object_from_string(object.data(),raw,0),"empty string construction");
    require(retdec_squirrel_object_string(object.data(),&text) && !*text,"zero length is empty"); function_4a9d70_this(address(object.data()));
    require(retdec_squirrel_object_from_string(object.data(),raw,2),"nonterminated bounded prefix");
    require(retdec_squirrel_object_string(object.data(),&text) && std::string(text)=="ab","bounded no-overread text"); function_4a9d70_this(address(object.data()));
}
void callback_call(HSQUIRRELVM vm) {
    Top restore(vm); Pair closure(vm), environment(vm); root(vm,environment);
    std::array<unsigned char,28> state{};
    store(state.data(),address(vm)); store(state.data()+8,environment.get());
    std::array<int32_t,3> temporary{};
    for(bool fail : {false,true}) {
        evaluate(vm,fail ? "return function(arg) { throw \"failed\"; };" : "return function(arg) { if (arg != 93) throw \"arg\"; return 77; };",&closure);
        store(state.data()+20,closure.get());
        ObjectView value(temporary.data()); value.initialize(kinoko_squirrel_object_vtable());
        sq_newuserdata(vm,4); sq_setreleasehook(vm,-1,release_userdata); value.capture(vm,-1); sq_pop(vm,1);
        const auto before=releases, base=sq_gettop(vm);
        const auto result=function_45e020_this(address(state.data()),address(temporary.data()),OT_INTEGER,93);
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
            g644=reinterpret_cast<char*>(vm); receiver=address(vm); kinoko_sq_set_context_exchange(exchange_vm);
            constructors(vm); resource_roots(vm); callbacks(vm); embedded(vm); values(vm); callback_call(vm);
            sq_newthread(vm,64); HSQUIRRELVM child=nullptr; sq_getthread(vm,-1,&child);
            embedded(child); callbacks(child);
            require(g644==reinterpret_cast<char*>(vm),"child execution restores host VM");
            sq_pop(vm,1); top(vm,0,"all root test operations balanced");
            std::printf("game objects pass %d: construction, bytecode, callback, strings, ownership OK\n",pass+1);
            g644=nullptr;
        }
        return 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"FAIL: %s\n",error.what()); return 1; }
}
