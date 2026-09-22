#include "kinoko/actor_manager.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_animation.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>

extern "C" int32_t retdec_function_45df10_impl(int32_t,int32_t);
namespace {
using namespace kinoko::actor;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
// These are external SqPlus references, not SQObjectPtr values. Their release
// order and the consuming initialization-call ABI match Squirrel 2.2.2.
void assign(void *destination,const void *source) {
    function_4a95c0_this(address(destination),address(source));
}
void release(void *object) { function_4a9d70_this(address(object)); }
}

// 45E120 captures the take before invoking script, so replacing the take in
// the callback does not immediately advance the newly selected animation.
extern "C" void kinoko_actor_tick(KinokoActor *actor) {
    if (!actor) return;
    const ActorView view(actor);
    const auto take=view.get(&ActorRecord::take);
    const auto type=function_4a9a30_this(address(view.bytes(&ActorRecord::object108)));
    const auto trace=kinoko_actor_trace_step_begin(actor,type);
    if (type==0x08000100) {
        const auto result=kinoko_actor_step_callback(address(actor));
        kinoko_actor_trace_step_end(actor,result,trace);
    }
    kinoko_actor_advance_animation(address(actor),take);
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
    view.set(&ActorRecord::manager,static_cast<Address>(address(manager)));
    KinokoOwnedObjectWords instance{};
    function_4a90c0_this(address(&instance),address(kinoko_actor_class_object()));
    assign(view.bytes(&ActorRecord::script_object),&instance);
    release(&instance);
    function_4a9bb0_this(address(view.bytes(&ActorRecord::script_object)),address(actor));

    auto **slot=static_cast<KinokoActor **>(std::malloc(sizeof(KinokoActor *)));
    if (!slot) return 0;
    *slot=actor;
    int32_t control=0;
    kinoko_native_control_create(address(&control),address(slot));
    if (!control) { std::free(slot);return 0; }
    const auto old_control=view.get(&ActorRecord::owner_control);
    view.set(&ActorRecord::owner,static_cast<Address>(address(slot)));
    view.set(&ActorRecord::owner_control,static_cast<Address>(control));
    kinoko_native_release_strong(old_control);

    view.set(&ActorRecord::active,static_cast<uint8_t>((view.get(&ActorRecord::initial).chip_flags&0x20000)==0));
    view.set(&ActorRecord::registration_flag20,uint8_t{0});
    view.set(&ActorRecord::visible,uint8_t{1});
    assign(view.bytes(&ActorRecord::update_callback),&first);
    assign(view.bytes(&ActorRecord::collision_callback),&second);
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

    int32_t owner_state[7]{};
    retdec_function_45df10_impl(address(owner_state),0);
    view.set(&ActorRecord::update_vm,pointer<SQVM>(owner_state[0]));
    assign(view.bytes(&ActorRecord::object96),owner_state+1);
    assign(view.bytes(&ActorRecord::object108),owner_state+4);
    release(owner_state+4);
    release(owner_state+1);
    view.view(&ActorRecord::initial).set(&InitialData::chip_bound_type,uint16_t{0xffff});
    view.set(&ActorRecord::hits,std::array<int32_t,4>{});
    view.set(&ActorRecord::local_bounds,Bounds{});
    view.set(&ActorRecord::free_width,1000.0f);
    view.set(&ActorRecord::free_height,1000.0f);
    view.set(&ActorRecord::crushed,uint8_t{0});

    if (vm && function_4a9a30_this(address(&first))==0x08000100) {
        KinokoOwnedObjectWords callback_object{};
        int32_t call_state[7]{};
        function_4a9540_this(address(&callback_object),second.type,second.value);
        call_state[0]=address(vm);
        function_4a9500_this(call_state+1,address(view.bytes(&ActorRecord::script_object)));
        function_4a9500_this(call_state+4,address(&first));
        function_45e020_this(address(call_state),address(&callback_object),callback_object.type,callback_object.value);
        release(call_state+4);
        release(call_state+1);
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
