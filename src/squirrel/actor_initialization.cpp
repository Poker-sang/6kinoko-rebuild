#include "kinoko/map_activation.h"
#include <squirrel.h>
#include "kinoko/actor_manager.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_animation.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>


namespace {
using namespace kinoko::actor;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
// These are external SqPlus references, not SQObjectPtr values. Their release
// order and the consuming initialization-call ABI match Squirrel 2.2.2.
void assign(void *destination,const void *source) {
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(destination), (const void *)(source)));
}
void release(void *object) { kinoko_sqplus_object_destroy((void *)(object)); }
}

// 45E120 captures the take before invoking script, so replacing the take in
// the callback does not immediately advance the newly selected animation.
extern "C" void kinoko_actor_tick(KinokoActor *actor) {
    if (!actor) return;
    const ActorView view(actor);
    const auto take=view.get(&ActorRecord::take);
    const auto type=kinoko_sqplus_object_type((void *)(view.bytes(&ActorRecord::update_function)));
    const auto trace=kinoko_actor_trace_step_begin(actor,type);
    if (type==0x08000100) {
        const auto result=kinoko_actor_step_callback(actor);
        kinoko_actor_trace_step_end(actor,result,trace);
    }
    kinoko_actor_advance_animation(actor,take);
}

// 45E5E0. Keep initialization before the callback and bounds publication after
// it: the callback can change both local bounds and position.
extern "C" int32_t kinoko_actor_initialize(KinokoActor *actor,KinokoActorManager *manager,
    const KinokoOwnedObjectWords *callback,float x,float y,float z,
    const KinokoOwnedObjectWords *argument) {
    auto *vm=kinoko_actor_default_vm();
    if (!actor) return 0;
    auto first=*callback,second=*argument;
    if (!first.vtable) first.vtable=kinoko_squirrel_object_vtable();
    if (!second.vtable) second.vtable=kinoko_squirrel_object_vtable();
    const ActorView view(actor);
    view.set(&ActorRecord::id,static_cast<int32_t>(view.get(&ActorRecord::pool_handle)&0xffff));
    view.set(&ActorRecord::manager,manager);
    KinokoOwnedObjectWords instance{};
    (int32_t)(intptr_t)(kinoko_sqplus_object_new_instance((void *)(&instance), (const void *)(kinoko_actor_class_object())));
    assign(view.bytes(&ActorRecord::script_object),&instance);
    release(&instance);
    kinoko_sqplus_object_set_instance((void *)(view.bytes(&ActorRecord::script_object)), (void *)(actor));

    auto **slot=static_cast<KinokoActor **>(std::malloc(sizeof(KinokoActor *)));
    if (!slot) return 0;
    *slot=actor;
    int32_t control=0;
    kinoko_native_control_create(address(&control),address(slot));
    if (!control) { std::free(slot);return 0; }
    const auto old_control=view.get(&ActorRecord::owner_control);
    view.set(&ActorRecord::owner,slot);
    view.set(&ActorRecord::owner_control,pointer<ControlRecord>(control));
    kinoko_native_release_strong(address(old_control));

    view.set(&ActorRecord::active,static_cast<uint8_t>((view.get(&ActorRecord::initial).chip_flags&0x20000)==0));
    view.set(&ActorRecord::registration_flag20,uint8_t{0});
    view.set(&ActorRecord::visible,uint8_t{1});
    assign(view.bytes(&ActorRecord::initial_function),&first);
    assign(view.bytes(&ActorRecord::initial_argument),&second);
    view.set(&ActorRecord::spawn_x,x);
    view.set(&ActorRecord::blend,int32_t{1});
    view.set(&ActorRecord::priority,int32_t{0});
    view.set(&ActorRecord::spawn_y,y);
    view.set(&ActorRecord::frame_time,int32_t{0});
    view.set(&ActorRecord::frame_index,int32_t{0});
    view.set(&ActorRecord::spawn_z,z);
    view.set(&ActorRecord::take_duration,int32_t{0});
    view.set(&ActorRecord::collision_group,uint32_t{0});
    view.set(&ActorRecord::scale_y,1.0f);
    view.set(&ActorRecord::scale_x,1.0f);
    view.set(&ActorRecord::scale,1.0f);
    view.set(&ActorRecord::collision_mask,uint32_t{0});
    view.set(&ActorRecord::callback_group,uint32_t{0});
    view.set(&ActorRecord::callback_mask,uint32_t{0});
    view.set(&ActorRecord::flags,uint32_t{0});
    view.set(&ActorRecord::rotation,0.0f);
    view.set(&ActorRecord::collision_flags,uint32_t{0});
    view.set(&ActorRecord::offset_y,0.0f);
    view.set(&ActorRecord::offset_x,0.0f);
    view.set(&ActorRecord::blue,int32_t{255});
    view.set(&ActorRecord::green,int32_t{255});
    view.set(&ActorRecord::red,int32_t{255});
    view.set(&ActorRecord::alpha,int32_t{255});
    view.set(&ActorRecord::x,x);
    view.set(&ActorRecord::y,y);
    view.set(&ActorRecord::world_bounds,Bounds{x,y,x,y});
    view.set(&ActorRecord::update_group,UINT32_MAX);
    view.set(&ActorRecord::unknown476,UINT32_MAX);
    view.set(&ActorRecord::direction,z);
    view.set(&ActorRecord::pitch,0.0f);
    view.set(&ActorRecord::pitch_top,0.0f);
    view.set(&ActorRecord::velocity_x,0.0f);
    view.set(&ActorRecord::velocity_y,0.0f);
    view.set(&ActorRecord::collision_scan_cache,std::array<int32_t,8>{});
    view.set(&ActorRecord::chip_cache_storage,std::array<unsigned char,32>{});
    view.set(&ActorRecord::animation,static_cast<KinokoAnimation *>(nullptr));
    view.set(&ActorRecord::current_frame,static_cast<KinokoAnimationFrame *>(nullptr));

    KinokoScriptCallback owner_state{};
    kinoko_script_callback_construct(&owner_state,nullptr);
    view.set(&ActorRecord::update_vm,owner_state.vm);
    assign(view.bytes(&ActorRecord::update_environment),&owner_state.environment);
    assign(view.bytes(&ActorRecord::update_function),&owner_state.closure);
    release(&owner_state.closure);
    release(&owner_state.environment);
    view.view(&ActorRecord::initial).set(&InitialData::chip_bound_type,uint16_t{0xffff});
    view.set(&ActorRecord::hits,std::array<int32_t,4>{});
    view.set(&ActorRecord::local_bounds,Bounds{});
    view.set(&ActorRecord::free_width,1000.0f);
    view.set(&ActorRecord::free_height,1000.0f);
    view.set(&ActorRecord::crushed,uint8_t{0});

    if (vm && kinoko_sqplus_object_type((void *)(&first))==0x08000100) {
        KinokoOwnedObjectWords callback_object{};
        KinokoScriptCallback call_state{};
        (int32_t)(intptr_t)(kinoko_sqplus_object_construct_value((void *)(&callback_object), second.type, second.value));
        call_state.vm=vm;
        (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(reinterpret_cast<int32_t *>(&call_state.environment)), (const void *)(view.bytes(&ActorRecord::script_object))));
        (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(reinterpret_cast<int32_t *>(&call_state.closure)), (const void *)(&first)));
        kinoko_script_callback_invoke_owned(&call_state,&callback_object,callback_object.type,callback_object.value);
        release(&call_state.closure);
        release(&call_state.environment);
    }
    const auto local=view.get(&ActorRecord::local_bounds);
    const auto position_x=view.get(&ActorRecord::x),position_y=view.get(&ActorRecord::y);
    const Bounds world{local.left+position_x,local.top+position_y,local.right+position_x,local.bottom+position_y};
    view.set(&ActorRecord::world_bounds,world);
    view.set(&ActorRecord::bounds_anchor_x,world.left);
    view.set(&ActorRecord::bounds_anchor_y,world.top);
    view.set(&ActorRecord::previous_bounds,world);
    view.set(&ActorRecord::previous_x,position_x);
    view.set(&ActorRecord::previous_y,position_y);
    return 1;
}

extern "C" KinokoActor *kinoko_actor_create_map_instance(KinokoActorManager *manager,
    const KinokoSquirrelObject *callback,float x,float y,int32_t chip_id,const unsigned char *initial_data) {
    const auto function=kinoko::legacy::load<KinokoOwnedObjectWords>(callback);
    const KinokoOwnedObjectWords argument{kinoko_squirrel_object_vtable(),OT_INTEGER,chip_id};
    return kinoko_actor_manager_create(manager,&function,x,y,-1.0f,&argument,initial_data);
}
extern "C" KinokoActor *kinoko_actor_create_collision_proxy(KinokoActorManager *manager) {
    KinokoOwnedObjectWords empty{};
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(&empty)));
    return kinoko_actor_manager_create(manager,&empty,0.0f,0.0f,1.0f,&empty,nullptr);
}
