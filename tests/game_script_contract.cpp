#include "kinoko/game_script_api.h"
#include "kinoko/game_script_host.h"
#include "kinoko/game_host.h"
#include "kinoko/scene_operations.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/map_activation.h"
#include "kinoko/map_render.h"
#include "kinoko/pat_animation.h"
#include "kinoko/stage_runtime.h"
#include "kinoko/render_queue.h"
#include "kinoko/squirrel_host_compat.h"
#include "squirrel_bridge_test_support.hpp"
#include <vector>
using namespace bridge_test;
namespace {
HSQUIRRELVM vm;
KinokoScriptCallback callback{};
kinoko::actor::ActorRecord actor{};
kinoko::actor::ManagerPrefix manager{};
int map_token,collision_token,layout_token;
KinokoGameObjects objects{nullptr,reinterpret_cast<KinokoActorManager*>(&manager),nullptr,reinterpret_cast<KinokoMapManager*>(&map_token)};
std::vector<int32_t> releases;
int created=0,map_creations=0,event_creations=0,pat_result=1,map_result=1;
bool fail_actor=false,missing_layer=false;
HSQOBJECT value(const KinokoOwnedObjectWords& object) { return kinoko::script::borrowed_value(object.type,object.value); }
KinokoOwnedObjectWords retain(const HSQOBJECT& incoming) {
    auto result=incoming; sq_addref(vm,&result);
    return {kinoko_squirrel_object_vtable(),result._type,kinoko::script::data_bits(result)};
}
HSQOBJECT borrow(int index) { HSQOBJECT o{}; require(SQ_SUCCEEDED(sq_getstackobj(vm,index,&o)),"borrow stack"); return o; }
void expect_last_release_order() {
    require(releases.size()>=2,"two consumed parameters");
    require(releases[releases.size()-2]==OT_CLOSURE && releases.back()==OT_TABLE,"closure before environment release");
}
}
extern "C" {
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*,int32_t) {}
void retdec_trace_squirrel_name(const char*,int32_t) {}
int32_t kinoko_squirrel_object_vtable() { return 0x12345678; }
void * kinoko_sqplus_object_initialize(void * id) { kinoko::script::ObjectView(id).initialize(kinoko_squirrel_object_vtable()); return id; }
void * kinoko_sqplus_object_copy_construct(void * out, const void * source) {
    kinoko::script::ObjectView(out).initialize(kinoko_squirrel_object_vtable());
    auto v=kinoko::script::ObjectView(source).value(); sq_addref(vm,&v); kinoko::script::ObjectView(out).write(v); return (void *)(intptr_t)(out);
}
void * kinoko_sqplus_object_assign(void * destination, const void * source) {
    const kinoko::script::ObjectView dst((int32_t)(intptr_t)(destination)),src((int32_t)(intptr_t)(source));
    auto incoming=src.value(),old=dst.value(); sq_addref(vm,&incoming); sq_release(vm,&old); dst.write(incoming); return destination;
}
int32_t  kinoko_sqplus_object_destroy(void * id) {
    const kinoko::script::ObjectView object((int32_t)(intptr_t)(id)); auto v=object.value();
    releases.push_back(v._type); sq_release(vm,&v); object.reset(); return (int32_t)(intptr_t)(id);
}
int32_t  kinoko_sqplus_object_raw_set_name(void * table, const char* key, const void * object) {
    kinoko::script::ObjectView(table).push(vm); sq_pushstring(vm,key,-1); kinoko::script::ObjectView(object).push(vm);
    const auto result=sq_newslot(vm,-3,SQFalse); sq_pop(vm,1); return SQ_SUCCEEDED(result);
}
int32_t function_4029b0(int32_t id,int32_t* object) { require(pointer<SQVM>(id)==vm,"result VM"); kinoko::script::ObjectView(object).push(vm); return 1; }
const KinokoGameObjects* kinoko_game_objects() { return &objects; }
SQVM* kinoko_actor_default_vm() { return vm; }
KinokoScriptCallback* kinoko_game_global_callback() { return &callback; }
KinokoCollisionState* kinoko_game_collision_state() { return reinterpret_cast<KinokoCollisionState*>(&collision_token); }
void kinoko_game_initialize_callback(KinokoScriptCallback* result) {
    result->vm=vm; (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(&result->environment))); (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(&result->closure)));
    sq_pushroottable(vm); result->environment=retain(borrow(-1)); sq_pop(vm,1);
}
KinokoActor* kinoko_actor_manager_create(KinokoActorManager* owner,const KinokoOwnedObjectWords* closure,
    float x,float y,float z,const KinokoOwnedObjectWords* argument,const void* initial) {
    require(owner==objects.actors && !initial,"manager and default initial data");
    require(closure->type==OT_CLOSURE && argument->type==OT_TABLE,"borrowed creation values");
    require(x==1.25f && y==-2.5f && z==3.0f,"cdecl floats"); ++created;
    return fail_actor ? nullptr : reinterpret_cast<KinokoActor*>(&actor);
}
void* kinoko_actor_manager_reset(KinokoActorManager* owner) { require(owner==objects.actors,"clear owner"); return owner; }
void kinoko_actor_move_with_camera(KinokoActorManager* owner,KinokoCamera* camera,float,float) { require(owner==objects.actors && camera==objects.camera,"move borrowed objects"); }
void* kinoko_collision_refresh(KinokoCollisionState* state) { require(state==kinoko_game_collision_state(),"refresh state"); return state; }
int32_t kinoko_collision_reset(KinokoCollisionState* state,KinokoActorManager* owner) { require(state==kinoko_game_collision_state() && owner==objects.actors,"collision reset"); return 1; }
KinokoActor* kinoko_collision_register_map(KinokoCollisionState*,KinokoActLayout* layout) { return layout ? reinterpret_cast<KinokoActor*>(&actor) : nullptr; }
KinokoActLayout* kinoko_map_lookup_layout(KinokoMapManager* map,const char*) { require(map==objects.map,"map receiver"); return missing_layer ? nullptr : reinterpret_cast<KinokoActLayout*>(&layout_token); }
int32_t kinoko_map_create_actors(KinokoActorManager*,KinokoActLayout*,const KinokoSquirrelObject* environment) {
    require(kinoko::script::ObjectView(const_cast<KinokoSquirrelObject*>(environment)).value()._type==OT_TABLE,"map environment"); ++map_creations; return 19;
}
int32_t kinoko_map_create_events(KinokoMapManager* map,SQVM* machine,const char*,const KinokoSquirrelObject*,const KinokoSquirrelObject*) {
    require(map==objects.map && machine==vm,"event receiver"); ++event_creations; return 23;
}
void kinoko_game_split_path(const char* path,char* directory) { require(std::string(path)=="data/actor/test.pat","animation path"); strcpy_s(directory,260,"data/actor/"); }
int32_t kinoko_pat_load(KinokoActorManager* owner,const char*,const char* directory) { require(owner==objects.actors && std::string(directory)=="data/actor/","PAT host arguments"); return pat_result; }
KinokoStageOwner* kinoko_stage_load(const char*) { return nullptr; }
int32_t kinoko_game_load_map_file(const char*) { return map_result; }
int32_t kinoko_game_release_map_state() { return 1; }
int32_t kinoko_clear_render_queue() { return 0; }
void* kinoko_scene_create_render_layer(const char*) { return &layout_token; }
}
namespace {
void register_entry(const char* name,int32_t (*entry)(SQVM*),const void* target) {
    sq_pushroottable(vm); sq_pushstring(vm,name,-1);
    const auto id=address(target); std::memcpy(sq_newuserdata(vm,4),&id,4);
    sq_newclosure(vm,entry,1); sq_newslot(vm,-3,SQFalse); sq_pop(vm,1);
}
void api_contract() {
    require(SQ_SUCCEEDED(sq_compilebuffer(vm,"return 1;",9,"closure",SQFalse)),"compile fixture");
    sq_newtable(vm);
    const auto fn=borrow(-2),env=borrow(-1);
    kinoko_game_initialize_callback(&callback);
    kinoko_script_set_global_update(retain(fn),retain(env));
    require(callback.vm==vm && callback.closure.value==kinoko::script::data_bits(fn),"callback fields");
    expect_last_release_order();
    // Self replacement must acquire before releasing old callback references.
    kinoko_script_set_global_update(retain(value(callback.closure)),retain(value(callback.environment)));
    require(callback.closure.type==OT_CLOSURE,"self replacement");
    kinoko_script_set_init(0x2a,retain(fn),retain(env)); expect_last_release_order();
    sq_pushobject(vm,env); sq_pushstring(vm,"Init002a",-1);
    require(SQ_SUCCEEDED(sq_get(vm,-2)) && sq_gettype(vm,-1)==OT_CLOSURE,"Init hexadecimal name"); sq_pop(vm,2);
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(actor.script_object.data())));
    auto actor_value=retain(env); std::memcpy(actor.script_object.data(),&actor_value,sizeof(actor_value));
    KinokoOwnedObjectWords result{};
    kinoko_script_create_actor(&result,retain(fn),1.25f,-2.5f,3.0f,retain(env));
    require(result.type==OT_TABLE && created==1,"successful actor reference"); expect_last_release_order();
    kinoko_sqplus_object_destroy((void *)(&result));
    fail_actor=true;
    kinoko_script_create_actor(&result,retain(fn),1.25f,-2.5f,3.0f,retain(env));
    require(result.type==OT_NULL && created==2,"failed actor returns null"); expect_last_release_order();
    fail_actor=false;
    require(kinoko_script_create_map_actors("layer",retain(env))==19 && map_creations==1,"map actor dispatch");
    missing_layer=true;
    require(kinoko_script_create_map_actors("missing",retain(env))==0 && map_creations==1,"missing layer consumes argument");
    missing_layer=false;
    require(kinoko_script_create_event("event",retain(fn),retain(env))==23 && event_creations==1,"event dispatch"); expect_last_release_order();
    pat_result=1; require(kinoko_script_load_animation("data/actor/test.pat")==1,"PAT success result");
    pat_result=0; require(kinoko_script_load_animation("data/actor/test.pat")==0,"PAT failure result");
    map_result=0x100; require(!kinoko_script_load_map("map"),"map tests low byte");
    map_result=0x101; require(kinoko_script_load_map("map")==1,"map true normalized");
    register_entry("create",kinoko_script_create_actor_entry,reinterpret_cast<const void*>(kinoko_script_create_actor));
    register_entry("setUpdate",kinoko_script_global_update_entry,reinterpret_cast<const void*>(kinoko_script_set_global_update));
    evaluate(vm,"local f=function() {}; local e={}; local a=create(f,1.25,-2.5,3.0,e); if(typeof a!=\"table\") throw \"actor result\"; setUpdate(f,e);");
    require(created==3,"actual VM native adapter");
    evaluate(vm,"setUpdate(null, {});");
    require(callback.closure.type==OT_NULL && callback.environment.type==OT_TABLE,"non closure restores default environment");
    kinoko_sqplus_object_destroy((void *)(&callback.closure)); kinoko_sqplus_object_destroy((void *)(&callback.environment));
    kinoko_sqplus_object_destroy((void *)(actor.script_object.data())); sq_pop(vm,2);
    top(vm,0,"balanced API and adapter frames");
}
}
int main() {
    try { Machine machine; vm=machine.get(); api_contract(); return 0; }
    catch(const std::exception& e) { std::fprintf(stderr,"game script: %s\n",e.what()); return 1; }
}
