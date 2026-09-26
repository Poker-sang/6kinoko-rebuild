#include "kinoko/actor_records.hpp"
#include "kinoko/actor_manager.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/legacy_memory.hpp"
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using namespace kinoko::actor;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"actor state line %d: %s\n",__LINE__,#x); std::abort(); } } while (0)
namespace {
std::vector<int32_t> events;
ActorRecord *reset_actor;
int32_t reset_priority;
}
extern "C" {
void kinoko_native_add_strong(int32_t p) { events.push_back(10); events.push_back(p); }
void kinoko_native_release_strong(int32_t p) { events.push_back(11); events.push_back(p); }
void kinoko_native_add_weak(int32_t p) { events.push_back(12); events.push_back(p); }
void kinoko_native_release_weak(int32_t p) { events.push_back(13); events.push_back(p); }
void * kinoko_sqplus_object_assign(void * out, const void * in) { std::memmove(out,in,12); return out; }
void * kinoko_sqplus_object_copy_construct(void * out, const void * in) { std::memcpy(out,in,12); events.push_back(static_cast<int32_t *>(out)[1]); return (void *)(intptr_t)(out); }
void*  kinoko_sqplus_object_destroy(void * p) { events.push_back(static_cast<int32_t *>(p)[1]); return (void*)(intptr_t)((int32_t)(intptr_t)(p)); }
int32_t kinoko_actor_clear_script(KinokoActor *actor) {
    CHECK(actor==reinterpret_cast<KinokoActor *>(reset_actor));
    CHECK(!reset_actor->owner && !reset_actor->owner_control && !reset_actor->step && !reset_actor->step_control);
    events.push_back(20); return 0;
}
int32_t kinoko_actor_initialize(KinokoActor *actor,KinokoActorManager *manager,
    const KinokoOwnedObjectWords *callback,float x,float y,float z,const KinokoOwnedObjectWords *argument) {
    CHECK(actor==reinterpret_cast<KinokoActor *>(reset_actor)); CHECK(manager==reset_actor->manager);
    CHECK(callback->type==31 && argument->type==32 && x==10 && y==20 && z==-1);
    reset_actor->initial_function.fill(0); reset_actor->initial_argument.fill(0);
    reset_actor->priority=73; events.push_back(21); return 1;
}
int32_t kinoko_actor_reset_priority(KinokoActor *actor,int32_t priority) {
    CHECK(actor==reinterpret_cast<KinokoActor *>(reset_actor)); reset_priority=priority; events.push_back(22); return 81;
}
}
int main() {
    ActorRecord source{},destination{};
    auto *src=reinterpret_cast<KinokoActor *>(&source), *dst=reinterpret_cast<KinokoActor *>(&destination);
    KinokoActor *slot=src;
    ControlRecord owner{},old_owner{},weak{},old_weak{};
    source.owner=&slot; source.owner_control=&owner; source.step=&slot; source.step_control=&weak;
    destination.owner_control=&old_owner; destination.step_control=&old_weak;
    source.pool_handle=0xabcd1234; source.priority=123; source.x=27.5f;
    source.collision_chip=reinterpret_cast<const unsigned char *>(&source.initial);
    source.collision_placement=source.inline_slots.data();
    destination.unknown372=0x12345678; destination.unknown23=91; destination.vtable=&old_owner;
    source.chip_cache_storage.fill(0x55); source.unknown360.fill(0x43);
    for (auto member:script_members) { (source.*member).fill(0x31); (destination.*member).fill(0); }
    CHECK(kinoko_actor_assign(dst,src)==dst);
    CHECK((events==std::vector<int32_t>{10,address(&owner),11,address(&old_owner),12,address(&weak),13,address(&old_weak)}));
    CHECK(destination.owner==&slot && destination.step==&slot);
    CHECK(destination.collision_chip==source.collision_chip && destination.collision_placement==source.collision_placement);
    CHECK(destination.unknown372==0x12345678 && destination.unknown23==91 && destination.vtable==&old_owner);
    CHECK(destination.pool_handle==source.pool_handle && destination.priority==123 && destination.x==27.5f);
    CHECK(destination.chip_cache_storage==source.chip_cache_storage && destination.unknown360==source.unknown360);
    for (auto member:script_members) CHECK(destination.*member==source.*member);
    events.clear(); kinoko_actor_assign(dst,dst);
    CHECK((events==std::vector<int32_t>{10,address(&owner),11,address(&owner)}));
    // Reset passes saved fields to the retaining Init entry, and takes the
    // priority written by Init instead of restoring the pre-reset priority.
    reset_actor=&destination; destination.spawn_x=10; destination.spawn_y=20; destination.spawn_z=-1;
    const KinokoOwnedObjectWords callback{1,31,2},argument{1,32,3};
    std::memcpy(destination.initial_function.data(),&callback,12);
    std::memcpy(destination.initial_argument.data(),&argument,12);
    events.clear(); CHECK(kinoko_actor_reset(dst)==81 && reset_priority==73);
    CHECK((events==std::vector<int32_t>{13,address(&weak),11,address(&owner),20,21,22}));
    return 0;
}
