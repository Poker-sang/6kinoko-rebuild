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
std::array<int32_t, 6> lookup_node{};
int32_t lookup_result = 0, expected_tree = 0, looked_up_key = 0;
int32_t chip_x = 0, chip_y = 0, chip_layer = 0, chip_cache = 0;
int priority_updates = 0, object_releases = 0;
std::vector<int32_t> cleanup_order;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "actor model line %d: %s\n", __LINE__, #x); return 1; } } while (0)
void require(bool condition) { if (!condition) std::abort(); }
}
extern "C" {
int32_t g23 = 0x12345678;
int32_t function_4706c0_this(int32_t tree, int32_t* entry, int32_t* key) {
    require(tree == expected_tree); looked_up_key = *key; *entry = lookup_result; return 0;
}
int32_t function_469780(int32_t actor, int32_t same) { require(actor == same); ++priority_updates; return actor; }
int32_t function_4697a0(int32_t actor) { return actor; }
int32_t function_4697c0(int32_t actor) { return actor; }
int32_t function_4697e0(int32_t actor, float, float, float, float) { return actor; }
int32_t function_4698a0(int32_t x, int32_t y, int32_t layer, int32_t cache) {
    chip_x = x; chip_y = y; chip_layer = layer; chip_cache = cache; return 17;
}
int32_t function_4a9b40_this(int32_t, int32_t) { return 0; }
int32_t function_4a9d70_this(int32_t object) { ++object_releases; return object + 4; }
void retdec_trace_star_state(const char*, int32_t) {}
int32_t function_405d60(int32_t handle) { cleanup_order.push_back(handle); return 0; }
int32_t function_463730(int32_t) { cleanup_order.push_back(-1); return 0; }
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
    expected_tree = address(lookup.data());
    lookup.set(&TreeIndex::head, Address{0x55550000});
    actor.set(&ActorRecord::manager, static_cast<Address>(address(&manager)));
    actor.set(&ActorRecord::x, 10.25f); actor.set(&ActorRecord::y, 20.75f);
    actor.set(&ActorRecord::scale, 2.0f);
    actor.set(&ActorRecord::scale_x, 0.5f); actor.set(&ActorRecord::scale_y, 1.5f);
    std::array<FrameRecord, 2> frames{};
    frames[0].duration = 2; frames[1].duration = 3;
    AnimationRecord animation{};
    animation.frames_begin = static_cast<Address>(address(frames.data()));
    animation.frames_end = animation.frames_begin + sizeof(frames);
    animation.left = 1; animation.top = 2; animation.right = 5; animation.bottom = 8;
    animation.flags = 0x1234; animation.has_bounds = 1;
    lookup_node[4] = address(&animation);
    lookup_result = address(lookup_node.data());
    CHECK(kinoko_actor_set_take(actor_address, 37) == address(frames.data()));
    CHECK(looked_up_key == 37 && actor.get(&ActorRecord::take) == 37);
    const auto local = actor.get(&ActorRecord::local_bounds);
    const auto world = actor.get(&ActorRecord::world_bounds);
    CHECK(local.left == 0.5f && local.top == 2 && local.right == 5.5f && local.bottom == 9);
    CHECK(world.left == 10.75f && world.right == 15.75f && world.top == 26.75f && world.bottom == 47.75f);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::width) == 5);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::height) == 21);
    kinoko_actor_advance_animation(actor_address, 36);
    CHECK(actor.get(&ActorRecord::frame_time) == 0);
    kinoko_actor_advance_animation(actor_address, 37);
    CHECK(actor.get(&ActorRecord::frame_time) == 1 && actor.get(&ActorRecord::frame_index) == 0);
    kinoko_actor_advance_animation(actor_address, 37);
    CHECK(actor.get(&ActorRecord::frame_index) == 1 && actor.get(&ActorRecord::frame_time) == 0);
    CHECK(actor.get(&ActorRecord::sprite_frame) == animation.frames_begin + sizeof(FrameRecord));
    for (int i = 0; i < 3; ++i) kinoko_actor_advance_animation(actor_address, 37);
    CHECK(actor.get(&ActorRecord::frame_index) == 1 && actor.get(&ActorRecord::frame_time) == 0);
    animation.loops = 1;
    for (int i = 0; i < 3; ++i) kinoko_actor_advance_animation(actor_address, 37);
    CHECK(actor.get(&ActorRecord::frame_index) == 0 && actor.get(&ActorRecord::current_frame) == animation.frames_begin);
    actor.set(&ActorRecord::direction, 1.0f);
    kinoko_actor_set_take(actor_address, 38);
    CHECK(actor.get(&ActorRecord::world_bounds).left == 4.75f);
    CHECK(actor.get(&ActorRecord::world_bounds).right == 9.75f);
    lookup_result = static_cast<int32_t>(lookup.get(&TreeIndex::head));
    const auto previous_animation = actor.get(&ActorRecord::animation);
    CHECK(kinoko_actor_set_take(actor_address, 999) == lookup_result);
    CHECK(actor.get(&ActorRecord::take) == 999 && actor.get(&ActorRecord::frame_time) == 0);
    CHECK(actor.get(&ActorRecord::animation) == previous_animation);
    ActorRecord source{}; source.frame_index = 100; source.frame_time = 71;
    kinoko_actor_sync_animation_state(actor_address, address(&source));
    CHECK(actor.get(&ActorRecord::frame_index) == 1 && actor.get(&ActorRecord::frame_time) == 71);
    source.frame_index = -1;
    kinoko_actor_sync_animation_state(actor_address, address(&source));
    CHECK(actor.get(&ActorRecord::frame_index) == -1);
    CHECK(actor.get(&ActorRecord::current_frame) == animation.frames_begin - sizeof(FrameRecord));
    CHECK(kinoko_actor_set_chip_flags(actor_address, nullptr, -1) == -1);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::chip_flags) == -1);
    CHECK(kinoko_actor_set_chip_bound_type(actor_address, nullptr, 0xffff) == 0xffff);
    CHECK(kinoko_actor_get_chip_id(actor_address, nullptr, 2) == 17);
    CHECK(chip_x == 10 && chip_y == 20 && chip_layer == 2);
    CHECK(chip_cache == actor_address + 512 + 2 * sizeof(int32_t));
    CHECK(kinoko_actor_reset_priority(actor_address, 42) == actor_address && priority_updates == 1);
    CHECK(actor.get(&ActorRecord::priority) == 42);
    CHECK(kinoko_actor_release(actor_address, nullptr) == 1);
    CHECK(actor.get(&ActorRecord::release_pending) && manager.cleanup_pending);
    InitialData initial{}; initial.width = 123; initial.chip_flags = -17;
    CHECK(kinoko_actor_set_init_data(actor_address, address(&initial)) == actor_address);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::width) == 123);
    CHECK(actor.view(&ActorRecord::initial).get(&InitialData::chip_flags) == -17);
    CHECK(bytes.front() == 0xa7 && bytes.back() == 0xa7);

    // Empty sentinels are container-owned and must survive manager clearing.
    std::array<int32_t, 6> animation_head{};
    std::array<int32_t, 5> actor_head{};
    animation_head[0] = animation_head[1] = animation_head[2] = address(animation_head.data());
    actor_head[0] = actor_head[1] = actor_head[2] = address(actor_head.data());
    reinterpret_cast<unsigned char*>(animation_head.data())[21] = 1;
    reinterpret_cast<unsigned char*>(actor_head.data())[17] = 1;
    manager.animation_lookup.head = static_cast<Address>(address(animation_head.data()));
    manager.actors.head = static_cast<Address>(address(actor_head.data()));
    struct ListHead { ListHead* next; ListHead* previous; } list;
    list.next = list.previous = &list;
    manager.animations.head = static_cast<Address>(address(&list));
    std::array<int32_t, 2> textures{27, 81};
    manager.textures.begin = static_cast<Address>(address(textures.data()));
    manager.textures.end = manager.textures.capacity = manager.textures.begin + sizeof(textures);
    manager.iteration.begin = 0x12340000; manager.iteration.end = 0x12340004;
    manager.iteration.capacity = 0x12340080;
    CHECK(kinoko_clear_actor_manager(address(&manager)) == 0x12340000);
    CHECK((cleanup_order == std::vector<int32_t>{27, 81, -1}));
    CHECK(manager.textures.end == manager.textures.begin && manager.textures.capacity == manager.textures.begin + sizeof(textures));
    CHECK(manager.iteration.end == manager.iteration.begin && manager.iteration.capacity == 0x12340080);
    CHECK(!manager.cleanup_pending && list.next == &list && list.previous == &list);
    CHECK(animation_head[1] == address(animation_head.data()) && actor_head[1] == address(actor_head.data()));
    std::puts("PASS: typed Actor animation, original bounds/timing/clamps, deferred ownership and cleanup order");
}
