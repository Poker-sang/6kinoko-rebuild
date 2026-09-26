#include "kinoko/actor_manager.h"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/legacy_memory.hpp"
namespace {
using namespace kinoko::actor;
using kinoko::legacy::address;
template<class T> void copy(const ActorView& out,const ActorView& in,T ActorRecord::*member) {
    out.set(member,in.get(member));
}
void copy_script(const ActorView& out,const ActorView& in,ScriptStorage ActorRecord::*member) {
    kinoko_sqplus_object_assign((void *)(out.bytes(member)), (const void *)(in.bytes(member)));
}
}
extern "C" int32_t kinoko_actor_reset(KinokoActor *actor) {
    const ActorView view(actor);
    const auto parent=view.get(&ActorRecord::step_control);
    view.set(&ActorRecord::step,static_cast<KinokoActor **>(nullptr));
    view.set(&ActorRecord::step_control,static_cast<ControlRecord *>(nullptr));
    kinoko_native_release_weak(address(parent));
    const auto owner=view.get(&ActorRecord::owner_control);
    view.set(&ActorRecord::owner,static_cast<KinokoActor **>(nullptr));
    view.set(&ActorRecord::owner_control,static_cast<ControlRecord *>(nullptr));
    kinoko_native_release_strong(address(owner));
    kinoko_actor_clear_script(actor);
    // Init now owns its argument-before-callback copies, also on exceptions.
    // Borrow these fields only until that entry has retained both references.
    kinoko_actor_initialize(actor,view.get(&ActorRecord::manager),
        reinterpret_cast<const KinokoOwnedObjectWords *>(view.bytes(&ActorRecord::initial_function)),
        view.get(&ActorRecord::spawn_x),view.get(&ActorRecord::spawn_y),view.get(&ActorRecord::spawn_z),
        reinterpret_cast<const KinokoOwnedObjectWords *>(view.bytes(&ActorRecord::initial_argument)));
    return kinoko_actor_reset_priority(actor,view.get(&ActorRecord::priority));
}
extern "C" KinokoActor *kinoko_actor_assign(KinokoActor *destination,KinokoActor *source) {
    const ActorView out(destination),in(source);
#define COPY(member) copy(out,in,&ActorRecord::member)
    COPY(owner_references); COPY(pool_handle); COPY(priority_entry);
    COPY(registration_flag20); COPY(visible); COPY(release_pending);
    auto *incoming=in.get(&ActorRecord::owner_control);
    auto **slot=in.get(&ActorRecord::owner);
    kinoko_native_add_strong(address(incoming));
    out.set(&ActorRecord::owner,slot);
    auto *previous=out.get(&ActorRecord::owner_control);
    out.set(&ActorRecord::owner_control,incoming);
    kinoko_native_release_strong(address(previous));
    COPY(step);
    incoming=in.get(&ActorRecord::step_control);
    previous=out.get(&ActorRecord::step_control);
    if (incoming!=previous) {
        kinoko_native_add_weak(address(incoming));
        kinoko_native_release_weak(address(previous));
        out.set(&ActorRecord::step_control,incoming);
    }
    COPY(active);
    for (auto member:{&ActorRecord::script_object,&ActorRecord::initial_function,&ActorRecord::initial_argument})
        copy_script(out,in,member);
    COPY(spawn_x); COPY(spawn_y); COPY(spawn_z); COPY(update_vm);
    copy_script(out,in,&ActorRecord::update_environment); copy_script(out,in,&ActorRecord::update_function);
    COPY(collision_vm);
    copy_script(out,in,&ActorRecord::collision_environment); copy_script(out,in,&ActorRecord::collision_function);
    COPY(manager); COPY(sprite_frame); COPY(offset_x); COPY(offset_y); COPY(rotation);
    COPY(scale); COPY(scale_x); COPY(scale_y); COPY(alpha); COPY(red); COPY(green); COPY(blue); COPY(blend);
    COPY(animation); COPY(current_frame); COPY(take); COPY(frame_index); COPY(frame_time); COPY(take_duration);
    COPY(id); COPY(priority); COPY(update_group); COPY(flags); COPY(x); COPY(y); COPY(previous_x); COPY(previous_y);
    COPY(velocity_x); COPY(velocity_y); COPY(parent_velocity_x); COPY(parent_velocity_y);
    COPY(direction); COPY(pitch); COPY(pitch_top); COPY(hits); COPY(hits);
    COPY(crushed); COPY(free_width); COPY(free_height); COPY(collision_group); COPY(collision_mask);
    COPY(callback_group); COPY(callback_mask); COPY(collision_chip); COPY(collision_placement); COPY(collision_index);
    // 46034F copies 340..371, excluding 372..375. Internal pointers stay
    // shallow copies of the source; do not invent rebasing to destination.
    COPY(inline_slots); COPY(bounds_anchor_x); COPY(bounds_anchor_y); COPY(unknown360);
    COPY(initial); COPY(local_bounds); COPY(world_bounds); COPY(world_bounds);
    COPY(previous_bounds); COPY(previous_bounds); COPY(collision_flags); COPY(unknown476);
    COPY(collision_scan_cache); COPY(chip_cache_storage);
#undef COPY
    return destination;
}
extern "C" int32_t kinoko_actor_assign_instance(int32_t destination,int32_t source) {
    return address(kinoko_actor_assign(kinoko::legacy::pointer<KinokoActor>(destination),kinoko::legacy::pointer<KinokoActor>(source)));
}
