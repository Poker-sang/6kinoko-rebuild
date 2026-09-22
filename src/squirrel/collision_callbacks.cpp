#include "kinoko/script_callbacks.h"
#include "kinoko/map_collision.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_game_objects.h"

extern "C" void retdec_trace(const char *);
namespace {
using namespace kinoko::actor;
using kinoko::legacy::address;

int32_t invoke(KinokoActor *receiver, KinokoActor *other) {
    const ActorView actor(receiver);
    auto *vm = actor.get(&ActorRecord::collision_vm);
    const SQInteger base = sq_gettop(vm);
    int32_t argument[3];
    kinoko_actor_trace_collision(receiver, 0);
    function_4a9500_this(argument, address(ActorView(other).bytes(&ActorRecord::script_object)));
    // Existing call helper consumes this copied external reference, including
    // failures. Do not add a second destructor or force stack reset on success.
    const int32_t result = kinoko_script_callback_invoke_owned(reinterpret_cast<KinokoScriptCallback *>(actor.bytes(&ActorRecord::collision_vm)),
        reinterpret_cast<KinokoOwnedObjectWords *>(argument), argument[1], argument[2]);
    kinoko_actor_trace_collision(receiver, 1);
    if (result < 0) {
        retdec_trace("actor:collision-callback-failed");
        sq_settop(vm, base);
        kinoko_actor_clear_failed_collision_callback(receiver);
    }
    return result;
}
}

extern "C" int32_t kinoko_collision_dispatch_pair(KinokoActor *first, KinokoActor *second) {
    const ActorView a(first), b(second);
    if (((b.get(&ActorRecord::callback_group) & a.get(&ActorRecord::callback_mask)) |
         (b.get(&ActorRecord::callback_mask) & a.get(&ActorRecord::callback_group))) == 0) return 0;
    const auto first_bounds = a.get(&ActorRecord::world_bounds);
    const auto second_bounds = b.get(&ActorRecord::world_bounds);
    if (!(second_bounds.right >= first_bounds.left && second_bounds.bottom >= first_bounds.top &&
          second_bounds.left <= first_bounds.right && second_bounds.top <= first_bounds.bottom)) return 0;
    int32_t result = 0;
    if ((a.get(&ActorRecord::callback_mask) & b.get(&ActorRecord::callback_group)) &&
        kinoko::script::ObjectView(a.bytes(&ActorRecord::collision_function)).value()._type == OT_CLOSURE)
        result = invoke(first, second);
    // The first callback can change masks and the second callback object.
    if ((a.get(&ActorRecord::callback_group) & b.get(&ActorRecord::callback_mask)) &&
        kinoko::script::ObjectView(b.bytes(&ActorRecord::collision_function)).value()._type == OT_CLOSURE)
        result = invoke(second, first);
    return result;
}

extern "C" int32_t kinoko_collision_dispatch_all(KinokoActorManager *manager) {
    const ManagerView owner(manager);
    const int32_t count = owner.get(&ManagerPrefix::iteration_count);
    auto *actors = owner.get(&ManagerPrefix::iteration).begin;
    auto *candidates = owner.get(&ManagerPrefix::callback_candidates).begin;
    int32_t candidate_count = 0;
    for (int32_t i = 0; i < count; ++i) {
        const ActorView actor(actors[i]);
        if (actor.get(&ActorRecord::active) &&
            (actor.get(&ActorRecord::callback_group) | actor.get(&ActorRecord::callback_mask)))
            candidates[candidate_count++] = actors[i];
    }
    for (int32_t first = 0; first < candidate_count; ++first)
        for (int32_t second = first + 1; second < candidate_count; ++second)
            kinoko_collision_dispatch_pair(candidates[first], candidates[second]);
    return candidate_count;
}

extern "C" int32_t kinoko_collision_dispatch_actor(KinokoActorManager *manager, KinokoActor *actor) {
    const ManagerView owner(manager);
    auto *actors = owner.get(&ManagerPrefix::iteration).begin;
    int32_t result = 0;
    // Keep the original fresh count read after every callback.
    for (uint32_t i = 0; i < static_cast<uint32_t>(owner.get(&ManagerPrefix::iteration_count)); ++i) {
        auto *other = actors[i];
        if (ActorView(other).get(&ActorRecord::active) && other != actor)
            result = kinoko_collision_dispatch_pair(actor, other);
    }
    return result;
}
