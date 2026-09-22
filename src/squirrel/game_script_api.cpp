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
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"
#include <windows.h>
#include <cstdio>
extern "C" {
void retdec_trace(const char*);
void retdec_trace_i32(const char*,int32_t);
void retdec_trace_squirrel_name(const char*,int32_t);
}
namespace {
using namespace kinoko::script;
// Adopt only references transferred through the public by-value ABI. Reverse
// declaration order releases closure before environment, as in the original.
struct Reference {
    KinokoOwnedObjectWords *object;
    ~Reference() { kinoko_sqplus_object_destroy((void *)(object)); }
    Reference(const Reference&)=delete;
    explicit Reference(KinokoOwnedObjectWords& value):object(&value) {}
};
void assign(void* destination,const void* source) {
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(destination), (const void *)(source)));
}
void copy(KinokoOwnedObjectWords& destination,const KinokoOwnedObjectWords& source) {
    (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(reinterpret_cast<int32_t*>(&destination)), (const void *)(&source)));
}
}
extern "C" int32_t kinoko_script_load_animation(const char* path) {
    char directory[260]{};
    static volatile LONG trace_count;
    const auto trace=InterlockedIncrement(&trace_count);
    if(trace<=32) { retdec_trace_i32("actor:load-animation-entry",address(path)); retdec_trace_squirrel_name("actor:load-animation-path",address(path)); }
    kinoko_game_split_path(path,directory);
    retdec_trace_i32("464f80:path",address(path));
    retdec_trace_squirrel_name("464f80:path-text",address(path));
    retdec_trace_i32("464f80:directory",address(directory));
    const auto result=kinoko_pat_load(kinoko_game_objects()->actors,path,directory);
    if(trace<=32) retdec_trace_i32("actor:load-animation-result",result);
    return result; // 4696ED returns the PAT result; old reconstruction returned zero.
}
extern "C" void* kinoko_script_clear_actors() { return kinoko_actor_manager_reset(kinoko_game_objects()->actors); }
extern "C" int32_t kinoko_script_move_actors(float dx,float dy) {
    const auto& objects=*kinoko_game_objects();
    kinoko_actor_move_with_camera(objects.actors,objects.camera,dx,dy); return 0;
}
extern "C" void* kinoko_script_refresh_collision() { return kinoko_collision_refresh(kinoko_game_collision_state()); }
extern "C" int32_t kinoko_script_clear_collision() { return kinoko_collision_reset(kinoko_game_collision_state(),kinoko_game_objects()->actors); }
extern "C" KinokoActor* kinoko_script_create_collision(const char* name) {
    return kinoko_collision_register_map(kinoko_game_collision_state(),kinoko_map_lookup_layout(kinoko_game_objects()->map,name));
}
extern "C" int32_t kinoko_script_load_act(const char* path) { return kinoko_stage_load(path)!=nullptr; }
extern "C" int32_t kinoko_script_load_map(const char* path) {
    static int32_t trace_count;
    if(trace_count<32) { retdec_trace("469840:LoadMap-entry"); retdec_trace_squirrel_name("469840:path",address(path)); }
    const auto result=kinoko_game_load_map_file(path);
    if(trace_count<32) { retdec_trace_i32("469840:LoadMap-result",result); ++trace_count; }
    return static_cast<uint8_t>(result)!=0; // 469856 tests AL, not the complete word.
}
extern "C" int32_t kinoko_script_release_map() { return kinoko_game_release_map_state(); }
extern "C" int32_t kinoko_script_clear_render_layers() { return kinoko_clear_render_queue(); }
extern "C" void* kinoko_script_create_render_layer(const char* name) { return kinoko_scene_create_render_layer(name); }

extern "C" int32_t kinoko_script_set_global_update(KinokoOwnedObjectWords closure,KinokoOwnedObjectWords environment) {
    Reference environment_owner(environment),closure_owner(closure);
    const kinoko::native::RecordView<KinokoScriptCallback> state(kinoko_game_global_callback());
    static volatile LONG trace_count;
    if(InterlockedIncrement(&trace_count)<=8) {
        retdec_trace("stagevm:set-global-update");
        retdec_trace_i32("stagevm:set-global-first-type",closure.type);
        retdec_trace_i32("stagevm:set-global-first-data",closure.value);
        retdec_trace_i32("stagevm:set-global-second-type",environment.type);
        retdec_trace_i32("stagevm:set-global-second-data",environment.value);
    }
    {
        KinokoScriptCallback replacement{};
        Reference temporary_environment(replacement.environment),temporary_closure(replacement.closure);
        if(closure.type==OT_CLOSURE) {
            replacement.vm=kinoko_actor_default_vm();
            copy(replacement.environment,environment);
            copy(replacement.closure,closure);
        } else kinoko_game_initialize_callback(&replacement);
        state.set(&KinokoScriptCallback::vm,replacement.vm);
        assign(state.bytes(&KinokoScriptCallback::environment),&replacement.environment);
        assign(state.bytes(&KinokoScriptCallback::closure),&replacement.closure);
    }
    if(trace_count<=8) {
        const auto env=state.get(&KinokoScriptCallback::environment),fn=state.get(&KinokoScriptCallback::closure);
        retdec_trace_i32("stagevm:set-global-vm",address(state.get(&KinokoScriptCallback::vm)));
        retdec_trace_i32("stagevm:set-global-env-type",env.type); retdec_trace_i32("stagevm:set-global-env-data",env.value);
        retdec_trace_i32("stagevm:set-global-func-type",fn.type); retdec_trace_i32("stagevm:set-global-func-data",fn.value);
    }
    return 0;
}
extern "C" int32_t kinoko_script_set_init(int32_t id,KinokoOwnedObjectWords closure,KinokoOwnedObjectWords environment) {
    Reference environment_owner(environment),closure_owner(closure);
    int32_t result=0;
    if(closure.type==OT_CLOSURE && environment.type==OT_TABLE) {
        char name[256]; sprintf_s(name,sizeof(name),"Init%04x",static_cast<unsigned>(id));
        result=kinoko_sqplus_object_raw_set_name((void *)(&environment), name, (const void *)(&closure));
        retdec_trace_i32("actor:init-registration-id",id); retdec_trace_i32("actor:init-registration-result",result);
    }
    return result;
}
extern "C" KinokoOwnedObjectWords* kinoko_script_create_actor(KinokoOwnedObjectWords* result,
    KinokoOwnedObjectWords closure,float x,float y,float z,KinokoOwnedObjectWords argument) {
    Reference argument_owner(argument),closure_owner(closure);
    if(!result) return nullptr;
    auto* manager=kinoko_game_objects()->actors;
    static volatile LONG trace_count;
    const auto trace=InterlockedIncrement(&trace_count);
    if(trace<=32) {
        retdec_trace_i32("actor-create:result-object",address(result)); retdec_trace_i32("actor-create:manager-state",address(manager));
        retdec_trace_i32("actor-create:first-type",closure.type); retdec_trace_i32("actor-create:first-data",closure.value);
        retdec_trace_i32("actor-create:second-type",argument.type); retdec_trace_i32("actor-create:second-data",argument.value);
    }
    if(!closure.vtable) closure.vtable=kinoko_squirrel_object_vtable();
    if(!argument.vtable) argument.vtable=kinoko_squirrel_object_vtable();
    // Native manager borrows these values throughout initialization. The owning
    // by-value parameters remain alive until after result's reference is acquired.
    auto* actor=kinoko_actor_manager_create(manager,&closure,x,y,z,&argument,nullptr);
    if(trace<=32) retdec_trace_i32("actor-create:callback-result",address(actor));
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(result)));
    if(actor) assign(result,kinoko::actor::ActorView(actor).bytes(&kinoko::actor::ActorRecord::script_object));
    return result;
}
extern "C" int32_t kinoko_script_create_map_actors(const char* name,KinokoOwnedObjectWords environment) {
    Reference owner(environment);
    const auto& objects=*kinoko_game_objects();
    auto* layout=kinoko_map_lookup_layout(objects.map,name);
    retdec_trace_squirrel_name("actor:map-layer",address(name)); retdec_trace_i32("actor:map-layout",address(layout));
    return layout ? kinoko_map_create_actors(objects.actors,layout,reinterpret_cast<const KinokoSquirrelObject*>(&environment)) : 0;
}
extern "C" int32_t kinoko_script_create_event(const char* name,KinokoOwnedObjectWords closure,KinokoOwnedObjectWords environment) {
    Reference environment_owner(environment),closure_owner(closure);
    return kinoko_map_create_events(kinoko_game_objects()->map,kinoko_actor_default_vm(),name,
        reinterpret_cast<const KinokoSquirrelObject*>(&closure),reinterpret_cast<const KinokoSquirrelObject*>(&environment));
}
