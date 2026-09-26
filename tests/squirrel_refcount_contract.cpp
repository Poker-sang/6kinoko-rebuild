#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/legacy_abi.h"
#include "sqpcheader.h"
#include "sqvm.h"
#include <cstdio>
#include <new>
#include <stdexcept>

namespace {
int32_t address(const void* value) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct BaseProbe final : SQRefCounted {
    void Release() override {} // deletion is explicitly under test below
};
void base_destructor(int flags) {
    auto* storage = sq_malloc(sizeof(BaseProbe));
    require(storage != nullptr, "probe allocation");
    auto* value = new (storage) BaseProbe;
    SQWeakRef* weak = value->GetWeakRef(OT_USERDATA);
    ++weak->_uiRef; // keep it alive after its target is destroyed
    const int32_t receiver = address(value);
    require(kinoko_call_thiscall1_result(value,
        reinterpret_cast<void*>(&kinoko_sq_delete_refcounted), flags) == receiver,
        "base destructor receiver/result");
    require(type(weak->_obj) == OT_NULL && weak->_obj._unVal.pRefCounted == nullptr,
        "source destructor must invalidate weak reference");
    weak->Release();
    if (!(flags & 1)) sq_free(storage, sizeof(BaseProbe));
}
void contracts() {
    require(((int32_t)(uintptr_t)kinoko_sq_delete_refcounted((SQRefCounted*)(uintptr_t)(0), nullptr, 1)) == 0, "null base destructor");
    require(((int32_t)(uintptr_t)kinoko_sq_shared_state((SQVM*)(uintptr_t)(0))) == 0, "null VM shared-state query");
    for (int i = 0; i < 64; ++i) {
        base_destructor(0);
        base_destructor(1);
    }
    HSQUIRRELVM vm = sq_open(32);
    require(vm != nullptr, "VM allocation");
    require(((int32_t)(uintptr_t)kinoko_sq_shared_state((SQVM*)(uintptr_t)(address(vm)))) == address(vm->_sharedstate), "source shared state");
    HSQUIRRELVM child = sq_newthread(vm, 16);
    require(child != nullptr, "thread allocation");
    require(((int32_t)(uintptr_t)kinoko_sq_shared_state((SQVM*)(uintptr_t)(address(child)))) == ((int32_t)(uintptr_t)kinoko_sq_shared_state((SQVM*)(uintptr_t)(address(vm)))),
        "thread must share parent state");
    const SQInteger top = sq_gettop(vm);
    require(kinoko_sq_noop_constructor((SQVM*)(uintptr_t)(address(vm))) == 0 && sq_gettop(vm) == top,
        "embedding no-op constructor must leave the stack unchanged");
    sq_close(vm);
}
}
int main() {
    try {
        contracts();
        std::puts("PASS: source base destruction, weak references and VM shared state");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Squirrel ownership contract failed: %s\n", error.what());
        return 1;
    }
}
