#include "kinoko/map_activation.h"
#include <squirrel.h>
#include "kinoko/actor_manager.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_animation.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/squirrel_host_object.hpp"
#include <cstdlib>


namespace {
using namespace kinoko::actor;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
// These are external SqPlus references, not SQObjectPtr values. Their release
// order and the consuming initialization-call ABI match Squirrel 2.2.2.
void assign(void *destination,const void *source) {
    kinoko_sqplus_object_assign((void *)(destination), (const void *)(source));
}
void release(void *object) { kinoko_sqplus_object_destroy(object); }
class LocalObject final {
    KinokoOwnedObjectWords words_{};
public:
    LocalObject() { kinoko_sqplus_object_initialize(&words_); }
    explicit LocalObject(const void *source) { kinoko_sqplus_object_copy_construct(&words_,source); }
    ~LocalObject() { release(&words_); }
    LocalObject(const LocalObject&)=delete;
    LocalObject& operator=(const LocalObject&)=delete;
    KinokoOwnedObjectWords *data() { return &words_; }
    KinokoOwnedObjectWords transfer() {
        const auto result=words_;
        kinoko_sqplus_object_initialize(&words_);
        return result;
    }
};
struct CallbackOwner {
    KinokoScriptCallback value{};
    ~CallbackOwner() { release(&value.closure); release(&value.environment); }
};
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
    // Retain argument before callback, including when they alias saved Init
    // fields. 45E5E0 consumes these copies on return and during C++ unwinding.
    LocalObject second(argument),first(callback);
    const ActorView view(actor);
    view.set(&ActorRecord::id,static_cast<int32_t>(view.get(&ActorRecord::pool_handle)&0xffff));
    view.set(&ActorRecord::manager,manager);
    {
        LocalObject instance;
        kinoko::script::ObjectView(instance.data()).write(
            kinoko::script::upstream::sqplus_create_instance(vm,
                kinoko::script::ObjectView(kinoko_actor_class_object()).value()));
        assign(view.bytes(&ActorRecord::script_object),instance.data());
    }
    kinoko_sqplus_object_set_instance((void *)(view.bytes(&ActorRecord::script_object)), (void *)(actor));

    auto **slot=static_cast<KinokoActor **>(std::malloc(sizeof(KinokoActor *)));
    if (!slot) return 0;
    int32_t control=0;
    kinoko_native_control_create(address(&control),address(slot));
    if (!control) { std::free(slot);return 0; }
    // Original shared owner assignment keeps the temporary strong reference
    // alive while releasing the outgoing control, then publishes the Actor.
    kinoko_native_add_strong(control);
    const auto old_control=view.get(&ActorRecord::owner_control);
    view.set(&ActorRecord::owner,slot);
    view.set(&ActorRecord::owner_control,pointer<ControlRecord>(control));
    kinoko_native_release_strong(address(old_control));
    kinoko_native_release_strong(control);
    *view.get(&ActorRecord::owner)=actor;

    view.set(&ActorRecord::active,static_cast<uint8_t>((view.get(&ActorRecord::initial).chip_flags&0x20000)==0));
    view.set(&ActorRecord::registration_flag20,uint8_t{0});
    view.set(&ActorRecord::visible,uint8_t{1});
    assign(view.bytes(&ActorRecord::initial_function),first.data());
    assign(view.bytes(&ActorRecord::initial_argument),second.data());
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

    {
        CallbackOwner owner;
        auto &owner_state=owner.value;
        kinoko_script_callback_construct(&owner_state,nullptr);
        view.set(&ActorRecord::update_vm,owner_state.vm);
        assign(view.bytes(&ActorRecord::update_environment),&owner_state.environment);
        assign(view.bytes(&ActorRecord::update_function),&owner_state.closure);
    }
    view.view(&ActorRecord::initial).set(&InitialData::chip_bound_type,uint16_t{0xffff});
    view.set(&ActorRecord::hits,std::array<int32_t,4>{});
    view.set(&ActorRecord::local_bounds,Bounds{});
    view.set(&ActorRecord::free_width,1000.0f);
    view.set(&ActorRecord::free_height,1000.0f);
    view.set(&ActorRecord::crushed,uint8_t{0});

    if (vm && kinoko_sqplus_object_type(first.data())==0x08000100) {
        LocalObject callback_argument;
        CallbackOwner call;
        auto &call_state=call.value;
        kinoko_sqplus_object_construct_value(callback_argument.data(),second.data()->type,second.data()->value);
        call_state.vm=vm;
        kinoko_sqplus_object_copy_construct(&call_state.environment,view.bytes(&ActorRecord::script_object));
        kinoko_sqplus_object_copy_construct(&call_state.closure,first.data());
        // Until this call owns the by-value argument, local unwinding owns it.
        auto callback_object=callback_argument.transfer();
        kinoko_script_callback_invoke_owned(&call_state,&callback_object,callback_object.type,callback_object.value);
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
    kinoko_sqplus_object_initialize((void *)(&empty));
    return kinoko_actor_manager_create(manager,&empty,0.0f,0.0f,1.0f,&empty,nullptr);
}
