#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_abi.h"
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

// Views do not turn legacy malloc/byte buffers into live C++ objects. All field
// offsets are from the recovered 0x220-byte Actor, not a new engine layout.
class ActorView final {
public:
    static constexpr size_t size = 0x220;
    enum Offset : size_t {
        vtable = 0, type = 8, owner = 24, owner_control = 28,
        step = 32, step_control = 36, script_object = 44,
        update_callback = 56, collision_callback = 68,
        collision_records = 328, collision_slots = 332,
        inline_slots = 340, inline_records = 376
    };
    explicit ActorView(int32_t actor) : bytes_(pointer<unsigned char>(actor)) {}
    int32_t at(size_t offset) const noexcept { return address(bytes_ + offset); }
    int32_t get(size_t offset) const noexcept {
        int32_t result;
        std::memcpy(&result, bytes_ + offset, sizeof(result));
        return result;
    }
    void set(size_t offset, int32_t value) const noexcept {
        std::memcpy(bytes_ + offset, &value, sizeof(value));
    }
    void clear() const noexcept { std::memset(bytes_, 0, size); }
private:
    unsigned char* bytes_;
};

// These are boost-style shared/weak control counts, NOT Squirrel reference
// counts. VM values continue to use the vendored source's external references.
class ControlView final {
public:
    explicit ControlView(int32_t control) : bytes_(pointer<unsigned char>(control)) {}
    LONG add_strong(LONG delta) const noexcept {
        return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(bytes_ + 4), delta);
    }
    LONG add_weak(LONG delta) const noexcept {
        return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(bytes_ + 8), delta);
    }
    int32_t get(size_t offset) const noexcept {
        int32_t result;
        std::memcpy(&result, bytes_ + offset, sizeof(result));
        return result;
    }
    void set(size_t offset, int32_t value) const noexcept {
        std::memcpy(bytes_ + offset, &value, sizeof(value));
    }
private:
    unsigned char* bytes_;
};

int32_t table_entry(int32_t table, size_t offset) noexcept {
    int32_t entry;
    std::memcpy(&entry, pointer<unsigned char>(table) + offset, sizeof(entry));
    return entry;
}
void call_control_method(int32_t control, int32_t table, size_t offset) {
    if (table) {
        const auto method = table_entry(table, offset);
        if (method) retdec_call_thiscall0(pointer(control), pointer(method));
    }
}

void raw_set_step(const ActorView& actor, const HSQOBJECT& step) {
    auto* vm = current_vm();
    StackTop restore(vm);
    ObjectView(actor.at(ActorView::script_object)).push(vm);
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
    ActorView view(actor);
    view.clear();
    view.set(ActorView::vtable, kinoko_actor_vtable());
    view.set(ActorView::type, 1);
    constexpr std::array<size_t, 7> objects{44, 56, 68, 96, 108, 124, 136};
    for (auto offset : objects)
        ObjectView(view.at(offset)).initialize(kinoko_squirrel_object_vtable());
    view.set(ActorView::collision_slots, view.at(ActorView::inline_slots));
    view.set(ActorView::collision_records, view.at(ActorView::inline_records));
    return actor;
}

extern "C" void retdec_actor_release_weak(int32_t control) {
    if (!control) return;
    const ControlView view(control);
    if (view.add_weak(-1) != 1) return;
    const auto table = view.get(0);
    if (table == kinoko_actor_control_vtable())
        std::free(pointer(control));
    else
        call_control_method(control, table, 8);
}

extern "C" void retdec_release_squirrel_object(int32_t control) {
    if (!control) return;
    const ControlView view(control);
    if (view.add_strong(-1) != 1) return;
    const auto table = view.get(0);
    if (table == kinoko_actor_control_vtable()) {
        std::free(pointer(view.get(12)));
        view.set(12, 0);
        retdec_actor_release_weak(control);
        return;
    }
    call_control_method(control, table, 4);
    if (view.add_weak(-1) == 1)
        call_control_method(control, table, 8);
}

extern "C" int32_t function_45e460_this(int32_t actor) {
    if (!actor) return 0;
    ActorView view(actor);
    // Keep the established source-backed reset/destructor diagnostic boundary.
    // Do not compile away trace calls globally or clear VM internals.
    function_4a9570_this(view.at(ActorView::update_callback));
    function_4a9570_this(view.at(ActorView::collision_callback));
    kinoko_actor_clear_script(actor);
    const auto parent_control = view.get(ActorView::step_control);
    view.set(ActorView::step, 0);
    view.set(ActorView::step_control, 0);
    retdec_actor_release_weak(parent_control);
    const auto owner_control = view.get(ActorView::owner_control);
    view.set(ActorView::owner, 0);
    view.set(ActorView::owner_control, 0);
    retdec_release_squirrel_object(owner_control);
    constexpr std::array<size_t, 7> objects{136, 124, 108, 96, 68, 56, 44};
    for (auto offset : objects)
        kinoko_squirrel_object_destroy(view.at(offset), address(current_vm()),
            kinoko_squirrel_object_vtable());
    return actor;
}

extern "C" int32_t function_4606d0_this(int32_t actor, int32_t owned_object) {
    if (!actor || !owned_object) return 0;
    const ActorView view(actor);
    const ObjectView object(owned_object);
    const auto value = object.value();
    if (value._type != OT_INSTANCE && value._type != OT_WEAKREF) {
        view.set(ActorView::step, 0);
        const auto previous = view.get(ActorView::step_control);
        view.set(ActorView::step_control, 0);
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
            const ActorView target(receiver);
            view.set(ActorView::step, target.get(ActorView::owner));
            const auto next = target.get(ActorView::owner_control);
            const auto previous = view.get(ActorView::step_control);
            if (next != previous) {
                if (next) ControlView(next).add_weak(1);
                retdec_actor_release_weak(previous);
                view.set(ActorView::step_control, next);
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
