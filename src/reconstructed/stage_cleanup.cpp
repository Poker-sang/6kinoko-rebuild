#include "kinoko/stage_cleanup.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/act_resource_records.hpp"
#include <cstddef>
#include <cstdlib>
#include <new>

extern "C" {
int32_t function_450020(int32_t resource);
extern int32_t g603, g604;
extern int32_t g638, g639;
extern int32_t g613;
int32_t function_40b3a0(void);
}

namespace {
struct StageOwner {
    void *source; // owned ACT source; released through its original destructor
    void *data;   // owned auxiliary allocation
    void *runtime; // owned runtime allocation; may borrow source during setup
};
struct StageNode {
    StageNode *next, *previous;
    StageOwner *owner;
};
struct RenderQueueNode {
    RenderQueueNode *next, *previous;
    void *payload;
};
static_assert(sizeof(RenderQueueNode) == 12);
static_assert(sizeof(StageNode) == 12 && sizeof(StageOwner) == 12);
static_assert(offsetof(StageOwner, runtime) == 8);

void destroy_owner(StageOwner *owner) {
    if (!owner) return;
    std::free(owner->data);
    owner->data = nullptr;
    // Detach legacy borrowed fixtures before destroying their source;
    // BeginStage clones remain independently owned by the runtime.
    if (owner->runtime) {
        const kinoko::native::RecordView<kinoko::act::RuntimeRecord> runtime(owner->runtime);
        if (runtime.get(&kinoko::act::RuntimeRecord::act) ==
                reinterpret_cast<uintptr_t>(owner->source))
            runtime.set(&kinoko::act::RuntimeRecord::act, uint32_t{0});
    }
    if (owner->source) {
        auto **vtable = *static_cast<void ***>(owner->source);
        retdec_call_thiscall1(owner->source, vtable[4], 1);
        owner->source = nullptr;
    }
    if (owner->runtime) {
        function_450020(static_cast<int32_t>(reinterpret_cast<uintptr_t>(owner->runtime)));
        std::free(owner->runtime);
        owner->runtime = nullptr;
    }
    std::free(owner);
}
}

// Original 4D3E50 owns a 12-byte circular list sentinel. 4D3EB0 is a
// separate initializer; it must not be merged into this allocation's failure
// path. The payload word is unused on the sentinel, as in the original.
extern "C" void kinoko_initialize_render_queue() {
    auto *head = static_cast<RenderQueueNode *>(std::malloc(sizeof(RenderQueueNode)));
    if (!head) throw std::bad_alloc();
    head->next = head->previous = head;
    g613 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(head));
}

// 465F70: destroy payloads first, reset the list, then release its nodes.
extern "C" int32_t kinoko_clear_global_stages() {
    auto *head = reinterpret_cast<StageNode *>(static_cast<uintptr_t>(static_cast<uint32_t>(g603)));
    if (!head) return 0;
    for (auto *node = head->next; node != head; node = node->next) {
        destroy_owner(node->owner);
        node->owner = nullptr;
    }
    auto *node = head->next;
    head->next = head->previous = head;
    g604 = 0;
    while (node != head) {
        auto *next = node->next;
        std::free(node);
        node = next;
    }
    return g603;
}

// 470890: the rebuilt sound manager owns buffers in its SE pool and BGM
// track records. Join its workers before releasing that storage; then clear
// the non-owning original ID lookup tree, preserving its sentinel.
extern "C" int32_t kinoko_clear_global_sound() {
    function_40b3a0();
    auto *head = reinterpret_cast<int32_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g638)));
    if (head) {
        kinoko_erase_animation_tree(head[1]);
        head[0] = head[1] = head[2] = g638;
    }
    g639 = 0;
    return 1;
}
