#include "kinoko/stage_cleanup.h"
#include "kinoko/legacy_abi.h"
#include "kinoko/actor_cleanup.h"
#include <cstddef>
#include <cstdlib>

extern "C" {
int32_t function_450020(int32_t resource);
extern int32_t g603, g604;
extern int32_t g638, g639;
int32_t function_40b3a0(void);
}

namespace {
struct StageOwner {
    void *source;
    void *data;
    int32_t *runtime;
};
struct StageNode {
    StageNode *next, *previous;
    StageOwner *owner;
};
static_assert(sizeof(StageNode) == 12 && sizeof(StageOwner) == 12);
static_assert(offsetof(StageOwner, runtime) == 8);

void destroy_owner(StageOwner *owner) {
    if (!owner) return;
    std::free(owner->data);
    owner->data = nullptr;
    // BeginStage currently borrows the source ACT. Detach this non-owning
    // alias before destroying its owner; cloned ACTs remain runtime-owned.
    if (owner->runtime && owner->runtime[3] ==
            static_cast<int32_t>(reinterpret_cast<uintptr_t>(owner->source)))
        owner->runtime[3] = 0;
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
