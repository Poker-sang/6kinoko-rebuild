#include "kinoko/actor_cleanup.h"

#include <cstddef>
#include <cstdlib>

extern "C" {
int32_t function_405d60(int32_t texture_handle);
int32_t function_463730(int32_t actor_tree);
extern int32_t g23;
}

namespace {
struct AnimationTreeNode {
    AnimationTreeNode *left, *parent, *right;
    int32_t key, value;
    uint8_t color, sentinel;
};
struct PriorityTreeNode {
    PriorityTreeNode *left, *parent, *right;
    int32_t value;
    uint8_t color, sentinel;
};
struct AnimationListNode {
    AnimationListNode *next, *previous;
    uint8_t unknown[8];
    uint32_t frames_begin, frames_end, frames_capacity;
};
struct AnimationList {
    AnimationListNode *head;
    uint32_t size;
};

static_assert(sizeof(void *) == 4);
static_assert(sizeof(AnimationTreeNode) == 24);
static_assert(offsetof(AnimationTreeNode, sentinel) == 21);
static_assert(sizeof(PriorityTreeNode) == 20);
static_assert(offsetof(PriorityTreeNode, sentinel) == 17);
static_assert(offsetof(AnimationListNode, frames_begin) == 16);

template <typename T>
T *pointer(int32_t address) {
    return reinterpret_cast<T *>(static_cast<uintptr_t>(static_cast<uint32_t>(address)));
}
int32_t address(const void *value) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}

// Original 429C70/4634D0: recurse right, then delete while walking left.
// The sentinel and payloads are owned by the surrounding container.
template <typename Node>
Node *erase_subtree(Node *node) {
    while (node && !node->sentinel) {
        erase_subtree(node->right);
        Node *next = node->left;
        std::free(node);
        node = next;
    }
    return node;
}

template <typename Node>
void clear_tree(Node *head, int32_t &size) {
    if (head) {
        erase_subtree(head->parent);
        head->left = head;
        head->parent = head;
        head->right = head;
    }
    size = 0;
}
}

extern "C" int32_t kinoko_erase_animation_tree(int32_t node) {
    return address(erase_subtree(pointer<AnimationTreeNode>(node)));
}
extern "C" int32_t kinoko_erase_priority_tree(int32_t node) {
    return address(erase_subtree(pointer<PriorityTreeNode>(node)));
}

// 464D90: unlink the owning list before releasing frame payloads and storage.
extern "C" int32_t kinoko_clear_animation_list(int32_t list_address) {
    if (!list_address)
        return 0;
    auto &list = *pointer<AnimationList>(list_address);
    auto *head = list.head;
    if (!head) {
        list.size = 0;
        return 0;
    }
    auto *node = head->next;
    head->next = head;
    head->previous = head;
    list.size = 0;
    while (node && node != head) {
        auto *next = node->next;
        const int32_t begin = static_cast<int32_t>(node->frames_begin);
        const int32_t end = static_cast<int32_t>(node->frames_end);
        if (begin && end >= begin) {
            for (uint32_t frame = node->frames_begin; frame != node->frames_end; frame += 248) {
                std::free(pointer<void>(*pointer<int32_t>(static_cast<int32_t>(frame + 244))));
                *pointer<int32_t>(static_cast<int32_t>(frame)) = g23;
            }
            std::free(pointer<void>(begin));
        }
        node->frames_begin = node->frames_end = node->frames_capacity = 0;
        std::free(node);
        node = next;
    }
    return 0;
}

// 464E20: texture handles, live actors, nonowning lookup, owning animations,
// priority tree, then transient iteration state. Keep capacities and sentinels.
extern "C" int32_t kinoko_clear_actor_manager(int32_t manager) {
    auto *words = pointer<int32_t>(manager);
    for (int32_t index = 0; index < (words[18] - words[17]) / 4; ++index)
        function_405d60(pointer<int32_t>(words[17])[index]);
    words[18] = words[17];
    function_463730(manager + 84);
    clear_tree(pointer<AnimationTreeNode>(words[10]), words[11]);
    kinoko_clear_animation_list(manager + 52);
    clear_tree(pointer<PriorityTreeNode>(words[22]), words[23]);
    words[26] = words[25];
    words[29] = 0;
    pointer<uint8_t>(manager)[120] = 0;
    return words[25];
}
