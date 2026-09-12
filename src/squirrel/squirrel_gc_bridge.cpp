#include "kinoko/squirrel_gc_bridge.h"
#include <cstddef>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqarray.h"

extern "C" {
int32_t retdec_gc_object_type(int32_t object);
void retdec_gc_finalize_collectable(int32_t object, int32_t type);
}

namespace {
static_assert(sizeof(void *) == 4);
static_assert(offsetof(SQCollectable, _uiRef) == 4);
static_assert(offsetof(SQCollectable, _next) == 12);
static_assert(offsetof(SQCollectable, _prev) == 16);
static_assert(offsetof(SQCollectable, _sharedstate) == 20);
static_assert(offsetof(SQSharedState, _gc_chain) == 68);
static_assert(offsetof(SQVM, _sharedstate) == 140);
static_assert(sizeof(SQArray) == 36 && offsetof(SQArray, _values) == 24);

template <typename T>
T *pointer(int32_t address) {
    return reinterpret_cast<T *>(static_cast<uintptr_t>(static_cast<uint32_t>(address)));
}
int32_t address(const void *value) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
}

// Squirrel 2.2.2 END_MARK: splice into the caller's live chain, not a local
// copy of the pointer to that chain. Use the upstream link operations.
extern "C" void kinoko_sq_gc_move_marked(int32_t object, int32_t type, int32_t *live_head) {
    auto *node = pointer<SQCollectable>(object);
    // SQVM shadows the base _sharedstate member; its constructor uses +140.
    auto *owner = type == OT_THREAD ? pointer<SQVM>(object)->_sharedstate : node->_sharedstate;
    if (owner)
        SQCollectable::RemoveFromChain(&owner->_gc_chain, node);
    auto *head = pointer<SQCollectable>(*live_head);
    SQCollectable::AddToChain(&head, node);
    *live_head = address(head);
}

// SQSharedState::CollectGarbage sweep/unmark ordering from the supplied source.
// Keep recovered Finalize bodies until all virtual slots are source compatible.
extern "C" int32_t kinoko_sq_gc_sweep(int32_t shared_state, int32_t live_head) {
    auto &state = *pointer<SQSharedState>(shared_state);
    int32_t visited = 0;
    auto *node = state._gc_chain;
    while (node) {
        const int32_t type = retdec_gc_object_type(address(node));
        ++node->_uiRef;
        if (type)
            retdec_gc_finalize_collectable(address(node), type);
        // Finalize can release another unreachable node and rewire _next.
        auto *next = node->_next;
        if (--node->_uiRef == 0 && type)
            node->Release();
        node = next;
        ++visited;
    }
    auto *head = pointer<SQCollectable>(live_head);
    for (node = head; node; node = node->_next)
        node->UnMark();
    state._gc_chain = head;
    return visited;
}

// 491BF0: the generated C body lost ECX and freed an uninitialized local.
extern "C" void __fastcall kinoko_sq_vm_release(int32_t vm, void *) {
    kinoko_sq_vm_delete(vm, nullptr, 1);
}

// 48D430 has the same lost-ECX defect as 491BF0.
extern "C" void __fastcall kinoko_sq_array_release(int32_t array, void *) {
    kinoko_sq_array_delete(array, nullptr, 1);
}

extern "C" int32_t __fastcall kinoko_sq_vm_delete(int32_t vm, void *, int32_t flags) {
    // Qualified destruction bypasses the recovered scalar-deleting vtable slot.
    pointer<SQVM>(vm)->SQVM::~SQVM();
    if (flags & 1)
        sq_vm_free(pointer<void>(vm), sizeof(SQVM));
    return vm;
}

extern "C" int32_t __fastcall kinoko_sq_array_delete(int32_t array, void *, int32_t flags) {
    auto *object = pointer<SQArray>(array);
    // SQArray's destructor is private. Spell out its source sequence, then
    // destroy its vector and base, rather than dispatching the damaged C slot.
    if (!(object->_uiRef & MARK_FLAG))
        SQCollectable::RemoveFromChain(&object->_sharedstate->_gc_chain, object);
    object->_values.~sqvector<SQObjectPtr>();
    object->SQCollectable::~SQCollectable();
    if (flags & 1)
        sq_vm_free(object, sizeof(SQArray));
    return array;
}
