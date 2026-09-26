#include "kinoko/scene_operations.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/game_host.h"
#include "kinoko/map_render.h"
#include "kinoko/legacy_memory.hpp"
#include <windows.h>
#include <cstring>
extern "C" {
void kinoko_trace(const char*);
void kinoko_trace_i32(const char*,int32_t);
void kinoko_trace_squirrel_name(const char*,int32_t);
}
namespace {
using namespace kinoko::actor;
using kinoko::legacy::address;
int32_t bits(float value) { int32_t result; std::memcpy(&result,&value,sizeof(result)); return result; }
constexpr const char* layer_names[]={"actor_back","actor_middle","actor_front","actor_water"};
}
extern "C" void kinoko_actor_move_with_camera(KinokoActorManager* manager,KinokoCamera* camera,float dx,float dy) {
    static volatile LONG trace_count;
    const auto trace=InterlockedIncrement(&trace_count);
    if(!manager || !camera) return;
    const ManagerView owner(manager);
    const kinoko::camera::View view(camera);
    if(trace<=16) {
        kinoko_trace_i32("actor:move-dx",bits(dx)); kinoko_trace_i32("actor:move-dy",bits(dy));
        kinoko_trace_i32("actor:move-camera-left-before",bits(view.get(&kinoko::camera::Record::bounds).left));
        kinoko_trace_i32("actor:move-camera-right-before",bits(view.get(&kinoko::camera::Record::bounds).right));
    }
    // 46260C jumps past BOTH actor movement and camera movement for an empty iteration.
    if(!owner.get(&ManagerPrefix::iteration_count)) return;
    auto** actors=owner.get(&ManagerPrefix::iteration).begin;
    for(uint32_t index=0; actors && index<static_cast<uint32_t>(owner.get(&ManagerPrefix::iteration_count)); ++index) {
        if(!actors[index]) continue; // retain the existing null-entry compatibility guard
        const ActorView actor(actors[index]);
        if(!actor.get(&ActorRecord::active) || actor.get(&ActorRecord::registration_flag20) ||
           !actor.get(&ActorRecord::update_group)) continue;
        const auto camera_bounds=view.get(&kinoko::camera::Record::bounds);
        auto bounds=actor.get(&ActorRecord::world_bounds);
        // Original compares the extended x87 sum before storing to float. Its
        // unordered comparisons do not reject the actor; retain exclusion form.
        if(static_cast<double>(camera_bounds.right)+64.0<bounds.left ||
           static_cast<double>(camera_bounds.left)-64.0>bounds.right ||
           static_cast<double>(camera_bounds.bottom)+64.0<bounds.top ||
           static_cast<double>(camera_bounds.top)-64.0>bounds.bottom) continue;
        actor.set(&ActorRecord::x,actor.get(&ActorRecord::x)+dx);
        actor.set(&ActorRecord::y,actor.get(&ActorRecord::y)+dy);
        bounds.left+=dx; bounds.right+=dx; bounds.top+=dy; bounds.bottom+=dy;
        actor.set(&ActorRecord::world_bounds,bounds);
        actor.set(&ActorRecord::previous_x,actor.get(&ActorRecord::x));
        actor.set(&ActorRecord::previous_y,actor.get(&ActorRecord::y));
        actor.set(&ActorRecord::previous_bounds,bounds);
    }
    view.set(&kinoko::camera::Record::x,view.get(&kinoko::camera::Record::x)+dx);
    view.set(&kinoko::camera::Record::y,view.get(&kinoko::camera::Record::y)+dy);
    auto bounds=view.get(&kinoko::camera::Record::bounds);
    bounds.left+=dx; bounds.top+=dy; bounds.right+=dx; bounds.bottom+=dy;
    view.set(&kinoko::camera::Record::bounds,bounds);
    if(trace<=16) {
        kinoko_trace_i32("actor:move-camera-left-after",bits(bounds.left));
        kinoko_trace_i32("actor:move-camera-right-after",bits(bounds.right));
    }
}
extern "C" KinokoRenderLayer* kinoko_actor_render_layer_object(KinokoActorManager* manager,uint32_t index) {
    return reinterpret_cast<KinokoRenderLayer*>(ManagerView(manager).get(&ManagerPrefix::render_layers)[index]);
}
extern "C" void kinoko_actor_trace_render_layers(KinokoActorManager* manager) {
    const char* labels[]={"actor:layer-back","actor:layer-middle","actor:layer-front","actor:layer-water"};
    for(uint32_t i=0;i<4;++i) kinoko_trace_i32(labels[i],address(kinoko_actor_render_layer_object(manager,i)));
}
extern "C" void* kinoko_scene_create_render_layer(const char* name) {
    if(!name) return nullptr; // existing script-entry guard
    const auto& objects=*kinoko_game_objects();
    KinokoRenderLayer* layer=nullptr;
    bool actor_layer=false;
    for(uint32_t index=0;index<4;++index) {
        if(std::strcmp(name,layer_names[index])==0) {
            layer=kinoko_actor_render_layer_object(objects.actors,index);
            actor_layer=true; break;
        }
    }
    if(!actor_layer) layer=kinoko_map_make_render_layer(objects.map,name);
    if(!layer) return nullptr; // existing null-layer compatibility guard
    kinoko_trace_squirrel_name("map:render-order-layer",address(name));
    static volatile LONG trace_count;
    if(InterlockedIncrement(&trace_count)<=16) {
        kinoko_trace("46a210:entry"); kinoko_trace_i32("46a210:value",address(layer));
        kinoko_trace_i32("46a210:g613",(int32_t)(intptr_t)kinoko_render_queue_identity());
    }
    return kinoko_render_queue_append(layer); // append even duplicates, in script order
}
