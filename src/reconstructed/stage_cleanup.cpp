#include "kinoko/integer_map.h"
#include "kinoko/render_queue.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/actor_cleanup.h"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_resource.h"
#include <cstddef>
#include <cstdlib>
#include <new>
#include <list>
#include "kinoko/legacy_memory.hpp"

extern "C" {
extern int32_t g603, g604;
extern int32_t g638, g639;
int32_t function_40b3a0(void);
}

using StageList = std::list<KinokoStageNode>;
struct KinokoStageNode { KinokoStageOwner *owner; StageList::iterator position; };

namespace {
using namespace kinoko::stage;
using kinoko::native::RecordView;
using kinoko::legacy::pointer;
StageList* stages() { return kinoko::legacy::pointer<StageList>(g603); }
void release_stage_list() {
    delete stages();g603=g604=0;
}
void release_render_queue() { kinoko_clear_render_queue(); }
void release_sound_tree() {
    kinoko_integer_map_destroy(g638);g638=g639=0;
}


}

extern "C" void kinoko_stage_owner_destroy(KinokoStageOwner *storage) {
    if (!storage) return;
    const OwnerView owner(storage);
    const auto source = owner.get(&OwnerRecord::document);
    auto *runtime_pointer = owner.get(&OwnerRecord::runtime);
    const auto holder = owner.get(&OwnerRecord::holder);
    std::free(holder);
    owner.set(&OwnerRecord::holder, static_cast<KinokoActSourceHolder *>(nullptr));
    if (source) {
        const auto *methods = kinoko::legacy::load<DocumentPrefix>(source).vtable;
        // Genuine thiscall virtual dispatch (465FCC); EDX is not an argument.
        const auto destroy = kinoko::legacy::load<DocumentVirtuals>(methods).deleting_destructor;
        destroy(source, 1);
        owner.set(&OwnerRecord::document, static_cast<KinokoActDocument *>(nullptr));
    }
    // The source destructor is a callback and may update the owner record.
    runtime_pointer = owner.get(&OwnerRecord::runtime);
    if (runtime_pointer) {
        kinoko_act_runtime_dispose(runtime_pointer);
        std::free(runtime_pointer);
    }
    std::free(storage);
}

// Use real CRT registration and callable source addresses; original absolute
// executable addresses cannot be registered in the reconstructed process.
extern "C" int32_t function_4d3ce0() {
    kinoko_stage_list_construct();
    return std::atexit(release_stage_list);
}

extern "C" int32_t function_4d3e50() {
    kinoko_initialize_render_queue();
    return std::atexit(release_render_queue);
}

extern "C" int32_t function_4d3f50() {
    g638=kinoko_integer_map_create();g639=0;
    return std::atexit(release_sound_tree);
}

// 465F70: destroy payloads first, reset the list, then release its nodes.
extern "C" int32_t kinoko_clear_global_stages() {
    if(!stages()) return 0;
    for(auto& entry:*stages()) { kinoko_stage_owner_destroy(entry.owner);entry.owner=nullptr; }
    stages()->clear();g604=0;return g603;
}

// 470890: the rebuilt sound manager owns buffers in its SE pool and BGM
// track records. Join its workers before releasing that storage; then clear
// the non-owning original ID lookup tree, preserving its sentinel.
extern "C" int32_t kinoko_clear_global_sound() {
    function_40b3a0();
    kinoko_integer_map_clear(g638);
    g639 = 0;
    return 1;
}

extern "C" void kinoko_stage_list_construct() { g603=kinoko::legacy::address(new StageList);g604=0; }
extern "C" void kinoko_stage_list_destroy() { release_stage_list(); }
extern "C" KinokoStageNode *kinoko_stage_list_end() {
    return pointer<KinokoStageNode>(g603);
}
extern "C" KinokoStageNode *kinoko_stage_list_first() {
    return stages() && !stages()->empty() ? &stages()->front() : kinoko_stage_list_end();
}
extern "C" KinokoStageNode *kinoko_stage_list_next(KinokoStageNode *node) {
    auto position = node->position;
    ++position;
    return position == stages()->end() ? kinoko_stage_list_end() : &*position;
}
extern "C" KinokoStageOwner *kinoko_stage_list_value(const KinokoStageNode *node) {
    return node->owner;
}
extern "C" KinokoStageNode *kinoko_stage_list_append(KinokoStageOwner *owner) {
    stages()->emplace_back();
    auto position = std::prev(stages()->end());
    position->owner = owner;
    position->position = position;
    g604 = static_cast<int32_t>(stages()->size());
    return &*position;
}
extern "C" int32_t function_4d47f0() {
    if(stages()) stages()->clear();g604=0;return g603;
}
