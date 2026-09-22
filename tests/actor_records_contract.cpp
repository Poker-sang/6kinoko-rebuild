#include "kinoko/texture_store.h"
#include "kinoko/game_host.h"
#include "kinoko/game_script_host.h"
#include "kinoko/integer_vector.h"
#include "kinoko/animation_storage.h"
#include "kinoko/actor_priority.h"
#include "kinoko/integer_map.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace kinoko::actor;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
namespace {
int32_t lookup_result = 0;
int32_t chip_x = 0, chip_y = 0, chip_layer = 0, chip_cache = 0;
int priority_updates = 0, object_releases = 0;
std::vector<int32_t> cleanup_order;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "actor model line %d: %s\n", __LINE__, #x); return 1; } } while (0)
void require(bool condition) { if (!condition) std::abort(); }
}
extern "C" {
int32_t g23 = 0x12345678;
static KinokoGameObjects objects{};
const KinokoGameObjects* kinoko_game_objects() { return &objects; }
KinokoCollisionState* kinoko_game_collision_state() { return nullptr; }
int32_t kinoko_actor_manager_reindex(KinokoActorManager* manager,KinokoActor* actor) { require(manager==objects.actors); ++priority_updates; return address(actor); }
int32_t kinoko_collision_dispatch_actor(KinokoActorManager*,KinokoActor* actor) { return address(actor); }
uint32_t kinoko_collision_chip_flags(KinokoCollisionState*,KinokoActor* actor) { return address(actor); }
int32_t kinoko_collision_has_chip(KinokoCollisionState*,KinokoActor* actor,float,float,float,float) { return address(actor); }
int32_t kinoko_collision_event_at_point(KinokoMapManager*,int32_t x,int32_t y,uint32_t layer,int32_t* cache) {
    chip_x=x; chip_y=y; chip_layer=layer; chip_cache=address(cache); return 17;
}
void * kinoko_sqplus_object_instance(void * , void * ) { return (void *)(intptr_t)(0); }
void*  kinoko_sqplus_object_destroy(void * object) { ++object_releases; return (void*)(intptr_t)((int32_t)(intptr_t)(object) + 4); }
void retdec_trace_star_state(const char*, int32_t) {}
int32_t kinoko_texture_release(int32_t handle) { cleanup_order.push_back(handle); return 0; }
void *kinoko_actor_manager_clear_actors(KinokoActorManager *) { cleanup_order.push_back(-1); return nullptr; }
}

int main() {
    std::array<unsigned char, sizeof(ActorRecord) + 2> bytes;
    bytes.fill(0xa7);
    const int32_t actor_address = address(bytes.data() + 1);
    const ActorView actor(bytes.data() + 1);
    actor.clear();
    ManagerPrefix manager{};
    const ManagerView manager_view(&manager);
    const auto lookup = manager_view.view(&ManagerPrefix::animation_lookup);
    lookup.set(&TreeIndex::head,static_cast<Address>(kinoko_integer_map_create()));
    ManagerPrefix global_manager{};
    objects.actors=reinterpret_cast<KinokoActorManager*>(&global_manager);
    actor.set(&ActorRecord::manager, reinterpret_cast<KinokoActorManager*>(&manager));
    actor.set(&ActorRecord::x, 10.25f); actor.set(&ActorRecord::y, 20.75f);
    actor.set(&ActorRecord::scale, 2.0f);
    actor.set(&ActorRecord::scale_x, 0.5f); actor.set(&ActorRecord::scale_y, 1.5f);
    std::array<FrameRecord, 2> frames{};
    frames[0].duration = 2; frames[1].duration = 3;
    AnimationRecord animation{};
    animation.frames_begin = reinterpret_cast<KinokoAnimationFrame *>(frames.data());
    animation.frames_end = reinterpret_cast<KinokoAnimationFrame *>(frames.data()+frames.size());
    animation.left = 1; animation.top = 2; animation.right = 5; animation.bottom = 8;
    animation.duration_total = 0x1234; animation.has_bounds = 1;
    kinoko_integer_map_put(lookup.get(&TreeIndex::head),37,address(&animation));
    CHECK(kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor_address), 37) == address(frames.data()));
    CHECK(actor.get(&ActorRecord::take) == 37);
    const auto local = actor.get(&ActorRecord::local_bounds);
    const auto world = actor.get(&ActorRecord::world_bounds);
    CHECK(local.left == 0.5f && local.top == 2 && local.right == 5.5f && local.bottom == 9);
    CHECK(world.left == 10.75f && world.right == 15.75f && world.top == 26.75f && world.bottom == 47.75f);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::width) == 5);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::height) == 21);
    kinoko_actor_advance_animation((KinokoActor *)(intptr_t)(actor_address), 36);
    CHECK(actor.get(&ActorRecord::frame_time) == 0);
    kinoko_actor_advance_animation((KinokoActor *)(intptr_t)(actor_address), 37);
    CHECK(actor.get(&ActorRecord::frame_time) == 1 && actor.get(&ActorRecord::frame_index) == 0);
    kinoko_actor_advance_animation((KinokoActor *)(intptr_t)(actor_address), 37);
    CHECK(actor.get(&ActorRecord::frame_index) == 1 && actor.get(&ActorRecord::frame_time) == 0);
    CHECK(actor.get(&ActorRecord::sprite_frame) == reinterpret_cast<KinokoAnimationFrame *>(frames.data()+1));
    for (int i = 0; i < 3; ++i) kinoko_actor_advance_animation((KinokoActor *)(intptr_t)(actor_address), 37);
    CHECK(actor.get(&ActorRecord::frame_index) == 1 && actor.get(&ActorRecord::frame_time) == 0);
    animation.loops = 1;
    for (int i = 0; i < 3; ++i) kinoko_actor_advance_animation((KinokoActor *)(intptr_t)(actor_address), 37);
    CHECK(actor.get(&ActorRecord::frame_index) == 0 && actor.get(&ActorRecord::current_frame) == animation.frames_begin);
    // The flipped take must resolve before its bounds can be recomputed.
    // Keep the genuinely missing take (999) below as a separate contract.
    kinoko_integer_map_put(lookup.get(&TreeIndex::head), 38, address(&animation));
    const auto flipped_slot = kinoko_integer_map_find(lookup.get(&TreeIndex::head), 38);
    CHECK(flipped_slot != static_cast<int32_t>(lookup.get(&TreeIndex::head)));
    CHECK(*pointer<int32_t>(flipped_slot) == address(&animation));
    actor.set(&ActorRecord::direction, 1.0f);
    kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor_address), 38);
    CHECK(actor.get(&ActorRecord::world_bounds).left == 4.75f);
    CHECK(actor.get(&ActorRecord::world_bounds).right == 9.75f);
    lookup_result = static_cast<int32_t>(lookup.get(&TreeIndex::head));
    const auto previous_animation = actor.get(&ActorRecord::animation);
    CHECK(kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor_address), 999) == lookup_result);
    CHECK(actor.get(&ActorRecord::take) == 999 && actor.get(&ActorRecord::frame_time) == 0);
    CHECK(actor.get(&ActorRecord::animation) == previous_animation);
    ActorRecord source{}; source.frame_index = 100; source.frame_time = 71;
    kinoko_actor_sync_animation_state((KinokoActor *)(intptr_t)(actor_address), (KinokoActor *)(intptr_t)(address(&source)));
    CHECK(actor.get(&ActorRecord::frame_index) == 1 && actor.get(&ActorRecord::frame_time) == 71);
    source.frame_index = -1;
    kinoko_actor_sync_animation_state((KinokoActor *)(intptr_t)(actor_address), (KinokoActor *)(intptr_t)(address(&source)));
    CHECK(actor.get(&ActorRecord::frame_index) == -1);
    CHECK(actor.get(&ActorRecord::current_frame) == pointer<KinokoAnimationFrame>(address(frames.data())-sizeof(FrameRecord)));
    CHECK(kinoko_actor_set_chip_flags(pointer<KinokoActor>(actor_address), nullptr, -1) == -1);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::chip_flags) == -1);
    CHECK(kinoko_actor_set_chip_bound_type(pointer<KinokoActor>(actor_address), nullptr, 0xffff) == 0xffff);
    CHECK(kinoko_actor_get_chip_id(pointer<KinokoActor>(actor_address), nullptr, 2) == 17);
    CHECK(chip_x == 10 && chip_y == 20 && chip_layer == 2);
    CHECK(chip_cache == actor_address + 512 + 2 * sizeof(int32_t));
    CHECK(kinoko_actor_reset_priority(pointer<KinokoActor>(actor_address), 42) == actor_address && priority_updates == 1);
    CHECK(actor.get(&ActorRecord::priority) == 42);
    CHECK(kinoko_actor_release(pointer<KinokoActor>(actor_address), nullptr) == 1);
    CHECK(actor.get(&ActorRecord::release_pending) && manager.cleanup_pending);
    CHECK(!global_manager.cleanup_pending); // Release uses actor owner; ResetPriority uses the global manager.
    InitialData initial{}; initial.width = 123; initial.chip_flags = -17;
    CHECK(kinoko_actor_set_init_data(pointer<KinokoActor>(actor_address), &initial) == pointer<KinokoActor>(actor_address));
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::width) == 123);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::chip_flags) == -17);
    CHECK(bytes.front() == 0xa7 && bytes.back() == 0xa7);

    kinoko_priority_construct((void *)(intptr_t)(address(&manager.actors)));
    kinoko_animation_list_construct(address(&manager.animations));
    std::array<int32_t, 2> textures{27, 81};
    kinoko_integer_vector_construct((KinokoIntegerVector*)(&manager.textures));
    for(auto handle:textures) kinoko_integer_vector_append((KinokoIntegerVector*)(&manager.textures), handle);
    std::array<KinokoActor *, 32> iteration_storage{};
    int iteration_owner_token = 0; // cleanup retains, never dereferences the owner
    manager.iteration.begin = iteration_storage.data();
    manager.iteration.end = iteration_storage.data() + 1;
    manager.iteration.storage_owner = &iteration_owner_token;
    CHECK((int32_t)(intptr_t)(kinoko_actor_manager_clear_resources((KinokoActorManager *)(intptr_t)(address(&manager)))) == address(iteration_storage.data()));
    CHECK((cleanup_order == std::vector<int32_t>{27, 81, -1}));
    CHECK(kinoko_integer_vector_size((KinokoIntegerVector*)(&manager.textures))==0);
    CHECK(manager.iteration.end == manager.iteration.begin && manager.iteration.storage_owner == &iteration_owner_token);
    CHECK(!manager.cleanup_pending && manager.animations.count==0);
    CHECK(kinoko_integer_map_size(manager.animation_lookup.head)==0 && manager.actors.count==0);
    kinoko_integer_vector_destroy((KinokoIntegerVector*)(&manager.textures));
    kinoko_animation_list_destroy(address(&manager.animations));
    kinoko_integer_map_destroy(manager.animation_lookup.head);
    kinoko_priority_destroy((void *)(intptr_t)(address(&manager.actors)));
    std::puts("PASS: typed Actor animation, original bounds/timing/clamps, deferred ownership and cleanup order");
}
