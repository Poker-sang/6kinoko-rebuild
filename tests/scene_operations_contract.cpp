#include "kinoko/scene_operations.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/game_host.h"
#include "kinoko/map_render.h"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <type_traits>
using namespace kinoko::actor;
using kinoko::legacy::address;
static_assert(std::is_same_v<decltype(ActorRecord::manager),KinokoActorManager*>);
namespace {
ManagerPrefix manager{};
kinoko::camera::Record camera{};
int map_token;
KinokoGameObjects objects{nullptr,reinterpret_cast<KinokoActorManager*>(&manager),reinterpret_cast<KinokoCamera*>(&camera),reinterpret_cast<KinokoMapManager*>(&map_token)};
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"scene operations line %d: %s\n",__LINE__,#x); std::abort(); } } while(0)
struct Layer;
struct Methods { int32_t (__thiscall *draw)(Layer*,KinokoCamera*); };
struct Layer { const Methods* methods; int id; };
std::vector<int> draw_order;
int map_lookups=0;
int32_t __fastcall draw(Layer* layer,void*,KinokoCamera* view) { CHECK(view==objects.camera); draw_order.push_back(layer->id); return 1; }
const Methods methods{reinterpret_cast<decltype(Methods::draw)>(draw)};
std::array<Layer,6> layers{{{&methods,0},{&methods,1},{&methods,2},{&methods,3},{&methods,4},{&methods,5}}};
void movement() {
    camera={}; camera.bounds={0,0,100,100};
    const auto before=camera;
    manager.iteration_count=0;
    kinoko_actor_move_with_camera(objects.actors,objects.camera,5,-7);
    CHECK(std::memcmp(&before,&camera,sizeof(camera))==0);
    std::array<ActorRecord,6> actors{};
    std::array<KinokoActor*,7> pointers{};
    for(size_t i=0;i<actors.size();++i) {
        auto& actor=actors[i]; actor.active=1; actor.update_group=1;
        actor.x=10; actor.y=20; actor.world_bounds={10,20,30,40};
        actor.previous_x=-1; actor.previous_y=-2; actor.previous_bounds={-1,-2,-3,-4};
        pointers[i]=reinterpret_cast<KinokoActor*>(&actor);
    }
    actors[1].active=0; actors[2].registration_flag20=1; actors[3].update_group=0;
    actors[4].world_bounds={164,20,170,40}; // right overscan boundary is inclusive
    actors[5].world_bounds={165,20,170,40}; // beyond the boundary
    const auto original=actors;
    manager.iteration.begin=pointers.data(); manager.iteration_count=7;
    kinoko_actor_move_with_camera(objects.actors,objects.camera,5,-7);
    CHECK(actors[0].x==15 && actors[0].y==13);
    CHECK(actors[0].world_bounds.left==15 && actors[0].world_bounds.right==35);
    CHECK(actors[0].world_bounds.top==13 && actors[0].world_bounds.bottom==33);
    CHECK(actors[0].previous_x==15 && actors[0].previous_y==13);
    CHECK(std::memcmp(&actors[0].previous_bounds,&actors[0].world_bounds,sizeof(Bounds))==0);
    CHECK(actors[4].x==15 && actors[4].world_bounds.left==169);
    for(int i:{1,2,3,5}) CHECK(std::memcmp(&actors[i],&original[i],sizeof(ActorRecord))==0);
    CHECK(camera.x==5 && camera.y==-7 && camera.bounds.left==5 && camera.bounds.top==-7);
    CHECK(camera.bounds.right==105 && camera.bounds.bottom==93);
    // An extended-precision +64 comparison must not first round to float 128.
    camera={}; camera.bounds={0,0,std::nextafter(64.0f,0.0f),100};
    actors[0]=original[0]; actors[0].world_bounds={128,20,130,40};
    manager.iteration_count=1;
    kinoko_actor_move_with_camera(objects.actors,objects.camera,5,0);
    CHECK(actors[0].x==10 && camera.x==5); // nonempty but fully rejected still moves camera
    // Original x87 unordered comparisons pass through the exclusion branches.
    camera.bounds={0,0,NAN,100};
    kinoko_actor_move_with_camera(objects.actors,objects.camera,5,0);
    CHECK(actors[0].x==15);
    const auto unchanged=camera;
    kinoko_actor_move_with_camera(nullptr,objects.camera,1,1);
    kinoko_actor_move_with_camera(objects.actors,nullptr,1,1);
    CHECK(std::memcmp(&unchanged,&camera,sizeof(camera))==0);
}
void rendering() {
    kinoko_initialize_render_queue();
    for(uint32_t i=0;i<4;++i) manager.render_layers[i]=reinterpret_cast<RenderLayerRecord*>(&layers[i]);
    CHECK(kinoko_scene_create_render_layer("actor_back"));
    CHECK(kinoko_scene_create_render_layer("actor_middle"));
    CHECK(kinoko_scene_create_render_layer("actor_front"));
    CHECK(kinoko_scene_create_render_layer("actor_water"));
    CHECK(map_lookups==0);
    const auto first=kinoko_scene_create_render_layer("map_back");
    const auto second=kinoko_scene_create_render_layer("map_back");
    CHECK(first && second && first!=second); // distinct queue nodes, same borrowed object
    manager.render_layers[0]=reinterpret_cast<RenderLayerRecord*>(&layers[5]);
    CHECK(kinoko_scene_create_render_layer("actor_back")); // no stale global alias
    CHECK(!kinoko_scene_create_render_layer("unknown"));
    CHECK(!kinoko_scene_create_render_layer("Actor_Back")); // names are case-sensitive
    CHECK(!kinoko_scene_create_render_layer(nullptr));
    manager.render_layers[1]=nullptr;
    CHECK(!kinoko_scene_create_render_layer("actor_middle"));
    CHECK(map_lookups==4 && kinoko_render_queue_size()==7);
    kinoko_draw_render_queue((struct KinokoCamera*)(uintptr_t)(address(objects.camera)));
    CHECK((draw_order==std::vector<int>{0,1,2,3,4,4,5}));
    kinoko_clear_render_queue(); CHECK(kinoko_render_queue_size()==0);
    for(size_t i=0;i<layers.size();++i) CHECK(layers[i].id==static_cast<int>(i));
}
}
extern "C" {
const KinokoGameObjects* kinoko_game_objects() { return &objects; }
void kinoko_trace(const char*) {}
void kinoko_trace_i32(const char*,int32_t) {}
void kinoko_trace_squirrel_name(const char*,int32_t) {}
KinokoRenderLayer* kinoko_map_make_render_layer(KinokoMapManager* map,const char* name) {
    CHECK(map==objects.map); ++map_lookups;
    return std::strcmp(name,"map_back")==0 ? reinterpret_cast<KinokoRenderLayer*>(&layers[4]) : nullptr;
}
}
int main() { movement(); rendering(); }
