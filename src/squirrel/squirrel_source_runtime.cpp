#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_vm_lifecycle.h"
#include "kinoko/upstream_bindings.hpp"
#include <atomic>
#include <cstddef>
#include <cstring>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"

namespace {
template<class T> T* pointer(int32_t value) noexcept {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(static_cast<uint32_t>(value)));
}
int32_t address(const void* value) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
static_assert(sizeof(void*) == 4 && sizeof(SQObjectPtr) == 8);
static_assert(sizeof(SQVM) == 168 && sizeof(SQVM::CallInfo) == 48);
static_assert(offsetof(SQVM, _sharedstate) == 140 && offsetof(SQVM, ci) == 132);

thread_local kinoko_sq_context_exchange exchange_receiver = nullptr;
std::atomic<int32_t> source_vm_vtable{0};

class ReceiverScope final {
public:
    explicit ReceiverScope(SQVM* vm) noexcept : exchange_(exchange_receiver),
        previous_(exchange_ ? exchange_(vm) : 0) {}
    ~ReceiverScope() { if (exchange_) exchange_(previous_); }
    ReceiverScope(const ReceiverScope&) = delete;
    ReceiverScope& operator=(const ReceiverScope&) = delete;
private:
    kinoko_sq_context_exchange exchange_;
    SQVM* previous_;
};

void remember_vm(SQVM* vm) noexcept {
    if (!vm) return;
    int32_t vtable = 0;
    std::memcpy(&vtable, vm, sizeof(vtable));
    source_vm_vtable.store(vtable, std::memory_order_relaxed);
}
}

extern "C" void kinoko_sq_set_context_exchange(kinoko_sq_context_exchange exchange) {
    exchange_receiver = exchange;
}

// The sole adaptation in vendored SQVM::CallNative. Preserve the callback's
// actual VM even when a source coroutine enters a recovered game callback.
// Standalone source VMs have no legacy receiver and simply call the function.
SQInteger kinoko_squirrel_invoke_native(HSQUIRRELVM vm, SQFUNCTION function) {
    ReceiverScope scope(vm);
    return function(vm);
}

extern "C" int32_t kinoko_sq_source_vm_vtable(void) {
    return source_vm_vtable.load(std::memory_order_relaxed);
}
extern "C" SQVM* kinoko_sq_open(int32_t stack_size) {
    auto* vm = sq_open(stack_size);
    remember_vm(vm);
    return vm;
}
extern "C" SQVM* kinoko_sq_construct_vm(void* storage, SQSharedState* state) {
    if (!storage) return 0;
    auto* vm = new (storage) SQVM(state);
    remember_vm(vm);
    return vm;
}
extern "C" SQVM* kinoko_sq_create_thread(SQVM* parent, int32_t stack_size) {
    auto* vm = parent;
    ReceiverScope scope(vm);
    auto* child = sq_newthread(vm, stack_size);
    remember_vm(child);
    return child;
}
extern "C" int32_t kinoko_sq_call(SQVM* vm, int32_t nargs, int32_t retval, int32_t raiseerror) {
    auto* machine = vm;
    ReceiverScope scope(machine);
    return sq_call(machine, nargs, retval != 0, raiseerror != 0);
}
extern "C" int32_t kinoko_sq_call_object(SQVM* vm, SQObjectPtr* closure, int32_t nargs,
    int32_t stackbase, SQObjectPtr* result, int32_t raiseerror) {
    auto* machine = vm;
    ReceiverScope scope(machine);
    return machine->Call(*closure, nargs, stackbase,
                         *result, raiseerror != 0);
}
extern "C" int32_t kinoko_sq_execute(SQVM* vm, SQObjectPtr* closure, int32_t target,
    int32_t nargs, int32_t stackbase, SQObjectPtr* result, int32_t raiseerror, int32_t resume_vm) {
    auto* machine = vm;
    ReceiverScope scope(machine);
    SQObjectPtr unused;
    // The recovered resume flag is boolean, NOT the source ExecutionType:
    // source 1 means generator resume; source 2 means suspended VM resume.
    return machine->Execute(resume_vm ? unused : *closure,
        resume_vm ? machine->_top : target, resume_vm ? -1 : nargs,
        resume_vm ? -1 : stackbase, *result, raiseerror != 0,
        resume_vm ? SQVM::ET_RESUME_VM : SQVM::ET_CALL);
}
extern "C" int32_t kinoko_sq_call_native(SQVM* vm, SQNativeClosure* closure, int32_t nargs,
    int32_t stackbase, SQObjectPtr* result, unsigned char* suspended) {
    // The original bool out-parameter occupies one byte, not an SQInteger.
    if (suspended) *suspended = 0;
    if (!vm || !closure) return 0;
    auto* machine = vm;
    ReceiverScope scope(machine);
    bool did_suspend = false;
    const bool success = machine->CallNative(closure, nargs,
        stackbase, *result, did_suspend);
    if (suspended) *suspended = did_suspend ? 1 : 0;
    return success;
}
extern "C" SQVM* kinoko_sq_pop(SQVM* vm, int32_t count) {
    if (vm && count > 0) {
        auto* machine = vm;
        // Preserve the recovered caller's underflow guard at the ABI boundary;
        // references and destruction use the source Pop operation itself.
        if (count > machine->_top) count = machine->_top;
        machine->Pop(count);
    }
    return vm;
}
extern "C" SQObjectPtr* kinoko_sq_get_up(SQVM* vm, int32_t index) {
    return vm ? &vm->GetUp(index) : 0;
}
extern "C" SQObjectPtr* kinoko_sq_get_at(SQVM* vm, int32_t index) {
    return vm ? &vm->GetAt(index) : 0;
}

// Keep shared-state layout knowledge in the actual source type, not a magic
// +140 dereference in the decompiled embedding layer.
extern "C" SQSharedState* kinoko_sq_shared_state(SQVM* vm) {
    return vm ? vm->_sharedstate : 0;
}

extern "C" void kinoko_sq_delete_shared_state(SQSharedState* state) {
    // 49C350 + 4985B0: the state destructor itself finalizes the root VM.
    if (state) sq_delete(state, SQSharedState);
}

extern "C" int32_t kinoko_sq_noop_constructor(int32_t /* vm */) {
    // The original embedding registered 4A1760 for these classes; it neither
    // allocates state nor pushes a return value. This is not a new constructor.
    return 0;
}

extern "C" SQRefCounted* __fastcall kinoko_sq_delete_refcounted(SQRefCounted* object, void* /* unused_edx */, int32_t flags) {
    if (!object) return 0;
    auto* value = object;
    // Qualified base destruction is intentional. The old vtable entry did
    // not dispatch a second derived destructor; it cleared the weak reference.
    value->SQRefCounted::~SQRefCounted();
    if ((flags & 1) != 0) sq_free(value, 0);
    return object;
}

extern "C" int32_t kinoko_sq_compile_act_source(SQVM* id, const char *text,
    int32_t length, const int32_t environment[2]) {
    auto *vm = id;
    ReceiverScope receiver(vm);
    HSQOBJECT object;
    std::memcpy(&object, environment, sizeof(object));
    return kinoko::script::upstream::sqrat_compile_and_run(vm, text, length, object,
        [](HSQUIRRELVM target, SQInteger count, SQBool result, SQBool errors) -> SQRESULT {
            return kinoko_sq_call(target, count, result, errors);
        });
}

extern "C" KinokoVmStackSnapshot kinoko_sq_stack_snapshot(const SQVM *vm) {
    return {vm->_stack._vals, vm->_top, vm->_stackbase};
}
