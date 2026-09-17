#include "kinoko/squirrel_value_bridge.h"
#include "kinoko/squirrel_vm_lifecycle.h"

#include <atomic>
#include <cstddef>
#include <cstring>

#include "sqpcheader.h"
#include "sqvm.h"
#include "sqclosure.h"
#include "sqfuncproto.h"

extern "C" {
int32_t kinoko_call_game_vm(int32_t vm, int32_t nargs, int32_t retval, int32_t raiseerror);
int32_t kinoko_resume_game_vm(int32_t vm, int32_t out, int32_t raiseerror);
}
namespace {
SQVM *machine(int32_t p) {
    return reinterpret_cast<SQVM *>(static_cast<uintptr_t>(static_cast<uint32_t>(p)));
}
int32_t address(const void *p) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p));
}
std::atomic<int32_t> source_vm_vtable{0};
static_assert(sizeof(void *) == 4);
static_assert(sizeof(SQVM) == 168);
static_assert(offsetof(SQVM, _sharedstate) == 140);
static_assert(offsetof(SQVM, _suspended) == 148);
static_assert(offsetof(SQVM, _suspended_root) == 152);
static_assert(offsetof(SQVM, _suspended_target) == 156);
static_assert(offsetof(SQVM, _suspended_traps) == 160);
static_assert(offsetof(SQVM, _suspend_varargs) == 164);
}

extern "C" int32_t kinoko_sq_source_vm_vtable(void) {
    return source_vm_vtable.load(std::memory_order_relaxed);
}

// 48A230 / sq_newthread. Allocation, construction, Init, parent-stack ownership
// and failure cleanup all belong to the supplied Squirrel source. Do not
// reconstruct these operations with byte offsets and manual refcount changes.
extern "C" int32_t kinoko_sq_create_thread(int32_t parent, int32_t stack_size) {
    auto *child = sq_newthread(machine(parent), stack_size);
    if (child) {
        int32_t vtable = 0;
        // The Windows x86 compatibility collector compares vtable identities.
        // memcpy reads the ABI word without an aliasing-violating int32_t lvalue.
        std::memcpy(&vtable, child, sizeof(vtable));
        source_vm_vtable.store(vtable, std::memory_order_relaxed);
    }
    return address(child);
}

// 490C40 / SQVM::Suspend, with the original explicit VM receiver.
extern "C" int32_t kinoko_sq_suspend(int32_t vm) {
    return machine(vm)->Suspend();
}

// 48ADB0 / sq_wakeupvm: source ownership and API ordering, current interpreter.
extern "C" int32_t kinoko_sq_wakeup(int32_t vm, int32_t wakeupret,
                                      int32_t retval, int32_t raiseerror) {
    auto *v = machine(vm);
    if (!v->_suspended)
        return sq_throwerror(v, _SC("cannot resume a vm that is not running any code"));
    SQObjectPtr result;
    auto &target = v->GetAt(v->_stackbase + v->_suspended_target);
    if (wakeupret) {
        target = v->GetUp(-1);
        v->Pop();
    } else target.Null();
    if (!kinoko_resume_game_vm(vm, address(&result), raiseerror)) return SQ_ERROR;
    if (retval) v->Push(result);
    return SQ_OK;
}

// 4A2C80 / thread_call. A local owner keeps the child alive across stack moves.
extern "C" int32_t kinoko_sq_thread_call(int32_t vm) {
    auto *v = machine(vm);
    SQObjectPtr object = stack_get(v, 1);
    if (type(object) != OT_THREAD) return sq_throwerror(v, _SC("wrong parameter"));
    auto *thread = _thread(object);
    const auto nargs = sq_gettop(v);
    thread->Push(thread->_roottable);
    for (SQInteger i = 2; i <= nargs; ++i) sq_move(thread, v, i);
    if (SQ_SUCCEEDED(kinoko_call_game_vm(address(thread), nargs, SQTrue, SQFalse))) {
        sq_move(v, thread, -1);
        sq_pop(thread, 1);
        return 1;
    }
    v->_lasterror = thread->_lasterror;
    return SQ_ERROR;
}

// 4A2DF0 / thread_wakeup. In particular, 4A2F23 assigns to the PARENT error.
extern "C" int32_t kinoko_sq_thread_wakeup(int32_t vm) {
    auto *v = machine(vm);
    SQObjectPtr object = stack_get(v, 1);
    if (type(object) != OT_THREAD) return sq_throwerror(v, _SC("wrong parameter"));
    auto *thread = _thread(object);
    const auto state = sq_getvmstate(thread);
    if (state == SQ_VMSTATE_IDLE)
        return sq_throwerror(v, _SC("cannot wakeup a idle thread"));
    if (state == SQ_VMSTATE_RUNNING)
        return sq_throwerror(v, _SC("cannot wakeup a running thread"));
    const SQBool wakeupret = sq_gettop(v) > 1;
    if (wakeupret) sq_move(thread, v, 2);
    if (SQ_SUCCEEDED(kinoko_sq_wakeup(address(thread), wakeupret, SQTrue, SQFalse))) {
        sq_move(v, thread, -1);
        sq_pop(thread, 1);
        if (sq_getvmstate(thread) == SQ_VMSTATE_IDLE) sq_settop(thread, 1);
        return 1;
    }
    sq_settop(thread, 1);
    v->_lasterror = thread->_lasterror;
    return SQ_ERROR;
}

extern "C" int32_t kinoko_sq_thread_status(int32_t vm) {
    auto *v = machine(vm);
    switch (sq_getvmstate(_thread(stack_get(v, 1)))) {
    case SQ_VMSTATE_IDLE: sq_pushstring(v, _SC("idle"), -1); break;
    case SQ_VMSTATE_RUNNING: sq_pushstring(v, _SC("running"), -1); break;
    case SQ_VMSTATE_SUSPENDED: sq_pushstring(v, _SC("suspended"), -1); break;
    }
    return 1;
}

extern "C" int32_t kinoko_sq_newthread(int32_t vm) {
    auto *v = machine(vm);
    const auto size = (_funcproto(_closure(stack_get(v, 2))->_function)->_stacksize << 1) + 2;
    auto *child = machine(kinoko_sq_create_thread(vm, size > 12 ? size : 12));
    if (!child) return SQ_ERROR;
    sq_move(child, v, -2);
    return 1;
}
