#include "kinoko/actor_lifecycle.h"
#include "kinoko/squirrel_host_compat.h"
#include "squirrel_bridge_test_support.hpp"
#include <cstdlib>
#include "kinoko/boost_control.hpp"
#include <boost/smart_ptr/detail/sp_counted_base_w32.hpp>

using namespace bridge_test;
using kinoko::script::ObjectStorage;
using kinoko::script::ObjectView;
namespace {
ObjectStorage step_key{};
int clears = 0, deletes = 0, disposes = 0, destroys = 0, userdata_releases = 0;
int32_t deleted_address = 0, active_actor = 0;
using SetStep = int32_t(__thiscall*)(int32_t, KinokoOwnedObjectWords);
using Destroy = int32_t(__thiscall*)(int32_t, unsigned char);
using InPlace = int32_t(__thiscall*)(int32_t);
}
extern "C" {
char* g644 = nullptr;
char g560 = 0;
int32_t kinoko_squirrel_object_vtable(void) { return 0x12345678; }
int32_t kinoko_actor_vtable(void) { return 0x14141414; }
int32_t kinoko_actor_step_key(void) { return address(&step_key); }
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
void _3f__3f_3_40_YAXPAX_40_Z(int32_t* allocation) {
    ++deletes; deleted_address = address(allocation); std::free(allocation);
}
// The VM is real. This test substitutes only the already reconstructed callback
// clearing service; the full service is exercised by stage_native_contract.
int32_t kinoko_actor_clear_script(KinokoActor *receiver_actor) {
    const auto actor=address(receiver_actor);
    require(ObjectView(actor + 56).value()._type == OT_NULL, "update reset before clear");
    require(ObjectView(actor + 68).value()._type == OT_NULL, "collision reset before clear");
    ++clears; return actor;
}
}
namespace {
int32_t exchange_vm(int32_t value) {
    const auto previous = address(g644); g644 = pointer<char>(value); receiver = value; return previous;
}
struct ControlFixture : boost::detail::sp_counted_base {
    ControlFixture(int strong, int weak) {
        for (int i = 1; i < strong; ++i) add_ref_copy();
        for (int i = 1; i < weak; ++i) weak_add_ref();
    }
    void dispose() override { ++disposes; }
    void destroy() override { ++destroys; }
    void* get_deleter(const boost::detail::sp_typeinfo&) override { return nullptr; }
    void* data() { return this; }
    int32_t operator[](int slot) const {
        return load<int32_t>(reinterpret_cast<const unsigned char*>(this) + 4 * slot);
    }
};
SQInteger release_userdata(SQUserPointer, SQInteger) { ++userdata_releases; return 0; }

KinokoOwnedObjectWords owned(HSQUIRRELVM vm, HSQOBJECT value) {
    sq_addref(vm, &value);
    return {kinoko_squirrel_object_vtable(), static_cast<int32_t>(value._type), kinoko::script::data_bits(value)};
}
void invoke(HSQUIRRELVM vm, int32_t actor, HSQOBJECT value) {
    const volatile uint32_t before = 0x55aa33cc, after = 0xcc33aa55;
    reinterpret_cast<SetStep>(kinoko_actor_set_step_method)(actor, owned(vm, value));
    require(before == 0x55aa33cc && after == 0xcc33aa55, "caller canaries");
}
void single_external_owner(HSQUIRRELVM vm, HSQOBJECT value) {
    sq_pushobject(vm, value); // Internal lifetime while the external counter is probed.
    const auto last = sq_release(vm, &value);
    sq_addref(vm, &value); sq_pop(vm, 1);
    require(last == SQTrue, "transferred argument consumed exactly one external reference");
}
void initialize_key(HSQUIRRELVM vm) {
    ObjectView(&step_key).initialize(kinoko_squirrel_object_vtable());
    sq_pushstring(vm, "step", -1); ObjectView(&step_key).capture(vm, -1); sq_pop(vm, 1);
}
void controls() {
    ControlFixture custom(2, 2);
    auto old_disposes = disposes, old_destroys = destroys;
    kinoko_native_release_strong(address(custom.data()));
    require(custom[1] == 1 && disposes == old_disposes, "nonfinal strong release");
    kinoko_native_release_strong(address(custom.data()));
    require(custom[1] == 0 && custom[2] == 1 && disposes == old_disposes + 1 && destroys == old_destroys,
        "dispose before implicit weak release");
    kinoko_native_release_weak(address(custom.data()));
    require(custom[2] == 0 && destroys == old_destroys + 1, "last custom weak destruction");
    auto* allocation = std::malloc(4); require(allocation != nullptr, "owner-slot allocation");
    auto* control = kinoko::native::upstream::create_owner_control(allocation);
    require(control != nullptr, "source control allocation");
    kinoko_native_add_weak(address(control));
    kinoko_native_release_strong(address(control));
    require(control->use_count() == 0 && kinoko::native::upstream::allocation(control) == nullptr,
        "source control frees owned slot, not Actor");
    kinoko_native_release_weak(address(control));
    kinoko_native_release_weak(0); kinoko_native_release_strong(0);
}
void initialize_table(HSQUIRRELVM vm, int32_t actor) {
    sq_newtable(vm); sq_pushstring(vm, "step", -1); sq_pushnull(vm);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "initial step slot");
    ObjectView(actor + 44).capture(vm, -1); sq_pop(vm, 1);
}
SQInteger script_step(HSQUIRRELVM vm) {
    require(g644 == reinterpret_cast<char*>(vm), "native callback receives child VM context");
    HSQOBJECT argument; sq_getstackobj(vm, 2, &argument);
    invoke(vm, active_actor, argument);
    return 0;
}
void set_slot(HSQUIRRELVM vm, const char* name, HSQOBJECT value) {
    Top restore(vm); sq_pushroottable(vm); sq_pushstring(vm, name, -1); sq_pushobject(vm, value);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "publish fixture slot");
}
void erase_slot(HSQUIRRELVM vm, const char* name) {
    Top restore(vm); sq_pushroottable(vm); sq_pushstring(vm, name, -1);
    require(SQ_SUCCEEDED(sq_deleteslot(vm, -2, SQFalse)), "erase fixture slot");
}
void lifecycle(HSQUIRRELVM vm) {
    const auto base = sq_gettop(vm);
    std::array<unsigned char, 0x222> bytes; bytes.fill(0xa7);
    std::array<unsigned char, 0x220> target{};
    const auto actor = address(bytes.data() + 1), other = address(target.data());
    require((int32_t)(intptr_t)(kinoko_actor_construct((KinokoActor *)(intptr_t)(0))) == 0 && (int32_t)(intptr_t)(kinoko_actor_dispose((KinokoActor *)(intptr_t)(0))) == 0, "null lifecycle");
    require((int32_t)(intptr_t)(kinoko_actor_construct((KinokoActor *)(intptr_t)(actor))) == actor, "construct unaligned Actor view");
    require(bytes.front() == 0xa7 && bytes.back() == 0xa7, "Actor allocation boundaries");
    require(load<int32_t>(bytes.data() + 1) == kinoko_actor_vtable(), "original Actor vtable identity");
    require(load<int32_t>(bytes.data() + 9) == 1, "original Actor type");
    require(load<int32_t>(bytes.data() + 329) == actor + 376 && load<int32_t>(bytes.data() + 333) == actor + 340,
        "inline collision storage pointers");
    for (auto offset : {44, 56, 68, 96, 108, 124, 136})
        require(ObjectView(actor + offset).value()._type == OT_NULL, "all seven wrappers initialized");
    // Original 45E300 deliberately leaves non-constructor fields untouched.
    for (auto offset : {4,20,21,23,40,148,152,240,336,372,424,540})
        require(bytes[1+offset]==0xa7,"constructor preserves bytes reserved for Init or previous storage");
    require(bytes[1+22]==0,"constructor clears deferred release");
    for (int offset=340;offset<372;++offset) require(bytes[1+offset]==0,"constructor clears inline collision storage only");
    initialize_table(vm, actor);
    ControlFixture control(2, 3);
    store(target.data() + 24, int32_t{0x12348765}); store(target.data() + 28, address(control.data()));
    Pair instance(vm), weak(vm);
    sq_newclass(vm, SQFalse); require(SQ_SUCCEEDED(sq_createinstance(vm, -1)), "real source instance");
    sq_setinstanceup(vm, -1, pointer(other)); instance.capture(); sq_pop(vm, 1);
    for (int i = 0; i < 10000; ++i) invoke(vm, actor, instance.get());
    require(load<int32_t>(bytes.data() + 33) == 0x12348765 && load<int32_t>(bytes.data() + 37) == address(control.data()),
        "SetStep retains original native owner/control");
    require(control[2] == 4, "self-rebinding does not duplicate weak ownership");
    single_external_owner(vm, instance.get());
    instance.push(); sq_weakref(vm, -1); weak.capture(); sq_pop(vm, 1);
    sq_throwerror(vm, "old error"); invoke(vm, actor, weak.get());
    require(control[2] == 4, "weakref does not invent native receiver resolution");
    sq_getlasterror(vm); require(sq_gettype(vm, -1) == OT_NULL, "failed instance lookup resets last error"); sq_pop(vm, 1);
    single_external_owner(vm, weak.get());
    HSQOBJECT null_value; sq_resetobject(&null_value); invoke(vm, actor, null_value);
    require(control[2] == 3 && load<int32_t>(bytes.data() + 33) == 0 && load<int32_t>(bytes.data() + 37) == 0,
        "null clears native weak link");
    ObjectStorage argument{}; ObjectView(&argument).initialize(kinoko_squirrel_object_vtable());
    sq_newuserdata(vm, 4); sq_setreleasehook(vm, -1, release_userdata);
    ObjectView(&argument).capture(vm, -1); sq_pop(vm, 1);
    const auto released = userdata_releases;
    require(kinoko_actor_set_step_owned((KinokoActor *)(intptr_t)(actor), (KinokoOwnedObjectWords *)(intptr_t)(address(&argument))) == address(&argument) + 4, "explicit SetStep return address");
    require(userdata_releases == released + 1 && argument.value._type == OT_NULL, "wrong-type owned argument released");
    require(kinoko_actor_set_step_owned((KinokoActor *)(intptr_t)(0), (KinokoOwnedObjectWords *)(intptr_t)(0)) == 0, "null arguments retain old no-op");
    set_slot(vm, "stepFixtureValue", instance.get());
    sq_pushroottable(vm); sq_pushstring(vm, "stepFixtureCall", -1); sq_newclosure(vm, script_step, 0);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "publish native callback"); sq_pop(vm, 1);
    active_actor = actor;
    sq_newthread(vm, 64); HSQUIRRELVM child = nullptr; sq_getthread(vm, -1, &child);
    evaluate(child, "stepFixtureCall(stepFixtureValue);\nstepFixtureCall(null);\n");
    require(g644 == reinterpret_cast<char*>(vm) && control[2] == 3, "child callback restores parent and ownership");
    sq_pop(vm, 1); erase_slot(vm, "stepFixtureCall"); erase_slot(vm, "stepFixtureValue");
    invoke(vm, actor, instance.get());
    const auto cleared = clears;
    require(reinterpret_cast<InPlace>(kinoko_actor_dispose_method)(actor) == actor, "in-place entry returns receiver");
    require(clears == cleared + 1 && control[2] == 3, "destructor releases last step link");
    for (auto offset : {44, 56, 68, 96, 108, 124, 136})
        require(ObjectView(actor + offset).value()._type == OT_NULL, "all wrappers destroyed");
    require(bytes.front() == 0xa7 && bytes.back() == 0xa7, "destruction boundaries");
    auto* allocation = std::malloc(0x220); require(allocation != nullptr, "Actor allocation");
    const auto allocated_actor = address(allocation), deleted = deletes;
    (int32_t)(intptr_t)(kinoko_actor_construct((KinokoActor *)(intptr_t)(allocated_actor)));
    require(reinterpret_cast<Destroy>(kinoko_actor_delete_method)(allocated_actor, 1) == allocated_actor, "deleting entry returns receiver");
    require(deletes == deleted + 1 && deleted_address == allocated_actor, "deleting entry frees actual receiver, never g1224");
    top(vm, base, "lifecycle stack balanced");
}
}
int main() {
    try {
        for (int pass = 0; pass < 8; ++pass) {
            Machine machine; auto* vm = machine.get();
            g644 = reinterpret_cast<char*>(vm); receiver = address(vm);
            kinoko_sq_set_context_exchange(exchange_vm);
            initialize_key(vm); controls(); lifecycle(vm);
            ObjectView(&step_key).release(vm); ObjectView(&step_key).reset();
            top(vm, 0, "root stack balanced"); g644 = nullptr;
            std::printf("actor pass %d: 10000 thiscalls, real VM/child, ownership, controls and canaries OK\n", pass + 1);
        }
        return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "FAIL: %s\n", error.what()); return 1; }
}
