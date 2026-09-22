#include "kinoko/actor_manager.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_pool_dispatch.hpp"
#include "kinoko/actor_priority.h"
#include "kinoko/native_buffer.h"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace kinoko::actor;
using kinoko::legacy::address;
namespace {
ManagerPrefix manager{};
std::array<ActorRecord,6> actors{};
std::vector<int> steps,motions,renders,retired,phases;
bool mutate=false;
bool initialize_ok=true;
uint32_t next_handle=1;
KinokoActor *actor_at(size_t i) { return reinterpret_cast<KinokoActor *>(&actors[i]); }
KinokoActorManager *receiver() { return reinterpret_cast<KinokoActorManager *>(&manager); }
void require(bool ok) { if (!ok) std::abort(); }
int32_t __fastcall delete_pool(KinokoActorPool *,void *,unsigned char) { return 0; }
KinokoActor *__fastcall acquire_pool(KinokoActorPool *,void *,uint32_t *handle) {
    *handle=next_handle; return actor_at(next_handle++-1);
}
int32_t __fastcall retire_pool(KinokoActorPool *,void *,uint32_t handle) {
    retired.push_back(handle); return 1;
}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"manager line %d: %s\n",__LINE__,#x);return 1; } } while(0)
}
extern "C" {
// Host dependencies are deliberately inert; the scheduler and priority/native
// buffers below are production implementations. Integration remains in stage_contract.
const void *kinoko_actor_owner_methods(void) { return nullptr; }
const void *kinoko_actor_render_layer_methods(void) { return nullptr; }
KinokoActorPool *kinoko_actor_pool_construct(KinokoActorPool *p) { return p; }
void kinoko_actor_owner_list_construct(KinokoActorManager *) {}
uint32_t kinoko_actor_owner_list_size(KinokoActorManager *) { return 512; }
void kinoko_actor_owner_list_clear(KinokoActorManager *) {}
KinokoActor *kinoko_actor_owner_list_acquire(KinokoActorManager *) { return nullptr; }
int32_t kinoko_integer_map_create(void) { return 0; }
void kinoko_animation_list_construct(int32_t) {}
void kinoko_integer_vector_construct(KinokoIntegerVector*) {}
KinokoActor* kinoko_actor_set_init_data(KinokoActor* a,const void*) { return a; }
int32_t kinoko_actor_initialize(KinokoActor *,KinokoActorManager *,const KinokoOwnedObjectWords *,float,float,float,const KinokoOwnedObjectWords *) { return initialize_ok; }
int32_t kinoko_collision_dispatch_all(KinokoActorManager *) { phases.push_back(1);return 0; }
void kinoko_actor_manager_refresh_collision(void) { phases.push_back(2); }
void kinoko_actor_manager_trace_actor(int32_t,KinokoActor *,KinokoCamera *,int32_t) {}
void kinoko_actor_tick(KinokoActor *actor) {
    auto &a=*reinterpret_cast<ActorRecord *>(actor);
    steps.push_back(a.id);
    if (mutate && a.id==0) {
        manager.update_mask=2; // the next actor and motion see the new mask
        actors[2].release_pending=1;
        actors[3].priority=-1;
        require(kinoko_actor_manager_reindex(receiver(),actor_at(3)));
        require(kinoko_actor_manager_reindex(receiver(),actor_at(4))); // new actor, motion only
    }
}
void kinoko_actor_motion_host(KinokoActor *actor) { motions.push_back(reinterpret_cast<ActorRecord *>(actor)->id); }
int32_t kinoko_actor_render_host(KinokoActor *actor,KinokoCamera *) {
    const auto id=reinterpret_cast<ActorRecord *>(actor)->id;
    renders.push_back(id); return id+10;
}
}
int main() {
    // Native thiscall dispatch must remain real virtual dispatch, including
    // the two argument words and callee stack cleanup.
    void *methods[]{reinterpret_cast<void *>(&delete_pool),reinterpret_cast<void *>(&acquire_pool),reinterpret_cast<void *>(&retire_pool)};
    void **pool=methods;
    manager.pool=reinterpret_cast<KinokoActorPool *>(&pool);
    kinoko_priority_construct((void *)(intptr_t)(address(&manager.actors)));
    std::array<RenderLayerRecord,4> layers{};
    for (size_t i=0;i<4;++i) manager.render_layers[i]=&layers[i];
    const int priorities[]{0xffff,-1,7,0x10000,7,0};
    for (size_t i=0;i<actors.size();++i) {
        actors[i].id=static_cast<int>(i);
        actors[i].pool_handle=static_cast<uint32_t>(i+1);
        actors[i].owner_references=1;
        actors[i].priority=priorities[i];
        actors[i].active=1;actors[i].update_group=2;
    }
    for (size_t i=0;i<5;++i) CHECK(kinoko_actor_manager_reindex(receiver(),actor_at(i)));
    CHECK(kinoko_actor_manager_refresh(receiver())==5);
    const int order[]{1,2,4,0,3};
    for (size_t i=0;i<5;++i) CHECK(manager.iteration.begin[i]==actor_at(order[i]));
    CHECK(layers[0].begin==0 && layers[0].end==1);
    CHECK(layers[1].begin==1 && layers[1].end==3);
    CHECK(layers[2].begin==3 && layers[2].end==4);
    CHECK(layers[3].begin==4 && layers[3].end==5);
    CHECK(kinoko_actor_manager_reindex(receiver(),actor_at(2)));
    CHECK(kinoko_actor_manager_refresh(receiver())==5);
    CHECK(manager.iteration.begin[1]==actor_at(4) && manager.iteration.begin[2]==actor_at(2));
    CHECK(kinoko_actor_manager_render_layer(receiver(),nullptr,1)==12);
    CHECK((renders==std::vector<int>{4,2}));
    kinoko_priority_clear((void *)(intptr_t)(address(&manager.actors)));
    for (auto &actor:actors) actor.priority_entry=nullptr;
    for (size_t i=0;i<4;++i) { actors[i].priority=0;CHECK(kinoko_actor_manager_reindex(receiver(),actor_at(i))); }
    actors[4].priority=0;
    actors[0].update_group=1;
    manager.update_mask=1;
    mutate=true;
    CHECK(kinoko_actor_manager_update(receiver(),nullptr)==4);
    CHECK((steps==std::vector<int>{0,1,2,3})); // deferred release until refresh
    CHECK((motions==std::vector<int>{3,1,4}));
    CHECK((retired==std::vector<int>{3}));
    CHECK((phases==std::vector<int>{1,2}));
    CHECK(actors[2].owner_references==1); // deferred retirement is not owner-list decrement
    actors[1].owner_references=2;
    kinoko_actor_manager_reset(receiver());
    CHECK(actors[1].owner_references==1 && manager.iteration_count==0);
    CHECK((retired==std::vector<int>{3,4,1,5}));
    CHECK(kinoko_actor_manager_refresh(receiver())==0 && !manager.cleanup_pending);
    initialize_ok=false;
    KinokoOwnedObjectWords empty{};
    CHECK(!kinoko_actor_manager_create(receiver(),&empty,0,0,0,&empty,nullptr));
    CHECK(retired.back()==1 && retired.size()==5); // 463CB6 failure cleanup
    CameraBoundsRecord camera{};camera.bounds={0,0,10,10};
    actors[5].world_bounds={74,0,75,1};actors[5].active=0;
    CHECK(kinoko_actor_activate(actor_at(5),reinterpret_cast<KinokoCamera *>(&camera),64));
    actors[5].world_bounds.left=74.01f;actors[5].active=0;
    CHECK(!kinoko_actor_activate(actor_at(5),reinterpret_cast<KinokoCamera *>(&camera),64));
    kinoko_priority_destroy((void *)(intptr_t)(address(&manager.actors)));
    kinoko_native_buffer_destroy(address(&manager.iteration));
    kinoko_native_buffer_destroy(address(&manager.callback_candidates));
    std::puts("PASS: priority partitions, stable reinsertion, callback masks, deferred ownership and creation failure");
}
