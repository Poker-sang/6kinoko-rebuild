#include "kinoko/actor_lifecycle.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_object.h"
#include <windows.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <type_traits>

extern "C" {
extern char* g644;
void _3f__3f_3_40_YAXPAX_40_Z(int32_t* allocation);
}

namespace {
using namespace kinoko::script;
inline char*& current_vm_storage = g644;
static_assert(sizeof(KinokoOwnedObjectWords) == sizeof(ObjectStorage));
static_assert(std::is_trivially_copyable_v<KinokoOwnedObjectWords>);
static_assert(std::is_trivially_destructible_v<KinokoOwnedObjectWords>);

HSQUIRRELVM current_vm() noexcept { return reinterpret_cast<HSQUIRRELVM>(current_vm_storage); }

using namespace kinoko::actor;

void raw_set_step(const ActorView& actor, const HSQOBJECT& step) {
    auto* vm = current_vm();
    StackTop restore(vm);
    ObjectView(actor.bytes(&ActorRecord::script_object)).push(vm);
    ObjectView(kinoko_actor_step_key()).push(vm);
    sq_pushobject(vm, step);
    // Preserve raw lookup and the original ignored failure result. There is no
    // second delegated lookup or diagnostic script execution here.
    sq_rawset(vm, -3);
}

int32_t instance_pointer(const ObjectView& object) {
    auto* vm = current_vm();
    StackTop restore(vm);
    object.push(vm);
    SQUserPointer receiver = nullptr;
    if (SQ_FAILED(sq_getinstanceup(vm, -1, &receiver, nullptr))) {
        // This is the old 4A9B40 contract, including its last-error reset.
        sq_reseterror(vm);
        return 0;
    }
    return address(receiver);
}
} // namespace

extern "C" KinokoActor *kinoko_actor_construct(KinokoActor *actor) {
    if (!actor) return 0;
    const ActorView view(actor);
    // 45E300 initializes only these fields; recycled storage retains all
    // other bytes until Init writes them. Never clear the entire Actor.
    view.set(&ActorRecord::pool_handle, uint32_t{0});
    view.set(&ActorRecord::priority_entry, static_cast<void *>(nullptr));
    view.set(&ActorRecord::owner, static_cast<KinokoActor **>(nullptr));
    view.set(&ActorRecord::owner_control, static_cast<ControlRecord *>(nullptr));
    view.set(&ActorRecord::step, static_cast<KinokoActor **>(nullptr));
    view.set(&ActorRecord::step_control, static_cast<ControlRecord *>(nullptr));
    view.set(&ActorRecord::update_vm, static_cast<SQVM *>(nullptr));
    view.set(&ActorRecord::collision_vm, static_cast<SQVM *>(nullptr));
    view.set(&ActorRecord::initial, InitialData{});
    view.set(&ActorRecord::inline_slots, std::array<unsigned char,12>{});
    view.set(&ActorRecord::bounds_anchor_x, 0.0f);
    view.set(&ActorRecord::bounds_anchor_y, 0.0f);
    view.set(&ActorRecord::unknown360, std::array<unsigned char,12>{});
    view.set(&ActorRecord::release_pending, uint8_t{0});
    view.set(&ActorRecord::vtable, pointer<const void>(kinoko_actor_vtable()));
    view.set(&ActorRecord::owner_references, int32_t{1});
    for (auto member : script_members)
        ObjectView(view.bytes(member)).initialize(kinoko_squirrel_object_vtable());
    view.set(&ActorRecord::collision_placement, static_cast<const void *>(view.bytes(&ActorRecord::inline_slots)));
    view.set(&ActorRecord::collision_chip, static_cast<const unsigned char *>(view.bytes(&ActorRecord::initial)));
    return actor;
}

extern "C" KinokoActor *kinoko_actor_dispose(KinokoActor *actor) {
    if (!actor) return 0;
    const ActorView view(actor);
    view.set(&ActorRecord::vtable, pointer<const void>(kinoko_actor_vtable()));
    // Keep the established source-backed reset/destructor diagnostic boundary.
    // Do not compile away trace calls globally or clear VM internals.
    kinoko_sqplus_object_reset((void *)(view.bytes(&ActorRecord::initial_function)));
    kinoko_sqplus_object_reset((void *)(view.bytes(&ActorRecord::initial_argument)));
    kinoko_actor_clear_script(actor);
    const auto parent_control = view.get(&ActorRecord::step_control);
    view.set(&ActorRecord::step, static_cast<KinokoActor **>(nullptr));
    view.set(&ActorRecord::step_control, static_cast<ControlRecord *>(nullptr));
    kinoko_native_release_weak(address(parent_control));
    const auto owner_control = view.get(&ActorRecord::owner_control);
    view.set(&ActorRecord::owner, static_cast<KinokoActor **>(nullptr));
    view.set(&ActorRecord::owner_control, static_cast<ControlRecord *>(nullptr));
    kinoko_native_release_strong(address(owner_control));
    for (auto member = script_members.rbegin(); member != script_members.rend(); ++member)
        kinoko_squirrel_object_destroy(address(view.bytes(*member)), address(current_vm()),
            kinoko_squirrel_object_vtable());
    // 45E56C/45E58A are member destructors after the explicit reset above.
    // Release hooks may have installed new links while wrappers were destroyed.
    kinoko_native_release_weak(address(view.get(&ActorRecord::step_control)));
    kinoko_native_release_strong(address(view.get(&ActorRecord::owner_control)));
    return actor;
}

extern "C" int32_t kinoko_actor_set_step_owned(KinokoActor *actor, KinokoOwnedObjectWords *owned_object) {
    if (!actor || !owned_object) return 0;
    const ActorView view(actor);
    const ObjectView object(owned_object);
    const auto value = object.value();
    if (value._type != OT_INSTANCE && value._type != OT_WEAKREF) {
        view.set(&ActorRecord::step, static_cast<KinokoActor **>(nullptr));
        const auto previous = view.get(&ActorRecord::step_control);
        view.set(&ActorRecord::step_control, static_cast<ControlRecord *>(nullptr));
        kinoko_native_release_weak(address(previous));
        ObjectStorage empty{};
        ObjectView(&empty).initialize(kinoko_squirrel_object_vtable());
        raw_set_step(view, empty.value);
        kinoko_squirrel_object_destroy(address(&empty), address(current_vm()),
            kinoko_squirrel_object_vtable());
    } else {
        // Do not invent weakref dereferencing: retain sq_getinstanceup's result
        // and the old distinction between native bookkeeping and script value.
        if (const auto receiver = instance_pointer(object)) {
            const ActorView target(pointer(receiver));
            view.set(&ActorRecord::step, target.get(&ActorRecord::owner));
            const auto next = target.get(&ActorRecord::owner_control);
            const auto previous = view.get(&ActorRecord::step_control);
            if (next != previous) {
                kinoko_native_add_weak(address(next));
                kinoko_native_release_weak(address(previous));
                view.set(&ActorRecord::step_control, next);
            }
        }
        raw_set_step(view, value);
    }
    // The caller transferred one EXTERNAL reference with its by-value words.
    // Consume it exactly once, including on failed native-instance lookup.
    return kinoko_squirrel_object_destroy(address(owned_object), address(current_vm()),
        kinoko_squirrel_object_vtable());
}

extern "C" KinokoActor *__fastcall kinoko_actor_dispose_method(KinokoActor *actor, void*) {
    return kinoko_actor_dispose(actor);
}
extern "C" KinokoActor *__fastcall kinoko_actor_delete_method(KinokoActor *actor, void*, unsigned char flags) {
    kinoko_actor_dispose(actor);
    if (flags & 1) _3f__3f_3_40_YAXPAX_40_Z(reinterpret_cast<int32_t *>(actor));
    return actor;
}
extern "C" int32_t __fastcall kinoko_actor_set_step_method(KinokoActor *actor, void*, KinokoOwnedObjectWords object) {
    return kinoko_actor_set_step_owned(actor, &object);
}

extern "C" void kinoko_actor_set_collision_parent(KinokoActor *actor,KinokoActor *parent) {
    KinokoOwnedObjectWords object{};
    kinoko_sqplus_object_initialize((void *)(&object));
    if (parent) (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(reinterpret_cast<int32_t *>(&object)), (const void *)(ActorView(parent).bytes(&ActorRecord::script_object))));
    kinoko_actor_set_step_owned(actor,&object);
}
