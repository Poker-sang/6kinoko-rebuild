#include "kinoko/actor_manager.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_priority.h"
#include "kinoko/actor_pool.h"
#include "kinoko/actor_pool_dispatch.hpp"
#include "kinoko/actor_owner_list.h"
#include "kinoko/actor_methods.h"
#include "kinoko/animation_storage.h"
#include "kinoko/integer_vector.h"
#include "kinoko/integer_map.h"
#include "kinoko/native_buffer.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>
#include <cstring>
#include <climits>

extern "C" int32_t kinoko_collision_dispatch_all(KinokoActorManager *);
namespace {
using namespace kinoko::actor;
using kinoko::native::RecordView;
using kinoko::legacy::address;
using kinoko::legacy::pointer;

bool insert_actor(const ManagerView manager, const ActorView actor) {
    auto *entry=kinoko_actor_priority_insert(manager.bytes(&ManagerPrefix::actors),
        reinterpret_cast<KinokoActor *>(actor.data()));
    if (!entry) return false;
    actor.set(&ActorRecord::priority_entry,entry);
    manager.set(&ManagerPrefix::cleanup_pending,uint8_t{1});
    return true;
}
void retire(const ManagerView manager, const ActorView actor) {
    auto *pool=manager.get(&ManagerPrefix::pool);
    pool_methods(pool).retire(pool,actor.get(&ActorRecord::pool_handle));
}
}

// 46B0A0. The global host reserves 512 bytes, beyond the verified prefix.
extern "C" KinokoActorManager *kinoko_actor_manager_construct(KinokoActorManager *manager) {
    if (!manager) return nullptr;
    // Clear only the verified host prefix; never overwrite caller tail storage.
    ManagerView(manager).clear();
    const ManagerView state(manager);
    state.set(&ManagerPrefix::methods,kinoko_actor_owner_methods());
    kinoko_actor_owner_list_construct(manager);
    auto *pool=static_cast<KinokoActorPool *>(std::calloc(1,80));
    if (!pool || !kinoko_actor_pool_construct(pool)) return nullptr;
    state.set(&ManagerPrefix::pool,pool);
    state.view(&ManagerPrefix::animation_lookup).set(&KinokoIntegerMapIndex::owner,
        kinoko_integer_map_create());
    kinoko_animation_list_construct(address(state.bytes(&ManagerPrefix::animations)));
    kinoko_integer_vector_construct((KinokoIntegerVector*)(state.bytes(&ManagerPrefix::textures)));
    kinoko_priority_construct(state.bytes(&ManagerPrefix::actors));
    for (int32_t i=0;i<4;++i) {
        auto *layer=static_cast<RenderLayerRecord *>(std::calloc(1,sizeof(RenderLayerRecord)));
        if (!layer) return nullptr;
        const RecordView<RenderLayerRecord> view(layer);
        view.set(&RenderLayerRecord::methods,kinoko_actor_render_layer_methods());
        view.set(&RenderLayerRecord::manager,manager);
        view.set(&RenderLayerRecord::index,i);
        auto layers=state.get(&ManagerPrefix::render_layers);
        layers[i]=layer;
        state.set(&ManagerPrefix::render_layers,layers);
    }
    return manager;
}

extern "C" int32_t kinoko_actor_manager_initialize(KinokoActorManager *manager) {
    if (!manager) return 0;
    const ManagerView state(manager);
    state.set(&ManagerPrefix::update_mask,int32_t{-1});
    state.set(&ManagerPrefix::iteration_count,int32_t{0});
    while (kinoko_actor_owner_list_size(manager)<512)
        if (!kinoko_actor_owner_list_acquire(manager)) break;
    kinoko_actor_owner_list_clear(manager);
    return 1;
}

extern "C" KinokoActor *kinoko_actor_manager_create(KinokoActorManager *manager,
    const KinokoOwnedObjectWords *callback,float x,float y,float z,
    const KinokoOwnedObjectWords *argument,const void *initial_data) {
    if (!manager) return nullptr;
    const ManagerView state(manager);
    uint32_t handle{};
    auto *pool=state.get(&ManagerPrefix::pool);
    auto *actor=pool_methods(pool).acquire(pool,&handle);
    if (!actor) return nullptr;
    const ActorView view(actor);
    view.set(&ActorRecord::pool_handle,handle);
    view.set(&ActorRecord::owner_references,int32_t{1});
    if (initial_data) kinoko_actor_set_init_data(actor,initial_data);
    // 463CB6 explicitly retires the handle when initialization fails.
    if (!kinoko_actor_initialize(actor,manager,callback,x,y,z,argument)) {
        pool_methods(pool).retire(pool,handle);
        return nullptr;
    }
    if (!insert_actor(state,view)) return nullptr;
    return actor;
}

extern "C" int32_t kinoko_actor_manager_reindex(KinokoActorManager *manager,KinokoActor *actor) {
    if (!manager || !actor) return 0;
    const ManagerView state(manager);
    const ActorView view(actor);
    if (auto *entry=view.get(&ActorRecord::priority_entry)) {
        kinoko_actor_priority_erase(state.bytes(&ManagerPrefix::actors),entry);
        view.set(&ActorRecord::priority_entry,static_cast<void *>(nullptr));
    }
    return insert_actor(state,view);
}

extern "C" void *kinoko_actor_manager_clear_actors(KinokoActorManager *manager) {
    const ManagerView state(manager);
    auto *index=state.bytes(&ManagerPrefix::actors);
    auto *sentinel=pointer<void>(state.get(&ManagerPrefix::actors).head);
    if (!sentinel) return nullptr;
    for (auto *entry=kinoko_actor_priority_first(index);entry!=sentinel;) {
        if (auto *actor=kinoko_actor_priority_value(entry)) {
            const ActorView view(actor);
            const auto references=view.get(&ActorRecord::owner_references)-1;
            view.set(&ActorRecord::owner_references,references);
            if (!references) retire(state,view);
        }
        // 463761 advances after retirement: release hooks can insert successors.
        entry=kinoko_actor_priority_next(index,entry);
    }
    kinoko_priority_clear(index);
    return sentinel;
}

extern "C" void *kinoko_actor_manager_reset(KinokoActorManager *manager) {
    if (!manager) return nullptr;
    const ManagerView state(manager);
    state.set(&ManagerPrefix::iteration_count,int32_t{0});
    auto *result=kinoko_actor_manager_clear_actors(manager);
    state.set(&ManagerPrefix::cleanup_pending,uint8_t{1});
    return result;
}

extern "C" int32_t kinoko_actor_manager_refresh(KinokoActorManager *manager) {
    if (!manager) return 0;
    const ManagerView state(manager);
    if (!state.get(&ManagerPrefix::cleanup_pending)) return state.get(&ManagerPrefix::iteration_count);
    const auto tree=state.get(&ManagerPrefix::actors);
    state.set(&ManagerPrefix::iteration_count,tree.count);
    // 463D56 publishes even zero; the empty branch leaves dirty/ranges intact.
    if (!tree.count) return 0;
    const auto iteration=state.get(&ManagerPrefix::iteration);
    if (!iteration.begin || iteration.end-iteration.begin<tree.count) {
        // Both buffers grow together, based on the iteration buffer alone.
        for (auto member:{&ManagerPrefix::iteration,&ManagerPrefix::callback_candidates}) {
            const auto buffer=state.view(member);
            if (tree.count>INT32_MAX/8 || !kinoko_native_buffer_resize(address(buffer.data()),tree.count*8u)) return 0;
        }
    }
    auto **actors=state.get(&ManagerPrefix::iteration).begin;
    auto *index=state.bytes(&ManagerPrefix::actors);
    auto *sentinel=pointer<void>(tree.head);
    int32_t count=0,back=0,middle=0,front=0;
    for (auto *entry=kinoko_actor_priority_first(index);entry!=sentinel;) {
        auto *actor=kinoko_actor_priority_value(entry);
        const ActorView view(actor);
        // 463DAA exposes the pending actor in the output slot during destruction.
        actors[count]=actor;
        if (view.get(&ActorRecord::release_pending)) {
            entry=kinoko_actor_priority_erase_next(index,view.get(&ActorRecord::priority_entry));
            retire(state,view); // deferred release does not decrement refcount
            state.set(&ManagerPrefix::iteration_count,state.get(&ManagerPrefix::iteration_count)-1);
        } else {
            ++count;
            const auto priority=view.get(&ActorRecord::priority);
            back+=priority==-1;
            middle+=priority<0xffff;
            front+=priority<0x10000;
            entry=kinoko_actor_priority_next(index,entry);
        }
    }
    const int32_t limits[]{0,back,middle,front,state.get(&ManagerPrefix::iteration_count)};
    const auto layers=state.get(&ManagerPrefix::render_layers);
    for (size_t i=0;i<layers.size();++i) if (layers[i]) {
        const RecordView<RenderLayerRecord> layer(layers[i]);
        if (i) layer.set(&RenderLayerRecord::begin,limits[i]);
        layer.set(&RenderLayerRecord::end,limits[i+1]);
    }
    state.set(&ManagerPrefix::cleanup_pending,uint8_t{0});
    return state.get(&ManagerPrefix::iteration_count);
}

extern "C" int32_t kinoko_actor_activate(KinokoActor *actor,KinokoCamera *camera,float extent) {
    if (!actor) return 0;
    const ActorView view(actor);
    if (view.get(&ActorRecord::registration_flag20)) return 0;
    if (camera) {
        const auto a=view.get(&ActorRecord::world_bounds);
        const auto b=RecordView<CameraBoundsRecord>(camera).get(&CameraBoundsRecord::bounds);
        if (!(a.left<=b.right+extent && b.left-extent<=a.right && b.bottom+extent>=a.top && a.bottom>=b.top-extent)) return 0;
    }
    view.set(&ActorRecord::active,uint8_t{1});
    return 1;
}

extern "C" int32_t kinoko_actor_manager_update(KinokoActorManager *manager,KinokoCamera *camera) {
    kinoko_actor_manager_refresh(manager);
    if (!manager) return 0;
    kinoko_collision_dispatch_all(manager);
    kinoko_actor_manager_refresh(manager);
    const ManagerView state(manager);
    auto **actors=state.get(&ManagerPrefix::iteration).begin;
    // 46423D snapshots the buffer, but 464281 reloads the count and 464276
    // reads the mask for each actor. A callback can change the update mask.
    for (int32_t i=0;actors && i<state.get(&ManagerPrefix::iteration_count);++i) {
        auto *actor=actors[i];
        const ActorView view(actor);
        kinoko_actor_manager_trace_actor(1,actor,camera,state.get(&ManagerPrefix::update_mask));
        if (actor && (view.get(&ActorRecord::active) || kinoko_actor_activate(actor,camera,64.0f)) &&
            (view.get(&ActorRecord::update_group)&state.get(&ManagerPrefix::update_mask)))
            kinoko_actor_tick(actor);
        kinoko_actor_manager_trace_actor(2,actor,camera,state.get(&ManagerPrefix::update_mask));
    }
    kinoko_actor_manager_refresh(manager);
    kinoko_actor_manager_refresh_collision();
    actors=state.get(&ManagerPrefix::iteration).begin;
    for (int32_t i=0;actors && i<state.get(&ManagerPrefix::iteration_count);++i) {
        auto *actor=actors[i];
        const ActorView view(actor);
        if (actor && view.get(&ActorRecord::active) &&
            (view.get(&ActorRecord::update_group)&state.get(&ManagerPrefix::update_mask)))
            kinoko_actor_motion_host(actor);
        kinoko_actor_manager_trace_actor(3,actor,camera,state.get(&ManagerPrefix::update_mask));
    }
    return state.get(&ManagerPrefix::iteration_count);
}

extern "C" int32_t kinoko_actor_manager_render_layer(KinokoActorManager *manager,KinokoCamera *camera,int32_t layer_index) {
    if (!manager || layer_index<0) return 0;
    const ManagerView state(manager);
    if (!state.get(&ManagerPrefix::iteration_count)) return 0;
    auto *layer=state.get(&ManagerPrefix::render_layers)[layer_index];
    if (!layer) return 0;
    const auto range=RecordView<RenderLayerRecord>(layer).load();
    auto **actors=state.get(&ManagerPrefix::iteration).begin;
    if (!actors || range.begin<0 || range.end<range.begin) return 0;
    int32_t result=0;
    for (auto i=range.begin;i<range.end;++i)
        if (actors[i]) result=kinoko_actor_render_host(actors[i],camera);
    return result;
}
