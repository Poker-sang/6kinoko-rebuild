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
static_assert(sizeof(KinokoOwnedObjectWords) == sizeof(ObjectStorage));
static_assert(std::is_trivially_copyable_v<KinokoOwnedObjectWords>);
static_assert(std::is_trivially_destructible_v<KinokoOwnedObjectWords>);

HSQUIRRELVM current_vm() noexcept { return reinterpret_cast<HSQUIRRELVM>(g644); }

using namespace kinoko::actor;

// Boost-style native strong/weak counts remain separate from external VM refs.
// Count slots have the original Win32 alignment required by Interlocked APIs.
class ControlView final {
    kinoko::native::RecordView<ControlRecord> view_;
public:
    explicit ControlView(int32_t control) : view_(pointer(control)) {}
    LONG add_strong(LONG delta) const noexcept {
        return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(
            view_.bytes(&ControlRecord::strong)), delta);
    }
    LONG add_weak(LONG delta) const noexcept {
        return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(
            view_.bytes(&ControlRecord::weak)), delta);
    }
    Address get(Address ControlRecord::* member) const noexcept {
        return view_.get(member);
    }
    void set(Address ControlRecord::* member, Address value) const noexcept {
        view_.set(member, value);
    }
};

void call_control_method(int32_t control, Address table,
                         Address ControlTable::* member) {
    if (!table) return;
    const kinoko::native::RecordView<ControlTable> methods(pointer(static_cast<int32_t>(table)));
    const auto entry = methods.get(member);
    if (!entry) return;
    // Exact zero-argument x86 member ABI, not a C register-context simulator.
    using Method = void (__thiscall*)(void*);
    reinterpret_cast<Method>(pointer(static_cast<int32_t>(entry)))(pointer(control));
}

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

extern "C" int32_t function_45e300_this(int32_t actor) {
    if (!actor) return 0;
    const ActorView view(pointer(actor));
    view.clear();
    view.set(&ActorRecord::vtable, static_cast<Address>(kinoko_actor_vtable()));
    view.set(&ActorRecord::type, int32_t{1});
    for (auto member : script_members)
        ObjectView(view.bytes(member)).initialize(kinoko_squirrel_object_vtable());
    view.set(&ActorRecord::collision_slots, static_cast<Address>(address(view.bytes(&ActorRecord::inline_slots))));
    view.set(&ActorRecord::collision_records, static_cast<Address>(address(view.bytes(&ActorRecord::initial))));
    return actor;
}

extern "C" void retdec_actor_release_weak(int32_t control) {
    if (!control) return;
    const ControlView view(control);
    if (view.add_weak(-1) != 1) return;
    const auto table = view.get(&ControlRecord::vtable);
    if (table == kinoko_actor_control_vtable())
        std::free(pointer(control));
    else
        call_control_method(control, table, &ControlTable::destroy);
}

extern "C" void retdec_release_squirrel_object(int32_t control) {
    if (!control) return;
    const ControlView view(control);
    if (view.add_strong(-1) != 1) return;
    const auto table = view.get(&ControlRecord::vtable);
    if (table == kinoko_actor_control_vtable()) {
        std::free(pointer(view.get(&ControlRecord::allocation)));
        view.set(&ControlRecord::allocation, 0);
        retdec_actor_release_weak(control);
        return;
    }
    call_control_method(control, table, &ControlTable::dispose);
    if (view.add_weak(-1) == 1)
        call_control_method(control, table, &ControlTable::destroy);
}

extern "C" int32_t function_45e460_this(int32_t actor) {
    if (!actor) return 0;
    const ActorView view(pointer(actor));
    // Keep the established source-backed reset/destructor diagnostic boundary.
    // Do not compile away trace calls globally or clear VM internals.
    function_4a9570_this(address(view.bytes(&ActorRecord::update_callback)));
    function_4a9570_this(address(view.bytes(&ActorRecord::collision_callback)));
    kinoko_actor_clear_script(actor);
    const auto parent_control = view.get(&ActorRecord::step_control);
    view.set(&ActorRecord::step, Address{0});
    view.set(&ActorRecord::step_control, Address{0});
    retdec_actor_release_weak(parent_control);
    const auto owner_control = view.get(&ActorRecord::owner_control);
    view.set(&ActorRecord::owner, Address{0});
    view.set(&ActorRecord::owner_control, Address{0});
    retdec_release_squirrel_object(owner_control);
    for (auto member = script_members.rbegin(); member != script_members.rend(); ++member)
        kinoko_squirrel_object_destroy(address(view.bytes(*member)), address(current_vm()),
            kinoko_squirrel_object_vtable());
    return actor;
}

extern "C" int32_t function_4606d0_this(int32_t actor, int32_t owned_object) {
    if (!actor || !owned_object) return 0;
    const ActorView view(pointer(actor));
    const ObjectView object(owned_object);
    const auto value = object.value();
    if (value._type != OT_INSTANCE && value._type != OT_WEAKREF) {
        view.set(&ActorRecord::step, Address{0});
        const auto previous = view.get(&ActorRecord::step_control);
        view.set(&ActorRecord::step_control, Address{0});
        retdec_actor_release_weak(previous);
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
                if (next) ControlView(next).add_weak(1);
                retdec_actor_release_weak(previous);
                view.set(&ActorRecord::step_control, next);
            }
        }
        raw_set_step(view, value);
    }
    // The caller transferred one EXTERNAL reference with its by-value words.
    // Consume it exactly once, including on failed native-instance lookup.
    return kinoko_squirrel_object_destroy(owned_object, address(current_vm()),
        kinoko_squirrel_object_vtable());
}

extern "C" int32_t __fastcall function_45e460(int32_t actor, void*) {
    return function_45e460_this(actor);
}
extern "C" int32_t __fastcall function_45f0c0(int32_t actor, void*, unsigned char flags) {
    function_45e460_this(actor);
    if (flags & 1) _3f__3f_3_40_YAXPAX_40_Z(pointer<int32_t>(actor));
    return actor;
}
extern "C" int32_t __fastcall function_4606d0(int32_t actor, void*, KinokoOwnedObjectWords object) {
    return function_4606d0_this(actor, address(&object));
}
